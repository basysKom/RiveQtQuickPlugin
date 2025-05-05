// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 Jonas Kalinka <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "riveqtpath.h"
#include "rqqplogging.h"
#include "riveqtutils.h"

#include <QVector2D>
#include <QMatrix2x2>
#include <QtMath>
#include <private/qtriangulator_p.h>

#if !defined(USE_QPAINTERPATH_STROKER)
#include <optional>
#endif

RiveQtPath::RiveQtPath()
    : rive::RenderPath()
{
    m_path.setFillRule(Qt::FillRule::WindingFill);
}

RiveQtPath::RiveQtPath(const RiveQtPath &other)
    : rive::RenderPath()
#if !defined(USE_QPAINTERPATH_STROKER)
    , m_pathSegmentsOutlineData(other.m_pathSegmentsOutlineData)
    , m_segmentCount(other.m_segmentCount)
#endif
    , m_path(other.m_path)
    , m_pathVertices(other.m_pathVertices)
    , m_pathOutlineVertices(other.m_pathOutlineVertices)
    , m_renderQuality(other.m_renderQuality)
{
}

RiveQtPath::RiveQtPath(const rive::RawPath &rawPath, rive::FillRule fillRule, RiveRenderSettings::RenderQuality renderQuality)
    : rive::RenderPath()
    , m_renderQuality(renderQuality)
{
    m_path.clear();
    m_path.setFillRule(RiveQtUtils::convert(fillRule));

    addRawPathImpl(rawPath);
}

void RiveQtPath::rewind()
{
    m_pathVertices.clear();

#if !defined(USE_QPAINTERPATH_STROKER)
    m_pathSegmentsOutlineData.clear();
#endif

    m_path.clear();
    m_pathSegmentOutlineDataDirty = true;
    m_pathSegmentDataDirty = true;
}

void RiveQtPath::moveTo(float x, float y)
{
    m_path.moveTo(x, y);
}

void RiveQtPath::lineTo(float x, float y)
{
    m_path.lineTo(x, y);
}

void RiveQtPath::cubicTo(float ox, float oy, float ix, float iy, float x, float y)
{
    m_path.cubicTo(ox, oy, ix, iy, x, y);
}

void RiveQtPath::close()
{
    m_path.closeSubpath();
}

void RiveQtPath::fillRule(rive::FillRule value)
{
    switch (value) {
    case rive::FillRule::evenOdd:
        m_path.setFillRule(Qt::FillRule::OddEvenFill);
        break;
    case rive::FillRule::nonZero:
    default:
        m_path.setFillRule(Qt::FillRule::WindingFill);
        break;
    }
}

void RiveQtPath::addRenderPath(rive::RenderPath *path, const rive::Mat2D &transform)
{
    if (!path) {
        qCDebug(rqqpRendering) << "Skip adding nullptr render path.";
        return;
    }

    RiveQtPath *qtPath = static_cast<RiveQtPath *>(path);

    QTransform qTransform(transform[0], transform[1], transform[2], transform[3], transform[4], transform[5]);

    QPainterPath qPath = qtPath->toQPainterPath() * qTransform;
    m_path.addPath(qPath);

    m_pathSegmentOutlineDataDirty = true;
    m_pathSegmentDataDirty = true;
}

void RiveQtPath::addRawPath(const rive::RawPath &path)
{
    addRawPathImpl(path);

    m_pathSegmentOutlineDataDirty = true;
    m_pathSegmentDataDirty = true;
}

void RiveQtPath::applyMatrix(const QMatrix4x4 &matrix)
{
    m_path = m_path * matrix.toTransform();
}

QVector<QVector<QVector2D>> RiveQtPath::toVertices()
{
    if (m_pathSegmentDataDirty) {
        updatePathSegmentsData();
    }
    return m_pathVertices;
}

void RiveQtPath::setQPainterPath(const QPainterPath &path)
{
    m_path = path;
}

