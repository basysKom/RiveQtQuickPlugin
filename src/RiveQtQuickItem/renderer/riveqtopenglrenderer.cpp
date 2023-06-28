
// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#define _USE_MATH_DEFINES
#include <math.h>

#include <QOpenGLFunctions>
#include <QOpenGLTexture>
#include <QOpenGLFramebufferObject>

#include <private/qtriangulator_p.h>

#include "rqqplogging.h"
#include "renderer/riveqtopenglrenderer.h"
#include "renderer/riveqtutils.h"

RiveQtOpenGLPath::RiveQtOpenGLPath(const RiveQtOpenGLPath &rqp)
{
    m_path = rqp.m_path;
    m_subPaths = rqp.m_subPaths;
    m_pathSegmentsData = rqp.m_pathSegmentsData;
    m_pathSegmentsOutlineData = rqp.m_pathSegmentsOutlineData;
}

RiveQtOpenGLPath::RiveQtOpenGLPath(rive::RawPath &rawPath, rive::FillRule fillRule)
{
    m_path.clear();
    m_path.setFillRule(RiveQtUtils::riveFillRuleToQt(fillRule));

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

    generateVertices();
}

void RiveQtOpenGLPath::rewind()
{
    m_pathSegmentsData.clear();
    m_pathSegmentsOutlineData.clear();
    m_subPaths.clear();
    m_path.clear();
}

void RiveQtOpenGLPath::fillRule(rive::FillRule value)
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

void RiveQtOpenGLPath::addRenderPath(RenderPath *path, const rive::Mat2D &m)
{
    if (!path) {
        return;
    }

    RiveQtOpenGLPath *qtPath = static_cast<RiveQtOpenGLPath *>(path);
    QMatrix4x4 matrix(m[0], m[2], 0, m[4], m[1], m[3], 0, m[5], 0, 0, 1, 0, 0, 0, 0, 1);

    m_subPaths.emplace_back(SubPath(qtPath, matrix));

    generateVertices();
}

QVector<QVector<QVector2D>> RiveQtOpenGLPath::toVertices() const
{
    return m_pathSegmentsData;
}

