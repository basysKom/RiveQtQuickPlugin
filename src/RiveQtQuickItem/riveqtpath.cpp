// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 Jonas Kalinka <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later
#include <optional>

#include <QVector2D>
#include <QMatrix2x2>
#include <QtMath>

#include <private/qtriangulator_p.h>

#include <clipper2/clipper.h>
#include <clipper2/clipper.offset.h>

#include "polygon_triangulate/polygon_triangulate.hpp"

#include "rqqplogging.h"
#include "riveqtpath.h"
#include "riveqtutils.h"

Clipper2Lib::PathD toClipperPath(const QVector<RiveQtPath::PathDataPoint> &path)
{
    Clipper2Lib::PathD cp;
    cp.reserve(path.size());
    for (const auto &p : path) {
        cp.push_back({ p.point.x(), p.point.y() });
    }
    return cp;
}

RiveQtPath::RiveQtPath(const unsigned segmentCount)
{
    m_qPainterPath.setFillRule(Qt::FillRule::WindingFill);
    setSegmentCount(segmentCount);
    m_pathSegmentOutlineDataDirty = true;
    m_pathSegmentDataDirty = true;
}

RiveQtPath::RiveQtPath(const RiveQtPath &other)
{
    m_qPainterPath = other.m_qPainterPath;
    m_pathVertices = other.m_pathVertices;
    m_pathSegmentsOutlineData = other.m_pathSegmentsOutlineData;
    m_pathOutlineVertices = other.m_pathOutlineVertices;
    m_segmentCount = other.m_segmentCount;
}

RiveQtPath::RiveQtPath(const rive::RawPath &rawPath, rive::FillRule fillRule, const unsigned segmentCount)
{
    m_qPainterPath.clear();
    m_qPainterPath.setFillRule(RiveQtUtils::riveFillRuleToQt(fillRule));
    setSegmentCount(segmentCount);

    for (const auto &[verb, pts] : rawPath) {
        switch (verb) {
        case rive::PathVerb::move:
            m_qPainterPath.moveTo(pts->x, pts->y);
            break;
        case rive::PathVerb::line:
            m_qPainterPath.lineTo(pts->x, pts->y);
            break;
        case rive::PathVerb::quad:
            m_qPainterPath.quadTo(pts[0].x, pts[0].y, pts[1].x, pts[1].y);
            break;
        case rive::PathVerb::cubic:
            m_qPainterPath.cubicTo(pts[0].x, pts[0].y, pts[1].x, pts[1].y, pts[2].x, pts[2].y);
            break;
        case rive::PathVerb::close:
            m_qPainterPath.lineTo(pts->x, pts->y);
            m_qPainterPath.closeSubpath();
            break;
        default:
            qCDebug(rqqpRendering) << "Unhandled case in RiveQtPath Constructor" << static_cast<int>(verb);
            break;
        }
    }
    m_pathSegmentOutlineDataDirty = true;
    m_pathSegmentDataDirty = true;
}

void RiveQtPath::rewind()
{
    m_pathVertices.clear();
    m_pathSegmentsOutlineData.clear();
    m_qPainterPath.clear();
    m_pathSegmentOutlineDataDirty = true;
    m_pathSegmentDataDirty = true;
}