bool RiveQtPath::intersectWith(const QPainterPath &other)
{
    if (!m_path.isEmpty() && !other.isEmpty()) {
        if (other.intersects(m_path)) {
            QPainterPath tempQPainterPath = other.intersected(m_path);
            if (!tempQPainterPath.isEmpty()) {
                m_path = tempQPainterPath;
                m_pathSegmentDataDirty = true;
                m_pathSegmentOutlineDataDirty = true;
                return true;
            }
        }
    }
    m_path = other;
    return false;
}

QPainterPath RiveQtPath::toQPainterPath() const
{
    return m_path;
}

QVector<QVector<QVector2D>> RiveQtPath::toVerticesLine(const QPen &pen)
{
    if (!m_pathSegmentOutlineDataDirty) {
        return m_pathOutlineVertices;
    }

#if !defined(USE_QPAINTERPATH_STROKER)
    updatePathSegmentsOutlineData();
    m_pathOutlineVertices.clear();
    // Early exit, nothing to do.
    if (m_pathSegmentsOutlineData.isEmpty()) {
        return m_pathOutlineVertices;
    }
#endif

    updatePathOutlineVertices(pen);
    m_pathSegmentOutlineDataDirty = false;

    return m_pathOutlineVertices;
}

#if !defined(USE_QPAINTERPATH_STROKER)
std::optional<QVector2D> calculateIntersection(const QVector2D &p1, const QVector2D &p2, const QVector2D &p3, const QVector2D &p4)
{
    const float x1 = p1.x(), y1 = p1.y();
    const float x2 = p2.x(), y2 = p2.y();
    const float x3 = p3.x(), y3 = p3.y();
    const float x4 = p4.x(), y4 = p4.y();

    float denominator = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);

    // If the lines are parallel or coincident, return an invalid point
    if (abs(denominator) < 0.01f) {
        return {};
    }

    const float factor1 = (x1 * y2 - y1 * x2);
    const float factor2 = (x3 * y4 - y3 * x4);

    const float intersectX = (factor1 * (x3 - x4) - (x1 - x2) * factor2) / denominator;
    const float intersectY = (factor1 * (y3 - y4) - (y1 - y2) * factor2) / denominator;
    return QVector2D(intersectX, intersectY);
}