// TODO: Optimize, this gets called each frame for a stroke/path
// there will be shortcuts and caching possible
QVector<QVector<QVector2D>> RiveQtOpenGLPath::toVerticesLine(const QPen &pen) const
{
    qreal lineWidth = pen.widthF();
    Qt::PenJoinStyle joinType = pen.joinStyle();
    Qt::PenCapStyle capStyle = pen.capStyle();

    QVector<QVector<QVector2D>> lineData;

    if (m_pathSegmentsOutlineData.isEmpty()) {
        return lineData;
    }

    for (QVector<QVector2D> pathData : m_pathSegmentsOutlineData) {
        if (pathData.size() <= 1) {
            continue;
        }

        QVector<QVector2D> lineDataSegment;

        int count = pathData.count();
        int endIndex = m_isClosed ? count : count - 1;

        auto addMiterJoin = [&](const QVector2D &p1, const QVector2D &p2, const QVector2D &offset) {
            lineDataSegment.append(p1 - offset);
            lineDataSegment.append(p1 + offset);
            lineDataSegment.append(p2 - offset);
            lineDataSegment.append(p2 + offset);
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
                for (int i = 0; i < numSegments; ++i) {
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

        for (int i = 0; i < endIndex; ++i) {
            const QVector2D p1 = pathData[i];
            const QVector2D p2 = pathData[(i + 1) % count];
            const QVector2D diff = p2 - p1;

            QVector2D normal(-diff.y(), diff.x());
            normal.normalize();

            const QVector2D offset = normal * (lineWidth / 2.0);

            if (!m_isClosed && (i == 0 || i == endIndex - 1)) {
                addMiterJoin(p1, p2, offset);
            } else {
                switch (joinType) {
                case Qt::PenJoinStyle::MiterJoin:
                    addMiterJoin(p1, p2, offset);
                    break;
                case Qt::PenJoinStyle::RoundJoin:
                    addRoundJoin(p1, p2, offset, 10);
                    break;
                case Qt::PenJoinStyle::BevelJoin:
                    addBevelJoin(p1, p2, offset);
                    break;
                default:
                    addMiterJoin(p1, p2, offset);
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

        lineData.append(lineDataSegment);
    }

    return lineData;
}

QPointF RiveQtOpenGLPath::cubicBezier(const QPointF &startPoint, const QPointF &controlPoint1, const QPointF &controlPoint2,
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

QVector<QVector2D> RiveQtOpenGLPath::qpainterPathToVector2D(const QPainterPath &path)
{
    QVector<QVector2D> pathData;

    if (path.isEmpty()) {
        //        qCDebug(rqqpRendering) << "Path is empty, cannot calculate vertices";
        return pathData;
    }

    QTriangleSet triangles = qTriangulate(path);

    for (int i = 0; i < triangles.indices.size(); i++) {
        int index;
        if (triangles.indices.type() == QVertexIndexVector::UnsignedInt) {
            index = static_cast<const quint32 *>(triangles.indices.data())[i];
        } else {
            index = static_cast<const quint16 *>(triangles.indices.data())[i];
        }

        qreal x = triangles.vertices[2 * index];
        qreal y = triangles.vertices[2 * index + 1];

        pathData.append(QVector2D(x, y));
    }

    return pathData;
}

QVector<QVector2D> RiveQtOpenGLPath::qpainterPathToOutlineVector2D(const QPainterPath &path)
{
    QVector<QVector2D> pathData;

    if (path.isEmpty()) {
        //        qCDebug(rqqpRendering) << "Path is empty, cannot calulate outline vector";
        return pathData;
    }

    QPointF point = path.elementAt(0);
    QVector2D centerPoint = QVector2D(point.x(), point.y());

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
            QPointF startPoint = pathData.last().toPointF();
            QPointF controlPoint1 = element;
            QPointF controlPoint2 = path.elementAt(i + 1);
            QPointF endPoint = path.elementAt(i + 2);

            // Calculate the number of segments you want for the curve.
            // You can change this value depending on the desired level of detail.
            const int segments = 20;

            for (int j = 1; j <= segments; ++j) {
                qreal t = static_cast<qreal>(j) / segments;
                QPointF point = cubicBezier(startPoint, controlPoint1, controlPoint2, endPoint, t);
                pathData.append(QVector2D(point.x(), point.y()));
            }

            i += 2; // Skip the next two control points, as we already processed them.
            break;
        }
        default:
            break;
        }
    }

    if (m_isClosed) {
        // Connect the last point to the first point.
        pathData.append(QVector2D(point.x(), point.y()));
    }

    return pathData;
}

void RiveQtOpenGLPath::generateVertices()
{
    if (m_path.isEmpty() && m_subPaths.empty()) {
        qCDebug(rqqpRendering) << "Current path and sub paths are empty, cannot generate vertices.";
        return;
    }

    m_pathSegmentsData.clear();
    m_pathSegmentsOutlineData.clear();

    m_pathSegmentsData.append(qpainterPathToVector2D(m_path));
    m_pathSegmentsOutlineData.append(qpainterPathToOutlineVector2D(m_path));

    for (SubPath &subPath : m_subPaths) {
        QPainterPath sourcePath = subPath.path()->toQPainterPath();
        QPainterPath transformedPath = RiveQtUtils::transformPathWithMatrix4x4(sourcePath, subPath.transform());

        m_pathSegmentsData.append(qpainterPathToVector2D(transformedPath));
        m_pathSegmentsOutlineData.append(qpainterPathToOutlineVector2D(transformedPath));
    }
}

RiveQtOpenGLRenderer::RiveQtOpenGLRenderer()
    : rive::Renderer()
{
    initializeOpenGLFunctions();
    m_Stack.emplace_back(RenderState());
}

void RiveQtOpenGLRenderer::initGL()
{
    // static fullscreen TRIANGLE_FAN

    m_normalizedQuadVertices = { // Positions  // Texture Coords
                                 -1.0f, -1.0f, 0.0f, 0.0f, //
                                 -1.0f, 1.0f,  0.0f, 1.0f, //
                                 1.0f,  1.0f,  1.0f, 1.0f, //
                                 1.0f,  -1.0f, 1.0f, 0.0f
    };

    // ---- Initialize Path Draw Shader Programm
    //
    m_pathShaderProgram = new QOpenGLShaderProgram();
    m_pathShaderProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/path_vertex_shader.glsl");
    m_pathShaderProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/path_fragement_shader.glsl");
    m_pathShaderProgram->link();

    // ---- All releated to blending shader program
    //
    m_blendShaderProgram = new QOpenGLShaderProgram();
    m_blendShaderProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/blending_vertex_shader.glsl");
    m_blendShaderProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/blending_fragment_shader.glsl");
    m_blendShaderProgram->link();

    m_fullscreenQuadVBO = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);

    m_fullscreenQuadVAO.create();
    m_fullscreenQuadVAO.bind();
    {
        m_fullscreenQuadVBO.create();
        m_fullscreenQuadVBO.bind();
        m_fullscreenQuadVBO.allocate(m_normalizedQuadVertices.constData(), m_normalizedQuadVertices.size() * sizeof(GLfloat));

        int posAttr = m_blendShaderProgram->attributeLocation("a_position");
        m_blendShaderProgram->enableAttributeArray(posAttr);
        m_blendShaderProgram->setAttributeBuffer(posAttr, GL_FLOAT, 0, 2, 4 * sizeof(GLfloat));

        int texCoordAttr = m_blendShaderProgram->attributeLocation("a_texCoord");
        m_blendShaderProgram->enableAttributeArray(texCoordAttr);
        m_blendShaderProgram->setAttributeBuffer(texCoordAttr, GL_FLOAT, 2 * sizeof(GLfloat), 2, 4 * sizeof(GLfloat));
    }
    m_fullscreenQuadVAO.release();

    // ---- All releated to Image shader program
    //
    m_imageShaderProgram = new QOpenGLShaderProgram();
    m_imageShaderProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/image_vertex_shader.glsl");
    m_imageShaderProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/image_fragment_shader.glsl");
    m_imageShaderProgram->link();

    m_imageQuadVAO.create();
    m_imageQuadVAO.bind();
    {
        m_imageQuadVBO.create();
        m_imageQuadVBO.bind();
        m_imageQuadVBO.allocate(m_normalizedQuadVertices.constData(), m_normalizedQuadVertices.size() * sizeof(GLfloat));

        int imagePosAttr = m_imageShaderProgram->attributeLocation("a_position");
        m_imageShaderProgram->enableAttributeArray(imagePosAttr);
        m_imageShaderProgram->setAttributeBuffer(imagePosAttr, GL_FLOAT, 0, 2, 4 * sizeof(GLfloat));

        int imageTexCoordAttr = m_imageShaderProgram->attributeLocation("a_texCoord");
        m_imageShaderProgram->enableAttributeArray(imageTexCoordAttr);
        m_imageShaderProgram->setAttributeBuffer(imageTexCoordAttr, GL_FLOAT, 2 * sizeof(GLfloat), 2, 4 * sizeof(GLfloat));
    }
    m_imageQuadVAO.release();

    // ---- All releated to Image Mesh shader program
    //
    m_imageMeshShaderProgram = new QOpenGLShaderProgram();
    m_imageMeshShaderProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/imageMesh_vertex_shader.glsl");
    m_imageMeshShaderProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/imageMesh_fragment_shader.glsl");
    m_imageMeshShaderProgram->link();

    m_meshVAO.create();
    m_meshVAO.bind();
    {
        // Vertex buffer for positions
        m_meshVertexBuffer = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
        m_meshVertexBuffer.create();
        m_meshVertexBuffer.bind();
        int meshPosAttr = m_imageMeshShaderProgram->attributeLocation("a_position");
        m_imageMeshShaderProgram->enableAttributeArray(meshPosAttr);
        m_imageMeshShaderProgram->setAttributeBuffer(meshPosAttr, GL_FLOAT, 0, 2, 0);

        // Vertex buffer for texture coordinates
        m_meshUvBuffer.create();
        m_meshUvBuffer.bind();
        int meshTexCoordAttr = m_imageMeshShaderProgram->attributeLocation("a_texCoord");
        m_imageMeshShaderProgram->enableAttributeArray(meshTexCoordAttr);
        m_imageMeshShaderProgram->setAttributeBuffer(meshTexCoordAttr, GL_FLOAT, 0, 2, 0);

        // Index buffer
        m_meshIndexBuffer = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
        m_meshIndexBuffer.create();
        m_meshIndexBuffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
        m_meshIndexBuffer.bind();
    }

    m_meshVAO.release();
}

void RiveQtOpenGLRenderer::reset()
{
    if (m_displayFramebuffer) {
        m_displayFramebuffer->bind();
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Clear color with alpha set to zero
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        m_displayFramebuffer->release();
    }
    if (m_alphaMaskFramebuffer) {
        m_alphaMaskFramebuffer->bind();
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Clear color with alpha set to zero
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        m_alphaMaskFramebuffer->release();
    }
    if (m_blendFramebuffer) {
        m_blendFramebuffer->bind();
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Clear color with alpha set to zero
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        m_blendFramebuffer->release();
    }
}

void RiveQtOpenGLRenderer::save()
{
    m_Stack.push_back(m_Stack.back());
}

void RiveQtOpenGLRenderer::restore()
{
    assert(m_Stack.size() > 1);
    m_Stack.pop_back();
}

void RiveQtOpenGLRenderer::transform(const rive::Mat2D &transform)
{
    QMatrix4x4 &stackMat = m_Stack.back().transform;
    QMatrix4x4 matrix(transform[0], transform[2], 0, transform[4], transform[1], transform[3], 0, transform[5], 0, 0, 1, 0, 0, 0, 0, 1);

    stackMat = stackMat * matrix;
}

void RiveQtOpenGLRenderer::setupBlendMode(rive::BlendMode blendMode)
{
    switch (blendMode) {
    case rive::BlendMode::colorDodge:
    case rive::BlendMode::overlay:
    case rive::BlendMode::colorBurn:
    case rive::BlendMode::softLight:
    case rive::BlendMode::hardLight:
    case rive::BlendMode::exclusion:
    case rive::BlendMode::hue:
    case rive::BlendMode::saturation:
    case rive::BlendMode::color:
    case rive::BlendMode::luminosity:
    case rive::BlendMode::screen:
        m_useBlendingTexture = true; // intentional fallthrough
    case rive::BlendMode::srcOver:
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        break;
    case rive::BlendMode::darken:
        glBlendFunc(GL_ONE, GL_ONE);
        glBlendEquation(GL_MIN);
        break;
    case rive::BlendMode::lighten:
        glBlendFunc(GL_ONE, GL_ONE);
        glBlendEquation(GL_MAX);
        break;
    case rive::BlendMode::difference:
        glBlendFunc(GL_ONE, GL_ONE);
        glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
        break;
    case rive::BlendMode::multiply:
        glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
        glBlendFuncSeparate(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        break;
    default:
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        break;
    }
}

const QMatrix4x4 &RiveQtOpenGLRenderer::modelMatrix()
{
    return m_Stack.front().transform;
}

void RiveQtOpenGLRenderer::composeFinalImage(rive::BlendMode blendMode)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (m_useBlendingTexture) {
        m_blendFramebuffer->release();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_blendFramebuffer->texture());

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_displayFramebuffer->texture());

        m_displayFramebuffer->bind();

        // Draw the fullscreen quad
        m_fullscreenQuadVAO.bind();
        m_blendShaderProgram->bind();

        m_fullscreenQuadVBO.bind();
        m_fullscreenQuadVBO.allocate(m_normalizedQuadVertices.constData(), m_normalizedQuadVertices.size() * sizeof(GLfloat));
        m_blendShaderProgram->setUniformValue("u_blendTexture", 0);
        m_blendShaderProgram->setUniformValue("u_displayTexture", 1);
        m_blendShaderProgram->setUniformValue("u_blendMode", static_cast<int>(blendMode));
        m_blendShaderProgram->setUniformValue("u_projection", m_projectionMatrix);
        m_blendShaderProgram->setUniformValue("u_applyTranform", false);

        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        // Clean up

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);
        m_blendShaderProgram->release();
        m_fullscreenQuadVAO.release();

        m_useBlendingTexture = false;
    }

    m_displayFramebuffer->release();

    // Bind the default framebuffer
    QOpenGLFramebufferObject::bindDefault();
    // Set the blend texture

    // Draw the fullscreen quad
    m_fullscreenQuadVAO.bind();
    m_blendShaderProgram->bind();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_displayFramebuffer->texture());

    m_fullscreenQuadVBO.bind();
    m_fullscreenQuadVBO.allocate(m_viewPortQuadVertices.constData(), sizeof(GLfloat) * m_viewPortQuadVertices.size());

    m_blendShaderProgram->setUniformValue("u_displayTexture", 0);
    m_blendShaderProgram->setUniformValue("u_blendMode", 0);
    m_blendShaderProgram->setUniformValue("u_projection", m_projectionMatrix);
    m_blendShaderProgram->setUniformValue("u_applyTranform", true);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    // Clean up
    m_blendShaderProgram->release();
    m_fullscreenQuadVAO.release();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void RiveQtOpenGLRenderer::configureGradientShader(const QGradient *gradient)
{
    QGradientStops gradientStops = gradient->stops();

    QVector<QVector4D> gradientColors;
    QVector<QVector2D> gradientPositions;

    for (const auto &stop : gradientStops) {
        QColor color = stop.second;
        gradientColors.append(QVector4D(color.redF(), color.greenF(), color.blueF(), color.alphaF()));
        gradientPositions.append(QVector2D(stop.first, 0.0f));
    }

    m_pathShaderProgram->setUniformValueArray("u_stopColors", gradientColors.constData(), gradientColors.size());
    m_pathShaderProgram->setUniformValueArray("u_stopPositions", gradientPositions.constData(), gradientPositions.size());
    m_pathShaderProgram->setUniformValue("u_numStops", static_cast<GLint>(gradientStops.size()));

    // Check the gradient type and set the appropriate shader uniforms
    if (gradient->type() == QGradient::LinearGradient) {
        const QLinearGradient *linearGradient = static_cast<const QLinearGradient *>(gradient);
        QPointF startPoint = linearGradient->start();
        QPointF endPoint = linearGradient->finalStop();

        m_pathShaderProgram->setUniformValue("u_gradientStart", startPoint);
        m_pathShaderProgram->setUniformValue("u_gradientEnd", endPoint);

        m_pathShaderProgram->setUniformValue("u_useGradient", true);
        m_pathShaderProgram->setUniformValue("u_gradientType", 0); // 0 for linear gradient

    } else if (gradient->type() == QGradient::RadialGradient) {
        const QRadialGradient *radialGradient = static_cast<const QRadialGradient *>(gradient);
        QPointF center = radialGradient->center();
        QPointF focalPoint = radialGradient->focalPoint();
        qreal radius = radialGradient->radius();

        m_pathShaderProgram->setUniformValue("u_gradientCenter", QVector2D(center));
        m_pathShaderProgram->setUniformValue("u_gradientFocalPoint", QVector2D(focalPoint));
        m_pathShaderProgram->setUniformValue("u_gradientRadius", static_cast<float>(radius));

        m_pathShaderProgram->setUniformValue("u_useGradient", true);
        m_pathShaderProgram->setUniformValue("u_gradientType", 1); // 1 for radial gradient
    }
}

