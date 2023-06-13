
// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later
#define _USE_MATH_DEFINES

#include <algorithm>
#include <math.h>

#include <QVector4D>
#include <QSGRenderNode>
#include <QQuickWindow>

#include <private/qtriangulator_p.h>

#include "riveqtrhirenderer.h"
#include "src/qtquick/rhi/texturetargetnode.h"

RiveQtRhiGLPath::RiveQtRhiGLPath(const unsigned segmentCount)
{
    m_path.setFillRule(Qt::FillRule::WindingFill);
    setSegmentCount(segmentCount);
}

RiveQtRhiGLPath::RiveQtRhiGLPath(const RiveQtRhiGLPath &other)
{
    m_path = other.m_path;
    m_subPaths = other.m_subPaths;
    m_pathSegmentsData = other.m_pathSegmentsData;
    m_pathSegmentsOutlineData = other.m_pathSegmentsOutlineData;
    m_pathOutlineData = other.m_pathOutlineData;
    m_segmentCount = other.m_segmentCount;
}

RiveQtRhiGLPath::RiveQtRhiGLPath(rive::RawPath &rawPath, rive::FillRule fillRule, const unsigned segmentCount)
{
    m_path.clear();
    m_path.setFillRule(RiveQtUtils::riveFillRuleToQt(fillRule));
    setSegmentCount(segmentCount);

    for (const auto &[verb, pts] : rawPath) {
        switch (verb) {
        case rive::PathVerb::move:
            m_path.moveTo(pts->x, pts->y);
            break;
        case rive::PathVerb::line:
            m_path.lineTo(pts->x, pts->y);
            break;
        case rive::PathVerb::cubic:
            m_path.cubicTo(pts[0].x, pts[0].y, pts[1].x, pts[1].y, pts[2].x, pts[2].y);
            break;
        case rive::PathVerb::close:
            m_path.lineTo(pts->x, pts->y);
            m_path.closeSubpath();
            m_isClosed = true;
            break;
        default:
            break;
        }
    }

    m_pathSegmentOutlineDataDirty = true;
    m_pathSegmentDataDirty = true;
}

void RiveQtRhiGLPath::rewind()
{
    m_pathSegmentsData.clear();
    m_pathSegmentsOutlineData.clear();
    m_subPaths.clear();
    m_path.clear();
}

void RiveQtRhiGLPath::fillRule(rive::FillRule value)
{
    switch (value) {
    case rive::FillRule::evenOdd:
        m_path.setFillRule(Qt::FillRule::OddEvenFill);
        break;
    case rive::FillRule::nonZero:
        m_path.setFillRule(Qt::FillRule::WindingFill);
        break;
    }
}

void RiveQtRhiGLPath::addRenderPath(RenderPath *path, const rive::Mat2D &m)
{
    if (!path) {
        return;
    }

    RiveQtRhiGLPath *qtPath = static_cast<RiveQtRhiGLPath *>(path);
    QMatrix4x4 matrix(m[0], m[2], 0, m[4], m[1], m[3], 0, m[5], 0, 0, 1, 0, 0, 0, 0, 1);

    m_subPaths.emplace_back(RhiSubPath(qtPath, matrix));

    m_pathSegmentOutlineDataDirty = true;
    m_pathSegmentDataDirty = true;

    // calculate vertices directly when we add a path
    generateVertices();
}

void RiveQtRhiGLPath::setQPainterPath(QPainterPath path)
{
    m_path = path;
}

void RiveQtRhiGLPath::setSegmentCount(const unsigned segmentCount)
{
    if (segmentCount == 0u) {
        m_segmentCount = 1u;
        return;
    }

    m_segmentCount = std::min(segmentCount, 100u);
}

QVector<QVector<QVector2D>> RiveQtRhiGLPath::toVertices()
{
    if (m_pathSegmentDataDirty) {
        generateVertices();
    }
    return m_pathSegmentsData;
}