QPointF RiveQtPath::cubicBezier(const QPointF &startPoint, const QPointF &controlPoint1, const QPointF &controlPoint2,
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

QVector2D cubicBezierTangent(const QPointF &p0, const QPointF &p1, const QPointF &p2, const QPointF &p3, const float t)
{
    const auto r = 3.f * (1.f - t) * (1.f - t) * (p1 - p0) + 6.f * (1.f - t) * t * (p2 - p1) + 3.f * t * t * (p3 - p2);
    return QVector2D(r.x(), r.y()).normalized();
}

void RiveQtPath::updatePathSegmentsOutlineData()
{
    m_pathSegmentsOutlineData.clear();

    if (m_path.isEmpty()) {
        m_pathSegmentOutlineDataDirty = false;
        return;
    }

    QVector<PathDataPoint> pathDataEnhanced;
    pathDataEnhanced.reserve(m_path.elementCount());

    const QPointF &point = m_path.elementAt(0);
    const QVector2D &centerPoint = QVector2D(point.x(), point.y());
    int currentStepIndex { 0 };

    // Add the current point
    pathDataEnhanced.append({ centerPoint, QVector2D(), currentStepIndex });

    for (int i = 1; i < m_path.elementCount(); ++i) {
        QPainterPath::Element element = m_path.elementAt(i);

        switch (element.type) {
        case QPainterPath::MoveToElement:
            if (pathDataEnhanced.size() > 1)
                m_pathSegmentsOutlineData.append(pathDataEnhanced);
            pathDataEnhanced.clear();
            currentStepIndex = 0;
            pathDataEnhanced.append({ QVector2D(element.x, element.y), QVector2D(), currentStepIndex });
            ++currentStepIndex;
            break;

        case QPainterPath::LineToElement:
            pathDataEnhanced.append({ QVector2D(element.x, element.y), QVector2D(), currentStepIndex });
            ++currentStepIndex;
            break;

        case QPainterPath::CurveToElement: {
            const QPointF &startPoint = pathDataEnhanced.last().point.toPointF();
            const QPointF &controlPoint1 = element;
            const QPointF &controlPoint2 = m_path.elementAt(i + 1);
            const QPointF &endPoint = m_path.elementAt(i + 2);

            pathDataEnhanced.last().tangent = cubicBezierTangent(startPoint, controlPoint1, controlPoint2, endPoint, 0.f);

            for (int j = 1; j <= m_segmentCount; ++j) {
                const qreal t = static_cast<qreal>(j) / m_segmentCount;
                const QPointF &point = cubicBezier(startPoint, controlPoint1, controlPoint2, endPoint, t);
                //                pathData.append(QVector2D(point.x(), point.y()));
                pathDataEnhanced.append({ QVector2D(point.x(), point.y()),
                                          cubicBezierTangent(startPoint, controlPoint1, controlPoint2, endPoint, t), currentStepIndex });
            }

            i += 2; // Skip the next two control points, as we already processed them.
            ++currentStepIndex;
            break;
        }
        default:
            break;
        }
    }

    m_pathSegmentsOutlineData.append(pathDataEnhanced);
    m_pathSegmentOutlineDataDirty = false;
}
#endif

void RiveQtPath::updatePathOutlineVertices(const QPen &pen)
{

#if defined(USE_QPAINTERPATH_STROKER)
    m_pathOutlineVertices.clear();
    QPainterPathStroker painterPathStroker(pen);
    painterPathStroker.setCurveThreshold(0.1);
    QPainterPath strokedPath = painterPathStroker.createStroke(m_path);
    QTriangleSet triangles = qTriangulate(strokedPath, QTransform(), static_cast<qreal>(m_renderQuality));

    QVector<QVector2D> pathData;
    pathData.reserve(triangles.indices.size());
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

    m_pathOutlineVertices.append(pathData);
#else

    const qreal lineWidth = pen.widthF();
    const Qt::PenJoinStyle &joinType = pen.joinStyle();
    const Qt::PenCapStyle &capStyle = pen.capStyle();

    for (const auto &pathData : qAsConst(m_pathSegmentsOutlineData)) {
        if (pathData.size() <= 1) {
            continue;
        }

        QVector<QVector2D> lineDataSegment;

        bool closed = false;

        if (pathData.first().point == pathData.last().point) {
            closed = true;
        }

        const int endIndex = closed ? pathData.count() : pathData.count() - 1;

        for (int i = 0; i < endIndex; ++i) {
            int nextI = (i + 1) % (pathData.count());
            const QVector2D &p1 = pathData[i].point;
            const QVector2D &p2 = pathData[nextI].point; // if endIndex, take 0
            const auto &t1 = pathData[i].tangent;
            const auto &t2 = pathData[nextI].tangent;

            QVector2D normal, normal2;
            const QVector2D diff = p2 - p1;

            if (pathData.at(i).tangent.isNull()) {
                normal = QVector2D(-diff.y(), diff.x()).normalized();
            } else {
                normal = QVector2D(-pathData.at(i).tangent.y(), pathData.at(i).tangent.x()).normalized();
            }
            if (pathData.at(nextI).tangent.isNull()) {
                normal2 = normal;
            } else {
                normal2 = QVector2D(-pathData.at(nextI).tangent.y(), pathData.at(nextI).tangent.x()).normalized();
            }
            const QVector2D offset = normal * (lineWidth / 2.0);
            const QVector2D offset2 = normal2 * (lineWidth / 2.0);

            // add the line segment
            lineDataSegment.append(p1 + offset);
            lineDataSegment.append(p1 - offset);
            lineDataSegment.append(p2 + offset2);

            lineDataSegment.append(p2 + offset2);
            lineDataSegment.append(p2 - offset2);
            lineDataSegment.append(p1 - offset);

            if (!closed && (i == 0 || i == endIndex - 1)) {
                switch (capStyle) {
                case Qt::PenCapStyle::FlatCap:
                    // No additional vertices needed for FlatCap
                    break;
                case Qt::PenCapStyle::RoundCap: {
                    int numSegments = m_segmentCount;

                    float phi = i == 0 ? M_PI / numSegments : -M_PI / numSegments;

                    const float cPhi = cos(phi);
                    const float sPhi = sin(phi);
                    float rotation[4] = { cPhi, -sPhi, sPhi, cPhi };

                    QVector<QVector2D> capVertices;
                    capVertices.reserve(3 * numSegments);

                    QVector2D currentOffset = i == 0 ? offset : offset2;
                    const auto centerPoint = i == 0 ? p1 : p2;

                    for (int i = 0; i < numSegments; ++i) {
                        capVertices.append(centerPoint + currentOffset);
                        capVertices.append(centerPoint);
                        const auto tmp = rotation[0] * currentOffset[0] + rotation[1] * currentOffset[1];
                        currentOffset[1] = rotation[2] * currentOffset[0] + rotation[3] * currentOffset[1];
                        currentOffset[0] = tmp;
                        capVertices.append(centerPoint + currentOffset);
                    }

                    lineDataSegment = i == 0 ? capVertices + lineDataSegment : lineDataSegment + capVertices;
                    break;
                }
                case Qt::PenCapStyle::SquareCap: {
                    if (lineDataSegment.size() < 6)
                        break;

                    const auto direction = i == 0 ? pathData.at(i).tangent : pathData.at(nextI).tangent;

                    if (i == 0) {
                        lineDataSegment[0] -= direction * lineWidth / 2.0;
                        lineDataSegment[1] -= direction * lineWidth / 2.0;
                        lineDataSegment[5] -= direction * lineWidth / 2.0;
                    } else { // is last element
                        const auto end = lineDataSegment.size();
                        lineDataSegment[end - 2] += direction * lineWidth / 2.0;
                        lineDataSegment[end - 3] += direction * lineWidth / 2.0;
                        lineDataSegment[end - 4] += direction * lineWidth / 2.0;
                    }
                    break;
                }
                default:
                    // No additional vertices needed for other cap styles
                    break;
                }
            }
            if (i < endIndex - 1) {
                auto p3 = pathData[(i + 2) % pathData.count()].point;
                auto t3 = pathData[(i + 2) % pathData.count()].tangent;
                bool needsJoin = pathData[nextI].stepIndex != pathData[(i + 2) % pathData.count()].stepIndex;

                if (closed && (i + 2) == pathData.count()) {
                    p3 = pathData[(i + 3) % pathData.count()].point;
                    t3 = pathData[(i + 3) % pathData.count()].tangent;
                    needsJoin = pathData[nextI].stepIndex != pathData[(i + 3) % pathData.count()].stepIndex;
                }

                if (!needsJoin)
                    continue;

                const auto &diff2 = p3 - p2;
                const auto &normal2 = QVector2D(-diff2.y(), diff2.x()).normalized();
                const auto &offset2 = normal2 * lineWidth / 2.0;
                const bool turnLeft = normal.x() * normal2.y() - normal.y() * normal2.x() > 0;

                switch (joinType) {
                default:
                    // this should never be the case, since we handle all rive types.
                    qCDebug(rqqpRendering) << "Unhandled path join type. Using rounded join. Type:" << joinType;
                    Q_FALLTHROUGH();
                case Qt::PenJoinStyle::RoundJoin: {
                    auto phi = acos(normal.x() * normal2.x() + normal.y() * normal2.y()) / m_segmentCount;

                    phi = turnLeft ? phi : -phi;
                    QVector2D currentOffset = turnLeft ? -offset : offset;

                    const float cPhi = cos(phi);
                    const float sPhi = sin(phi);
                    const float rotation[4] = { cPhi, -sPhi, sPhi, cPhi };

                    QVector<QVector2D> capVertices;
                    capVertices.reserve(3 * m_segmentCount);

                    const auto centerPoint = p2;

                    for (int i = 0; i < m_segmentCount; ++i) {
                        capVertices.append(centerPoint + currentOffset);
                        capVertices.append(centerPoint);

                        const auto tmp = rotation[0] * currentOffset[0] + rotation[1] * currentOffset[1];
                        currentOffset[1] = rotation[2] * currentOffset[0] + rotation[3] * currentOffset[1];
                        currentOffset[0] = tmp;

                        capVertices.append(centerPoint + currentOffset);
                    }

                    lineDataSegment = lineDataSegment + capVertices;

                    break;
                }
                case Qt::PenJoinStyle::MiterJoin: {
                    if (!offset.isNull() && !offset2.isNull()) {
                        if (turnLeft) {
                            // calculate the intersection of the offset from p1 and p2
                            if (const auto pM = calculateIntersection(p1 - offset, p2 - offset, p3 - offset2, p2 - offset2);
                                pM.has_value()) {
                                lineDataSegment.append(p1 - offset);
                                lineDataSegment.append(pM.value());
                                lineDataSegment.append(p2 - offset2);
                            }
                        } else {
                            if (const auto pM = calculateIntersection(p1 + offset, p2 + offset, p3 + offset2, p2 + offset2);
                                pM.has_value()) {
                                lineDataSegment.append(p1 + offset);
                                lineDataSegment.append(pM.value());
                                lineDataSegment.append(p2 + offset2);
                            }
                        }
                    }
                    Q_FALLTHROUGH();
                }
                case Qt::PenJoinStyle::BevelJoin:
                    if (turnLeft) {
                        lineDataSegment.append(p1 - offset);
                        lineDataSegment.append(p2);
                        lineDataSegment.append(p2 - offset2);
                    } else {
                        lineDataSegment.append(p1 + offset);
                        lineDataSegment.append(p2);
                        lineDataSegment.append(p2 + offset2);
                    }
                    break;
                }
            }
        }

        m_pathOutlineVertices.append(lineDataSegment);
    }
#endif
}

void RiveQtPath::addRawPathImpl(const rive::RawPath &path)
{
    for (const auto &[verb, pts] : path) {
        switch (verb) {
        case rive::PathVerb::move:
            m_path.moveTo(pts[0].x, pts[0].y);
            break;
        case rive::PathVerb::line:
            m_path.lineTo(pts[1].x, pts[1].y);
            break;
        case rive::PathVerb::quad:
            m_path.quadTo(pts[1].x, pts[1].y, pts[2].x, pts[2].y);
            break;
        case rive::PathVerb::cubic:
            m_path.cubicTo(pts[1].x, pts[1].y, pts[2].x, pts[2].y, pts[3].x, pts[3].y);
            break;
        case rive::PathVerb::close:
            // why was this done?
            // m_path.lineTo(pts->x, pts->y);
            m_path.closeSubpath();
            break;
        }
    }
}

void RiveQtPath::updatePathSegmentsData()
{
    m_pathVertices.clear();

    if (m_path.isEmpty()) {
        m_pathSegmentDataDirty = false;
        return;
    }

    QTriangleSet triangles = qTriangulate(m_path, QTransform(), m_renderQuality);

    QVector<QVector2D> pathData;
    pathData.reserve(triangles.indices.size());
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

    m_pathVertices.append(pathData);
    m_pathSegmentDataDirty = false;
}