void RiveQtOpenGLRenderer::drawPath(rive::RenderPath *path, rive::RenderPaint *paint)
{
    if (!path) {
        qCDebug(rqqpRendering) << "Cannot draw path that is nullptr.";
        return;
    }
    if (!paint) {
        qCDebug(rqqpRendering) << "Cannot draw path, paint is empty.";
        return;
    }

    RiveQtOpenGLPath *qtPath = static_cast<RiveQtOpenGLPath *>(path);
    RiveQtPaint *qtPaint = static_cast<RiveQtPaint *>(paint);

    const auto &blendMode = qtPaint->blendMode();
    // Set the appropriate OpenGL blend functions
    setupBlendMode(qtPaint->blendMode());

    glEnable(GL_BLEND);
    {
        QVector<QVector<QVector2D>> pathData;

        if (qtPaint->paintStyle() == rive::RenderPaintStyle::fill) {
            pathData = qtPath->toVertices();
        }

        if (qtPaint->paintStyle() == rive::RenderPaintStyle::stroke) {
            pathData = qtPath->toVerticesLine(qtPaint->pen());
        }

        // Check if the alphaMaskFramebuffer is valid
        if (m_alphaMaskFramebuffer && m_alphaMaskFramebuffer->isValid()) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_alphaMaskFramebuffer->texture());
        } else {
            qWarning() << "AlphaMaskFramebuffer is not valid.";
        }

        if (m_useBlendingTexture) {
            if (m_blendFramebuffer && m_blendFramebuffer->isValid()) {
                m_blendFramebuffer->bind();

                glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Clear color with alpha set to zero
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

            } else {
                qWarning() << "BlendFramebuffer is not valid. disable blending texture use.";
            }

        } else {
            if (m_displayFramebuffer && m_displayFramebuffer->isValid()) {
                m_displayFramebuffer->bind();

            } else {
                qWarning() << "m_displayFramebuffer is not valid. disable its use.";
            }
        }

        QOpenGLVertexArrayObject vao;
        vao.create();
        vao.bind();

        QOpenGLBuffer vbo(QOpenGLBuffer::VertexBuffer);
        vbo.create();
        vbo.bind();
        vbo.setUsagePattern(QOpenGLBuffer::DynamicDraw);

        int maxVerticesSize = 0;
        for (const auto &segment : qAsConst(pathData)) {
            maxVerticesSize = qMax(maxVerticesSize, segment.size());
        }
        vbo.allocate(maxVerticesSize * sizeof(QVector2D));

        m_pathShaderProgram->bind();

        int posAttr = m_pathShaderProgram->attributeLocation("a_position");
        m_pathShaderProgram->enableAttributeArray(posAttr);

        for (const auto &segment : qAsConst(pathData)) {
            QVector2D *vboData = static_cast<QVector2D *>(vbo.map(QOpenGLBuffer::WriteOnly));
            std::copy(segment.constBegin(), segment.constEnd(), vboData);
            vbo.unmap();

            m_pathShaderProgram->setUniformValue("u_useAlphaMask", m_IsClippingDirty);
            m_pathShaderProgram->setUniformValue("u_alphaMaskTexture", 0);

            QColor color = qtPaint->color();

            // Gradient code starts here
            if (!color.isValid()) {

                const auto *gradient = qtPaint->brush().gradient();
                if (gradient) {
                    configureGradientShader(gradient);
                } else {
                    m_pathShaderProgram->setUniformValue("u_useGradient", false);
                }
                m_pathShaderProgram->setUniformValue("u_color", QColor(255, 0, 0, 255));
            } else {
                m_pathShaderProgram->setUniformValue("u_useGradient", false);
                m_pathShaderProgram->setUniformValue("u_color", color);
            }
            // Gradient code ends here

            m_pathShaderProgram->setUniformValue("u_viewportDimensions", QVector2D(m_viewportWidth, m_viewportHeight));
            m_pathShaderProgram->setUniformValue("u_projection", m_projectionMatrix);
            m_pathShaderProgram->setUniformValue("u_transform", transform());
            m_pathShaderProgram->setAttributeBuffer(posAttr, GL_FLOAT, 0, 2, 0);

            if (qtPaint->paintStyle() == rive::RenderPaintStyle::stroke) {
                glDrawArrays(GL_TRIANGLE_STRIP, 0, segment.count());
            } else {
                glDrawArrays(GL_TRIANGLES, 0, segment.count());
            }
        }

        m_pathShaderProgram->disableAttributeArray(posAttr);
        m_pathShaderProgram->release();
        vao.release();
        vbo.release();

        // this function exists to make the code easier to read
        // it's aware and relays on the states set here, like the active buffer, blending ect.
        composeFinalImage(blendMode);
    }
    glDisable(GL_BLEND);

    if (blendMode == rive::BlendMode::darken || blendMode == rive::BlendMode::lighten) {
        glBlendEquation(GL_FUNC_ADD);
    }

    m_IsClippingDirty = false;
}