// TODO: Optimize, this gets called each frame for a stroke/path
// there will be shortcuts and caching possible
QVector<QVector<QVector2D>> RiveQtRhiGLPath::toVerticesLine(const QPen &pen)
{
    if (m_pathSegmentOutlineDataDirty) {
        generateOutlineVertices();
    }

    m_pathOutlineData.clear();

    if (m_pathSegmentsOutlineData.isEmpty()) {
        return m_pathOutlineData;
    }

    qreal lineWidth = pen.widthF();

    Qt::PenJoinStyle joinType = pen.joinStyle();
    Qt::PenCapStyle capStyle = pen.capStyle();

    if (m_pathSegmentsOutlineData.isEmpty()) {
        generateVertices();

        if (m_pathSegmentsOutlineData.isEmpty()) {
            return m_pathOutlineData;
        }
    }

    for (QVector<QVector2D> pathData : m_pathSegmentsOutlineData) {
        if (pathData.size() <= 1) {
            continue;
        }

        QVector<QVector2D> lineDataSegment;

        int count = pathData.count();
        int endIndex = count; // m_isClosed ? count : count - 1;

        auto addMiterJoin = [&](const QVector2D &p1, const QVector2D &p2, const QVector2D &offset) {
            lineDataSegment.append(p1 + offset);
            lineDataSegment.append(p2 + offset);
            lineDataSegment.append(p1 - offset);
            lineDataSegment.append(p2 - offset);
        };

        auto addRoundJoin = [&](const QVector2D &p1, const QVector2D &p2, const QVector2D &offset, int numSegments) {
            QVector2D prevPoint1 = p1 - offset;
            QVector2D prevPoint2 = p1 + offset;

            float angleStep = M_PI / numSegments;
            float angle = atan2(offset.y(), offset.x());

            for (int i = 0; i < numSegments; ++i) {
                float newAngle = angle + angleStep * (i + 1);
                QVector2D newOffset(cos(newAngle) * lineWidth / 2, sin(newAngle) * lineWidth / 2);
                QVector2D newPoint1 = p1 - newOffset;
                QVector2D newPoint2 = p1 + newOffset;

                lineDataSegment.append(prevPoint1);
                lineDataSegment.append(prevPoint2);
                lineDataSegment.append(newPoint1);

                lineDataSegment.append(newPoint1);
                lineDataSegment.append(prevPoint2);
                lineDataSegment.append(newPoint2);

                prevPoint1 = newPoint1;
                prevPoint2 = newPoint2;
            }

            lineDataSegment.append(prevPoint1);
            lineDataSegment.append(prevPoint2);
            lineDataSegment.append(p2 - offset);

            lineDataSegment.append(p2 - offset);
            lineDataSegment.append(prevPoint2);
            lineDataSegment.append(p2 + offset);

            lineDataSegment.append(p1 + offset);
            lineDataSegment.append(p2 + offset);
            lineDataSegment.append(p1 - offset);
            lineDataSegment.append(p2 - offset);
        };

        auto addBevelJoin = [&](const QVector2D &p1, const QVector2D &p2, const QVector2D &offset) {
            QVector2D p1_outer = p1 - offset;
            QVector2D p1_inner = p1 + offset;
            QVector2D p2_outer = p2 - offset;
            QVector2D p2_inner = p2 + offset;

            lineDataSegment.append(p1_outer);
            lineDataSegment.append(p1_inner);
            lineDataSegment.append(p2_inner);

            lineDataSegment.append(p1_outer);
            lineDataSegment.append(p2_inner);
            lineDataSegment.append(p2_outer);
        };

        auto addCap = [&](const QVector2D &p, const QVector2D &offset, const QVector2D &normal, bool isStart) {
            switch (capStyle) {
            case Qt::PenCapStyle::FlatCap:
                // No additional vertices needed for FlatCap
                break;
            case Qt::PenCapStyle::RoundCap: {
                int numSegments = 20;
                float angleStep = M_PI / numSegments;
                float startAngle = atan2(offset.y(), offset.x());

                QVector<QVector2D> capVertices;
                QVector2D previousOffset(cos(startAngle) * lineWidth / 2, sin(startAngle) * lineWidth / 2);
                for (int i = 0; i < numSegments + 1; ++i) {
                    float newAngle = startAngle + angleStep * (i + 1);
                    QVector2D newOffset(cos(newAngle) * lineWidth / 2, sin(newAngle) * lineWidth / 2);

                    capVertices.append(p);
                    capVertices.append(p + previousOffset);
                    capVertices.append(p + newOffset);

                    previousOffset = newOffset;
                }

                if (isStart) {
                    lineDataSegment = capVertices + lineDataSegment;
                } else {
                    lineDataSegment.append(capVertices);
                }

                break;
            }
            case Qt::PenCapStyle::SquareCap: {
                QVector<QVector2D> capVertices;
                capVertices.append(p - normal * (lineWidth / 2) + offset);
                capVertices.append(p + normal * (lineWidth / 2) + offset);
                capVertices.append(p - normal * (lineWidth / 2) - offset);
                capVertices.append(p + normal * (lineWidth / 2) - offset);

                if (isStart) {
                    lineDataSegment = capVertices + lineDataSegment;
                } else {
                    lineDataSegment.append(capVertices);
                }
                break;
            }
            default:
                // No additional vertices needed for other cap styles
                break;
            }
        };

        for (int i = 0; i < (m_isClosed ? endIndex : endIndex - 1); ++i) {
            const QVector2D p1 = pathData[i];
            const QVector2D p2 = pathData[(i + 1) % (endIndex)]; // if endIndex, take 0
            const QVector2D diff = p2 - p1;

            const QVector2D normal = QVector2D(-diff.y(), diff.x()).normalized();

            const QVector2D offset = normal * (lineWidth / 2.0);

            if (!m_isClosed && (i == 0 || (i == endIndex && endIndex > 3))) {
                addRoundJoin(p1, p2, offset, 20);
            } else {
                switch (joinType) {
                case Qt::PenJoinStyle::MiterJoin:
                    // Miter Join may not work so well, specifically on tesselated segmented bezier curves
                    // disable it for now,.. we may need to the join specific on the source (like curve)
                    // addMiterJoin(p1, p2, offset);
                    // break;
                    // the fallthrough is intendend here
                case Qt::PenJoinStyle::RoundJoin:
                    addRoundJoin(p1, p2, offset, 20);
                    break;
                case Qt::PenJoinStyle::BevelJoin:
                    addBevelJoin(p1, p2, offset);
                    break;
                default:
                    // add both while this shouldn't end here since we handle all known rive cases
                    // SvgMiterJoin = 0x100,
                    // MPenJoinStyle = 0x1c0
                    addMiterJoin(p1, p2, offset);
                    addRoundJoin(p1, p2, offset, 20);
                }
            }
        }

        if (!m_isClosed) {
            QVector2D startNormal(-pathData[1].y() + pathData[0].y(), pathData[1].x() - pathData[0].x());
            startNormal.normalize();
            QVector2D endNormal(-pathData.last().y() + pathData[count - 2].y(), pathData.last().x() - pathData[count - 2].x());
            endNormal.normalize();

            addCap(pathData.first(), startNormal * (lineWidth / 2.0), startNormal, true);
            addCap(pathData.last(), -endNormal * (lineWidth / 2.0), endNormal, false);
        }

        // TODO: this converts the created TRIANGLE STRIP to TRIANGLES
        // We should create TRIANGLES to start with...
        // This is done to allow a multipass shader for blending
        QVector<QVector2D> triangledSegmentData;
        // make it triangles!
        for (int i = 0; i < lineDataSegment.size() - 2; ++i) {
            if (i % 2 == 0) {
                triangledSegmentData.push_back(lineDataSegment[i]);
                triangledSegmentData.push_back(lineDataSegment[i + 1]);
                triangledSegmentData.push_back(lineDataSegment[i + 2]);
            } else {
                triangledSegmentData.push_back(lineDataSegment[i]);
                triangledSegmentData.push_back(lineDataSegment[i + 2]);
                triangledSegmentData.push_back(lineDataSegment[i + 1]);
            }
        }

        m_pathOutlineData.append(triangledSegmentData);
    }

    return m_pathOutlineData;
}