void RiveQtPath::fillRule(rive::FillRule value)
{
    switch (value) {
    case rive::FillRule::evenOdd:
        m_qPainterPath.setFillRule(Qt::FillRule::OddEvenFill);
        break;
    case rive::FillRule::nonZero:
        m_qPainterPath.setFillRule(Qt::FillRule::WindingFill);
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
    m_qPainterPath.addPath(qPath);

    m_pathSegmentOutlineDataDirty = true;
    m_pathSegmentDataDirty = true;
}

void RiveQtPath::setQPainterPath(QPainterPath path)
{
    m_qPainterPath = path;
}

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

void RiveQtPath::setSegmentCount(const unsigned segmentCount)
{
    if (segmentCount == 0u) {
        qCDebug(rqqpRendering) << "Segment count cannot be 0. Using 1 instead.";
        m_segmentCount = 1u;
    } else if (segmentCount > 100u) {
        qCDebug(rqqpRendering) << "Segment count is limited to 100 to avoid exceedingly long rendertime.";
        m_segmentCount = 100u;
    } else {
        m_segmentCount = segmentCount;
    }
}

QVector<QVector<QVector2D>> RiveQtPath::toVertices()
{
    if (m_pathSegmentDataDirty) {
        updatePathSegmentsData();
    }
    return m_pathVertices;
}

QVector<QVector<QVector2D>> RiveQtPath::toVerticesLine(const QPen &pen)
{
    if (m_pathSegmentOutlineDataDirty) {
        updatePathSegmentsOutlineData();
        m_pathOutlineVertices.clear();
    } else {
        return m_pathOutlineVertices;
    }

    // Early exit, nothing to do.
    if (m_pathSegmentsOutlineData.isEmpty()) {
        return m_pathOutlineVertices;
    }

    const qreal lineWidth = pen.widthF();
    const Qt::PenJoinStyle &joinType = pen.joinStyle();
    const Qt::PenCapStyle &capStyle = pen.capStyle();

    const auto &inflatedPaths =
        Clipper2Lib::InflatePaths(m_clipperPaths, lineWidth / 2.0, Clipper2Lib::JoinType::Miter, Clipper2Lib::EndType::Butt, 1, 2.0, 2.0);

    QVector<QVector2D> pathPoints;
    QVector<QVector2D> pathVertices;

    for (const auto &path : inflatedPaths) {
        //        const auto &path = inflatedPaths.at(i);
        if (path.size() < 3)
            continue;

        pathPoints.clear();
        pathVertices.clear();
        pathPoints.reserve(path.size());
        int outputSize = 3 * (path.size() - 2);
        pathVertices.reserve(outputSize);

        double *X = new double[path.size()];
        double *Y = new double[path.size()];

        for (size_t i = 0; i < path.size(); ++i) {
            const auto &p = path.at(i);
            qDebug() << p.x << p.y;
            if (i > 0 && p != path.at(i - 1)) {
                pathPoints.append(QVector2D(p.x, p.y));
                X[i] = p.x;
                Y[i] = p.y;
            }
        }

        int *triangles = polygon_triangulate(path.size(), X, Y);

        qDebug() << "size" << pathPoints.size();

        for (size_t i = 0; i < outputSize; ++i) {
            qDebug() << triangles[i];
            pathVertices.append(pathPoints.at(triangles[i] % pathPoints.size()));
        }
        delete triangles;
        delete[] X;
        delete[] Y;

        m_pathOutlineVertices.append(pathVertices);
    }

    //    size_t pointCount { 0 };

    //    for (const auto &path : inflatedPaths) {
    //        pointCount += path.size();
    //    }
    //    for (const auto &path : inflatedPaths) {
    //        for (int i = 0; i < path.size() / 2; ++i) {
    //            const auto &p = path.at(i);
    //            const auto &p2 = path.at(path.size() - i - 1);
    //            points.push_back(p.x);
    //            points.push_back(p.y);
    //            points.push_back(p2.x);
    //            points.push_back(p2.y);
    //            qDebug() << p.x << "\t" << p.y;
    //            qDebug() << p2.x << "\t" << p2.y;
    //        }
    //        if (path.size() % 2) {
    //            points.resize(points.size() - 2);
    //        }
    //    }

    //    QVectorPath outlinePath(&points[0], points.size());

    //    const auto &triangles = qTriangulate(outlinePath);
    //    int n int *polygon_triangulate();

    //    QVector<QVector2D> pathData;
    //    pathData.reserve(triangles.indices.size());
    //    int index;
    //    for (int i = 0; i < triangles.indices.size(); i++) {
    //        if (triangles.indices.type() == QVertexIndexVector::UnsignedInt) {
    //            index = static_cast<const quint32 *>(triangles.indices.data())[i];
    //        } else {
    //            index = static_cast<const quint16 *>(triangles.indices.data())[i];
    //        }

    //        const qreal x = triangles.vertices[2 * index];
    //        const qreal y = triangles.vertices[2 * index + 1];

    //        pathData.append(QVector2D(x, y));
    //    }

    //    m_pathOutlineVertices.append(pathData);

#if false

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
                QVector2D p3 = pathData[(i + 2) % pathData.count()].point;

                if (closed && (i + 2) == pathData.count())
                    p3 = pathData[(i + 3) % pathData.count()].point;

                const auto diff2 = p3 - p2;
                const auto normal2 = QVector2D(-diff2.y(), diff2.x()).normalized();
                const auto offset2 = normal2 * lineWidth / 2.0;
                const bool turnLeft = normal.x() * normal2.y() - normal.y() * normal2.x() > 0;

                switch (joinType) {
                default:
                    qCDebug(rqqpRendering) << "Unhandled path join type. Using rounded join. Type:" << joinType;
                    // this should never be the case, since we handle all rive types.
                    // fallthrough intended
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
                            if (const auto pM = calculateIntersection(p1 - offset, p2 - offset, p3 - offset2, p2 - offset2);
                                pM.has_value()) {
                                lineDataSegment.append(p2 - offset);
                                lineDataSegment.append(pM.value());
                                lineDataSegment.append(p2 - offset2);
                            }
                        } else {
                            if (const auto pM = calculateIntersection(p1 + offset, p2 + offset, p3 + offset2, p2 + offset2);
                                pM.has_value()) {
                                lineDataSegment.append(p2 + offset);
                                lineDataSegment.append(pM.value());
                                lineDataSegment.append(p2 + offset2);
                            }
                        }
                    }
                    // intended fallthrough
                }
                case Qt::PenJoinStyle::BevelJoin:
                    if (turnLeft) {
                        lineDataSegment.append(p2 - offset);
                        lineDataSegment.append(p2);
                        lineDataSegment.append(p2 - offset2);
                    } else {
                        lineDataSegment.append(p2 + offset);
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

    m_pathSegmentOutlineDataDirty = false;

    return m_pathOutlineVertices;
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

    if (m_qPainterPath.isEmpty()) {
        m_pathSegmentOutlineDataDirty = false;
        return;
    }

    QVector<PathDataPoint> pathData;
    pathData.reserve(m_qPainterPath.elementCount());

    const QPointF &point = m_qPainterPath.elementAt(0);
    const QVector2D &centerPoint = QVector2D(point.x(), point.y());

    // Add the current point
    pathData.append({ centerPoint, {} });

    for (int i = 1; i < m_qPainterPath.elementCount(); ++i) {
        QPainterPath::Element element = m_qPainterPath.elementAt(i);

        switch (element.type) {
        case QPainterPath::MoveToElement:
            m_pathSegmentsOutlineData.append(pathData);
            pathData.clear();
            pathData.append({ QVector2D(element.x, element.y), QVector2D() });
            break;

        case QPainterPath::LineToElement:
            pathData.append({ QVector2D(element.x, element.y), QVector2D() });
            break;

        case QPainterPath::CurveToElement: {
            const QPointF &startPoint = pathData.last().point.toPointF();
            const QPointF &controlPoint1 = element;
            const QPointF &controlPoint2 = m_qPainterPath.elementAt(i + 1);
            const QPointF &endPoint = m_qPainterPath.elementAt(i + 2);

            pathData.last().tangent = cubicBezierTangent(startPoint, controlPoint1, controlPoint2, endPoint, 0.f);

            for (int j = 1; j <= m_segmentCount; ++j) {
                const qreal t = static_cast<qreal>(j) / m_segmentCount;
                const QPointF &point = cubicBezier(startPoint, controlPoint1, controlPoint2, endPoint, t);
                pathData.append(
                    { QVector2D(point.x(), point.y()), cubicBezierTangent(startPoint, controlPoint1, controlPoint2, endPoint, t) });
            }

            i += 2; // Skip the next two control points, as we already processed them.
            break;
        }
        default:
            break;
        }
    }

    m_pathSegmentsOutlineData.append(pathData);

    m_clipperPaths.clear();
    m_clipperPaths.reserve(m_pathSegmentsOutlineData.size());

    for (const auto &path : m_pathSegmentsOutlineData) {
        m_clipperPaths.push_back(toClipperPath(path));
    }

    m_pathSegmentOutlineDataDirty = false;
}

void RiveQtPath::updatePathSegmentsData()
{
    m_pathVertices.clear();

    if (m_qPainterPath.isEmpty()) {
        m_pathSegmentDataDirty = false;
        return;
    }

    QTriangleSet triangles = qTriangulate(m_qPainterPath);

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