void RiveQtOpenGLRenderer::clipPath(rive::RenderPath *path)
{
    m_IsClippingDirty = true;
    m_alphaMaskFramebuffer->bind();

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Clear color with alpha set to zero
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    RiveQtOpenGLPath *qtPath = static_cast<RiveQtOpenGLPath *>(path);

    QColor outlineColor(255, 0, 0, 255); // Red color for the outlines

    QVector<QVector<QVector2D>> pathData;

    pathData = qtPath->toVertices();

    for (const auto &segment : qAsConst(pathData)) {
        QOpenGLVertexArrayObject vao;
        vao.create();
        vao.bind();

        QOpenGLBuffer vbo(QOpenGLBuffer::VertexBuffer);
        vbo.create();
        vbo.bind();
        vbo.allocate(segment.constData(), segment.count() * sizeof(QVector2D));

        // Enable the shader program
        m_pathShaderProgram->bind();

        m_pathShaderProgram->setUniformValue("u_useGradient", false);
        m_pathShaderProgram->setUniformValue("u_useAlphaMask", false);
        m_pathShaderProgram->setUniformValue("u_color", outlineColor);

        // Set the viewport dimensions
        m_pathShaderProgram->setUniformValue("u_viewportDimensions", QVector2D(m_viewportWidth, m_viewportHeight));
        m_pathShaderProgram->setUniformValue("u_projection", m_projectionMatrix);
        m_pathShaderProgram->setUniformValue("u_transform", transform());

        // Set up the vertex attribute pointer for the position
        int posAttr = m_pathShaderProgram->attributeLocation("a_position");
        m_pathShaderProgram->enableAttributeArray(posAttr);
        m_pathShaderProgram->setAttributeBuffer(posAttr, GL_FLOAT, 0, 2, 0);

        glDrawArrays(GL_TRIANGLES, 0, vbo.size() / sizeof(QVector2D));

        // Clean up
        m_pathShaderProgram->disableAttributeArray(posAttr);
        m_pathShaderProgram->release();
        vao.release();
        vbo.release();
    }

    m_alphaMaskFramebuffer->release();
}