QPointF RiveQtRhiGLPath::cubicBezier(const QPointF &startPoint, const QPointF &controlPoint1, const QPointF &controlPoint2,
                                     const QPointF &endPoint, qreal t)
{
    qreal oneMinusT = 1 - t;
    qreal oneMinusTSquared = oneMinusT * oneMinusT;
    qreal oneMinusTCubed = oneMinusTSquared * oneMinusT;
    qreal tSquared = t * t;
    qreal tCubed = tSquared * t;

    QPointF point = oneMinusTCubed * startPoint + 3 * oneMinusTSquared * t * controlPoint1 + 3 * oneMinusT * tSquared * controlPoint2
        + tCubed * endPoint;
    return point;
}

QVector<QVector2D> RiveQtRhiGLPath::qpainterPathToVector2D(const QPainterPath &path)
{
    QVector<QVector2D> pathData;

    if (path.isEmpty()) {
        return pathData;
    }

    QTriangleSet triangles = qTriangulate(path);
    int index;

    for (int i = 0; i < triangles.indices.size(); i++) {
        if (triangles.indices.type() == QVertexIndexVector::UnsignedInt) {
            index = static_cast<const quint32 *>(triangles.indices.data())[i];
        } else {
            index = static_cast<const quint16 *>(triangles.indices.data())[i];
        }

        const qreal x = triangles.vertices[2 * index];
        const qreal y = triangles.vertices[2 * index + 1];

        pathData.append(QVector2D(x, y));
    }

    return pathData;
}