void RiveQtOpenGLRenderer::drawImage(const rive::RenderImage *image, rive::BlendMode blendMode, float opacity)
{
    return;
    if (!image) {
        return;
    }

    // Set the appropriate OpenGL blend functions
    setupBlendMode(blendMode);

    if (m_useBlendingTexture) {
        qWarning() << "Blending Modes that require shaders are not yet supported for Qt OpenGL Rive Image Rendering.";
        m_useBlendingTexture = false;
    }

    glEnable(GL_BLEND);

    const auto *riveImage = static_cast<const RiveQtImage *>(image);
    const QImage &qImage = riveImage->image();
    QOpenGLTexture texture(qImage);

    // Bind the texture
    texture.bind();

    m_imageQuadVAO.bind();

    float halfWidth = qImage.width() / 2.0f;
    float halfHeight = qImage.height() / 2.0f;

    // todo -> move to RiveQtImage to do this not every frame...
    GLfloat imageData[] = {
        // Positions      // Texture coordinates
        -halfWidth, -halfHeight, 0.0f, 1.0f, // Bottom-left
        halfWidth,  -halfHeight, 1.0f, 1.0f, // Bottom-right
        halfWidth,  halfHeight,  1.0f, 0.0f, // Top-right
        -halfWidth, halfHeight,  0.0f, 0.0f // Top-left
    };

    m_imageQuadVBO.bind();
    m_imageQuadVBO.allocate(imageData, sizeof(imageData));

    m_imageShaderProgram->bind();

    m_imageShaderProgram->setUniformValue("u_texture", 0);
    m_imageShaderProgram->setUniformValue("u_projection", m_projectionMatrix);
    m_imageShaderProgram->setUniformValue("u_transform", transform());
    m_imageShaderProgram->setUniformValue("u_model", modelMatrix());
    m_imageShaderProgram->setUniformValue("u_opacity", opacity);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    m_imageShaderProgram->release();

    m_imageQuadVAO.release();

    glDisable(GL_BLEND);
}