QVector<QVector2D> RiveQtRhiGLPath::qpainterPathToOutlineVector2D(const QPainterPath &path)
{
    QVector<QVector2D> pathData;

    if (path.isEmpty()) {
        return pathData;
    }

    const QPointF &point = path.elementAt(0);
    const QVector2D &centerPoint = QVector2D(point.x(), point.y());

    // Add the center point of the triangle fan.
    pathData.append(centerPoint);

    for (int i = 1; i < path.elementCount(); ++i) {
        QPainterPath::Element element = path.elementAt(i);

        switch (element.type) {
        case QPainterPath::MoveToElement:
        case QPainterPath::LineToElement: {
            QPointF point = element;
            pathData.append(QVector2D(point.x(), point.y()));
            break;
        }

        case QPainterPath::CurveToElement: {
            const QPointF &startPoint = pathData.last().toPointF();
            const QPointF &controlPoint1 = element;
            const QPointF &controlPoint2 = path.elementAt(i + 1);
            const QPointF &endPoint = path.elementAt(i + 2);

            for (int j = 1; j <= m_segmentCount; ++j) {
                const qreal t = static_cast<qreal>(j) / m_segmentCount;
                const QPointF &point = cubicBezier(startPoint, controlPoint1, controlPoint2, endPoint, t);
                pathData.append(QVector2D(point.x(), point.y()));
            }

            i += 2; // Skip the next two control points, as we already processed them.
            break;
        }
        default:
            break;
        }
    }

    if (pathData.first() == pathData.last()) {
        m_isClosed = true;
    }

    if (m_isClosed) {
        // Connect the last point to the first point.
        pathData.append(QVector2D(point.x(), point.y()));
    }

    return pathData;
}