void RiveQtOpenGLRenderer::drawImageMesh(const rive::RenderImage *image, rive::rcp<rive::RenderBuffer> vertices_f32,
                                         rive::rcp<rive::RenderBuffer> uvCoords_f32, rive::rcp<rive::RenderBuffer> indices_u16,
                                         rive::BlendMode blendMode, float opacity)
{
    const QImage &qImage = static_cast<const RiveQtImage *>(image)->image();
    QOpenGLTexture texture(qImage);

    // Bind the texture
    texture.bind();

    RiveQtBufferF32 *qtVertices = static_cast<RiveQtBufferF32 *>(vertices_f32.get());
    RiveQtBufferF32 *qtUvCoords = static_cast<RiveQtBufferF32 *>(uvCoords_f32.get());
    RiveQtBufferU16 *qtIndices = static_cast<RiveQtBufferU16 *>(indices_u16.get());

    // Set the appropriate OpenGL blend functions
    setupBlendMode(blendMode);

    if (m_useBlendingTexture) {
        qWarning() << "Blending Modes that require shaders are not yet supported for Qt OpenGL Rive Image Mesh Rendering.";
        m_useBlendingTexture = false;
    }

    glEnable(GL_BLEND);

    m_meshVAO.bind();
    m_meshVertexBuffer.bind();
    m_meshVertexBuffer.allocate(qtVertices->data(), qtVertices->size());
    m_meshUvBuffer.bind();
    m_meshUvBuffer.allocate(qtUvCoords->data(), qtUvCoords->size());

    m_imageMeshShaderProgram->bind();

    m_imageMeshShaderProgram->setUniformValue("u_texture", 0);
    m_imageMeshShaderProgram->setUniformValue("u_projection", m_projectionMatrix);
    m_imageMeshShaderProgram->setUniformValue("u_transform", transform());
    m_imageMeshShaderProgram->setUniformValue("u_model", modelMatrix());
    m_imageMeshShaderProgram->setUniformValue("u_opacity", opacity);

    m_meshIndexBuffer.bind();
    m_meshIndexBuffer.allocate(qtIndices->data(), qtIndices->size());
    // Issue the draw call with the indexed geometry
    glDrawElements(GL_TRIANGLES, qtIndices->count(), GL_UNSIGNED_SHORT, nullptr);

    // Release the shader program and the VAO
    m_imageMeshShaderProgram->release();
    m_meshVAO.release();

    glDisable(GL_BLEND);
}