void RiveQtRhiGLPath::generateVertices()
{
    if (!m_pathSegmentDataDirty) {
        return;
    }

    if (m_path.isEmpty() && m_subPaths.empty()) {
        m_pathSegmentsData.clear();
        return;
    }

    m_pathSegmentsData.clear();

    m_pathSegmentsData.append(qpainterPathToVector2D(m_path));

    for (RhiSubPath &RhiSubPath : m_subPaths) {
        QPainterPath sourcePath = RhiSubPath.path()->toQPainterPath();
        QPainterPath transformedPath = RiveQtUtils::transformPathWithMatrix4x4(sourcePath, RhiSubPath.transform());

        m_pathSegmentsData.append(RhiSubPath.path()->m_pathSegmentsData);
        m_pathSegmentsData.append(qpainterPathToVector2D(transformedPath));
    }
    m_pathSegmentDataDirty = false;
}

void RiveQtRhiGLPath::generateOutlineVertices()
{
    if (!m_pathSegmentOutlineDataDirty) {
        return;
    }

    if (m_path.isEmpty() && m_subPaths.empty()) {
        m_pathSegmentsOutlineData.clear();
        return;
    }

    m_pathSegmentsOutlineData.clear();
    m_pathSegmentsOutlineData.append(qpainterPathToOutlineVector2D(m_path));

    for (RhiSubPath &RhiSubPath : m_subPaths) {
        QPainterPath sourcePath = RhiSubPath.path()->toQPainterPath();
        QPainterPath transformedPath = RiveQtUtils::transformPathWithMatrix4x4(sourcePath, RhiSubPath.transform());

        m_pathSegmentsOutlineData.append(qpainterPathToOutlineVector2D(transformedPath));
        m_pathSegmentsOutlineData.append(RhiSubPath.path()->m_pathSegmentsOutlineData);
    }
    m_pathSegmentOutlineDataDirty = false;
}

RiveQtRhiRenderer::RiveQtRhiRenderer(QQuickItem *item)
    : rive::Renderer()
    , m_item(item)
{
    m_rhiRenderStack.push_back(RhiRenderState());
}

RiveQtRhiRenderer::~RiveQtRhiRenderer()
{
    for (TextureTargetNode *textureTargetNode : m_renderNodes) {
        delete textureTargetNode;
    }
}

void RiveQtRhiRenderer::save()
{
    m_rhiRenderStack.push_back(m_rhiRenderStack.back());
    m_rhiRenderStack.back().stackNodes.clear();
}

void RiveQtRhiRenderer::restore()
{
    assert(m_rhiRenderStack.size() > 1);
    m_rhiRenderStack.pop_back();
    m_rhiRenderStack.back().opacity = 1.0;
}

void RiveQtRhiRenderer::transform(const rive::Mat2D &transform)
{
    QMatrix4x4 &stackMat = m_rhiRenderStack.back().transform;
    QMatrix4x4 matrix(transform[0], transform[2], 0, transform[4], transform[1], transform[3], 0, transform[5], 0, 0, 1, 0, 0, 0, 0, 1);

    stackMat = stackMat * matrix;
}

void RiveQtRhiRenderer::drawPath(rive::RenderPath *path, rive::RenderPaint *paint)
{
    if (!path || !paint) {
        return;
    }

    RiveQtRhiGLPath *qtPath = static_cast<RiveQtRhiGLPath *>(path);
    RiveQtPaint *qtPaint = static_cast<RiveQtPaint *>(paint);

    QVector<QVector<QVector2D>> pathData;

    if (qtPaint->paintStyle() == rive::RenderPaintStyle::fill) {
        pathData = qtPath->toVertices();
    }

    if (qtPaint->paintStyle() == rive::RenderPaintStyle::stroke) {
        pathData = qtPath->toVerticesLine(qtPaint->pen());
    }

    QColor color = qtPaint->color();

    TextureTargetNode *node = getRiveDrawTargetNode();

    m_rhiRenderStack.back().opacity = qtPaint->opacity();

    // opacity is used to apply the strength of the blending effect/layer, though
    // for some reason it looks like it failes in case gradients are used
    // look at that later
    node->setOpacity(currentOpacity()); // inherit the opacity from the parent
    node->setBlendMode(qtPaint->blendMode());

    if (color.isValid()) {
        node->setColor(color);
    } else {
        node->setGradient(qtPaint->brush().gradient());
    }

#if 0 // this allows to draw the clipping area which it useful for debugging :)
  TextureTargetNode *drawClipping = getRiveDrawTargetNode();
  drawClipping->setOpacity(currentOpacity()); // inherit the opacity from the parent
  drawClipping->setColor(QColor(255, 0, 0, 5));
  drawClipping->updateGeometry(m_rhiRenderStack.back().clippingGeometry, QMatrix4x4());
#endif

    node->updateClippingGeometry(m_rhiRenderStack.back().clippingGeometry);
    node->updateGeometry(pathData, transformMatrix());

    m_rhiRenderStack.back().stackNodes.append(node);
}