void RiveQtOpenGLRenderer::updateModelMatrix(const QMatrix4x4 &modelMatrix)
{
    m_Stack.front().transform = modelMatrix;
}

void RiveQtOpenGLRenderer::updateProjectionMatrix(const QMatrix4x4 &projMatrix)
{
    m_projectionMatrix = projMatrix;
}

void RiveQtOpenGLRenderer::updateViewportSize()
{
    // for clipping we have to get the viewport dimensions
    // the clip texture is scaled on the entire viewport
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    int viewportWidth = viewport[2];
    int viewportHeight = viewport[3];

    if (viewportWidth == m_viewportWidth && viewportHeight == m_viewportHeight) {
        return;
    }

    m_viewportWidth = viewportWidth;
    m_viewportHeight = viewportHeight;

    QOpenGLFramebufferObject *newDisplay = new QOpenGLFramebufferObject(m_viewportWidth, m_viewportHeight);
    delete m_displayFramebuffer;
    m_displayFramebuffer = newDisplay;

    QOpenGLFramebufferObject *newBlend = new QOpenGLFramebufferObject(m_viewportWidth, m_viewportHeight);
    delete m_blendFramebuffer;
    m_blendFramebuffer = newBlend;

    // this is a fullscreen texture! so we push the entire screen size in.
    QOpenGLFramebufferObject *newAlpha = new QOpenGLFramebufferObject(m_viewportWidth, m_viewportHeight);
    delete m_alphaMaskFramebuffer;
    m_alphaMaskFramebuffer = newAlpha;

    m_viewPortQuadVertices = {
        // Positions          // Texture coordinates
        0.0f,
        0.0f,
        0.0f,
        1.0f, // Bottom-left
        static_cast<float>(m_viewportWidth),
        0.0f,
        1.0f,
        1.0f, // Bottom-right
        static_cast<float>(m_viewportWidth),
        static_cast<float>(m_viewportHeight),
        1.0f,
        0.0f, // Top-right
        0.0f,
        static_cast<float>(m_viewportHeight),
        0.0f,
        0.0f // Top-left
    };
}

const QMatrix4x4 &RiveQtOpenGLRenderer::transform()
{
    return m_Stack.back().transform;
}

SubPath::SubPath(RiveQtOpenGLPath *path, const QMatrix4x4 &transform)
    : m_Path(path)
    , m_Transform(transform)
{
}

RiveQtOpenGLPath *SubPath::path() const
{
    return m_Path;
}
QMatrix4x4 SubPath::transform() const
{
    return m_Transform;
}