void RiveQtRhiRenderer::clipPath(rive::RenderPath *path)
{
    // alternativly we could save the paths + transform to the node and
    // draw each one by one to the stencil buffer
    // -> I would guess that would be faster
    RiveQtRhiGLPath *qtPath = static_cast<RiveQtRhiGLPath *>(path);
    auto pathVertices = qtPath->toVertices();

    for (auto &path : pathVertices) {
        for (auto &point : path) {
            QVector4D vec4(point, 0.0f, 1.0f);
            vec4 = transformMatrix() * vec4;
            point = vec4.toVector2D();
        }
    }

    m_rhiRenderStack.back().clippingGeometry = pathVertices;

    for (TextureTargetNode *textureTargetNode : m_rhiRenderStack.back().stackNodes) {
        textureTargetNode->updateClippingGeometry(m_rhiRenderStack.back().clippingGeometry);
    }
}

void RiveQtRhiRenderer::drawImage(const rive::RenderImage *image, rive::BlendMode blendMode, float opacity)
{
    TextureTargetNode *node = getRiveDrawTargetNode();

    m_rhiRenderStack.back().opacity = opacity;
    node->setOpacity(currentOpacity()); // inherit the opacity from the parent
    node->setBlendMode(blendMode);

    node->setTexture(static_cast<const RiveQtImage *>(image)->image(), //
                     nullptr, nullptr, nullptr,
                     transformMatrix()); //

#if 0 // this allows to draw the clipping area which it usefull for debugging :)
    TextureTargetNode *drawClipping = getRiveDrawTargetNode();
    drawClipping->setOpacity(currentOpacity()); // inherit the opacity from the parent
    drawClipping->setColor(QColor(255, 0, 0, 50));
    drawClipping->updateGeometry(m_rhiRenderStack.back().clippingGeometry, QMatrix4x4());
#endif

    node->updateClippingGeometry(m_rhiRenderStack.back().clippingGeometry);
    m_rhiRenderStack.back().stackNodes.append(node);
}

void RiveQtRhiRenderer::drawImageMesh(const rive::RenderImage *image, rive::rcp<rive::RenderBuffer> vertices_f32,
                                      rive::rcp<rive::RenderBuffer> uvCoords_f32, rive::rcp<rive::RenderBuffer> indices_u16,
                                      rive::BlendMode blendMode, float opacity)
{
    TextureTargetNode *node = getRiveDrawTargetNode();

    m_rhiRenderStack.back().opacity = opacity;

    node->setOpacity(currentOpacity()); // inherit the opacity from the parent
    node->setBlendMode(blendMode);

    node->setTexture(static_cast<const RiveQtImage *>(image)->image(), //
                     static_cast<RiveQtBufferF32 *>(vertices_f32.get()), //
                     static_cast<RiveQtBufferF32 *>(uvCoords_f32.get()), //
                     static_cast<RiveQtBufferU16 *>(indices_u16.get()),
                     transformMatrix()); //

#if 0 // this allows to draw the clipping area which it usefull for debugging :)
  TextureTargetNode *drawClipping = getRiveDrawTargetNode();
  drawClipping->setOpacity(currentOpacity()); // inherit the opacity from the parent
  drawClipping->setColor(QColor(255, 0, 0, 50));
  drawClipping->updateGeometry(m_rhiRenderStack.back().clippingGeometry, QMatrix4x4());
#endif

    node->updateClippingGeometry(m_rhiRenderStack.back().clippingGeometry);
    m_rhiRenderStack.back().stackNodes.append(node);
}

void RiveQtRhiRenderer::render(QRhiCommandBuffer *cb)
{
    for (TextureTargetNode *textureTargetNode : m_renderNodes) {
        textureTargetNode->render(cb);
    }
}

TextureTargetNode *RiveQtRhiRenderer::getRiveDrawTargetNode()
{
    int layer = 0;
    TextureTargetNode *pathNode = nullptr;

    for (TextureTargetNode *textureTargetNode : m_renderNodes) {
        if (textureTargetNode->isRecycled()) {
            pathNode = textureTargetNode;
            pathNode->take();
            break;
        }

        layer++;
    }

    if (!pathNode) {
        pathNode = new TextureTargetNode(m_item, m_displayBuffer, m_viewportRect, &m_combinedMatrix, &m_projectionMatrix);
        pathNode->take();
        m_renderNodes.append(pathNode);
    }

    return pathNode;
}

void RiveQtRhiRenderer::setProjectionMatrix(const QMatrix4x4 *projectionMatrix, const QMatrix4x4 *modelMatrix)
{
    auto aspectX = m_item->width() / (m_artboardSize.width());
    auto aspectY = m_item->height() / (m_artboardSize.height());

    m_projectionMatrix = *projectionMatrix;
    m_projectionMatrix.scale((m_item->window()->width() / m_item->width()), (m_item->window()->height() / m_item->height()));

    m_combinedMatrix = *projectionMatrix;

    switch (m_fillMode) {
    case RenderSettings::Stretch: {
        m_combinedMatrix.scale((m_item->window()->width() / m_item->width()) * aspectX,
                               (m_item->window()->height() / m_item->height()) * aspectY);
        break;
    }
    case RenderSettings::PreserveAspectCrop: {
        auto scaleFactor = qMax(aspectX, aspectY);
        m_combinedMatrix.scale((m_item->window()->width() / m_item->width()) * scaleFactor,
                               (m_item->window()->height() / m_item->height()) * scaleFactor);
        break;
    }
    default:
    case RenderSettings::PreserveAspectFit: {
        auto scaleFactor = qMin(aspectX, aspectY);
        m_combinedMatrix.scale((m_item->window()->width() / m_item->width()) * scaleFactor,
                               (m_item->window()->height() / m_item->height()) * scaleFactor);
        break;
    }
    }
}

void RiveQtRhiRenderer::updateViewPort(const QRectF &viewportRect, QRhiTexture *displayBuffer)
{
    while (!m_renderNodes.empty()) {
        auto *textureTargetNode = m_renderNodes.last();
        m_renderNodes.removeAll(textureTargetNode);
        delete textureTargetNode;
    }

    m_viewportRect = viewportRect;
    m_displayBuffer = displayBuffer;
}

void RiveQtRhiRenderer::recycleRiveNodes()
{
    for (TextureTargetNode *textureTargetNode : m_renderNodes) {
        textureTargetNode->recycle();
    }
}

const QMatrix4x4 &RiveQtRhiRenderer::transformMatrix() const
{
    return m_rhiRenderStack.back().transform;
}

float RiveQtRhiRenderer::currentOpacity()
{
    float opacity = 1.0;
    for (const auto &renderState : m_rhiRenderStack) {
        opacity *= renderState.opacity;
    }
    return opacity;
}

RhiSubPath::RhiSubPath(RiveQtRhiGLPath *path, const QMatrix4x4 &transform)
    : m_Path(path)
    , m_Transform(transform)
{
}

RiveQtRhiGLPath *RhiSubPath::path() const
{
    return m_Path;
}
QMatrix4x4 RhiSubPath::transform() const
{
    return m_Transform;
}
