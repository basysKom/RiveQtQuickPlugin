// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 Jonas Kalinka <jonas.kalinka@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <optional>

#include <QVector2D>
#include <QMatrix2x2>
#include <QtMath>

#include <private/qtriangulator_p.h>

#include "rqqplogging.h"
#include "riveqtpath.h"
#include "riveqtutils.h"

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

std::optional<QVector2D> calculateLineIntersection(const QVector2D &p1, const QVector2D &p2, const QVector2D &p3, const QVector2D &p4)
{
    const float x1 = p1.x(), y1 = p1.y();
    const float x2 = p2.x(), y2 = p2.y();
    const float x3 = p3.x(), y3 = p3.y();
    const float x4 = p4.x(), y4 = p4.y();

    const float x = ((x1 * y2 - y1 * x2) * (x3 - x4) - (x1 - x2) * (x3 * y4 - y3 * x4)) / ((x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4));
    const float y = ((x1 * y2 - y1 * x2) * (y3 - y4) - (y1 - y2) * (x3 * y4 - y3 * x4)) / ((x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4));

    // Check if the intersection point lies within the line segments
    if (x >= std::min(p1.x(), p2.x()) && x <= std::max(p1.x(), p2.x()) && y >= std::min(p1.y(), p2.y()) && y <= std::max(p1.y(), p2.y())
        && x >= std::min(p3.x(), p4.x()) && x <= std::max(p3.x(), p4.x()) && y >= std::min(p3.y(), p4.y()) && y <= std::max(p3.y(), p4.y()))
        return QVector2D(x, y);

    return {};
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
    if (!m_pathSegmentOutlineDataDirty) {
        return m_pathOutlineVertices;
    }

    updatePathSegmentsOutlineData();
    m_pathOutlineVertices.clear();

    // Early exit, nothing to do.
    if (m_pathSegmentsOutlineData.isEmpty()) {
        return m_pathOutlineVertices;
    }

    updatePathOutlineVertices(pen);
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

    QVector<PathDataPoint> pathDataEnhanced;
    pathDataEnhanced.reserve(m_qPainterPath.elementCount());

    const QPointF &point = m_qPainterPath.elementAt(0);
    const QVector2D &centerPoint = QVector2D(point.x(), point.y());
    int currentStepIndex { 0 };

    // Add the current point
    pathDataEnhanced.append({ centerPoint, QVector2D(), currentStepIndex });

    for (int i = 1; i < m_qPainterPath.elementCount(); ++i) {
        QPainterPath::Element element = m_qPainterPath.elementAt(i);

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
            const QPointF &controlPoint2 = m_qPainterPath.elementAt(i + 1);
            const QPointF &endPoint = m_qPainterPath.elementAt(i + 2);

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

// ChatGPT
// write a c++ function that checks, if two triangles given in points p1, p2, p3 and p4, p5, p6 overlap and returns bool that is true,
// if they overlap, false otherwise
bool RiveQtPath::doTrianglesOverlap(const QVector2D &p1, const QVector2D &p2, const QVector2D &p3, const QVector2D &p4, const QVector2D &p5,
                                    const QVector2D &p6)
{
    using Point = QVector2D;

    // Lambda function to calculate the orientation of three points
    auto orientation = [](Point p1, Point p2, Point p3) {
        double val = (p2.y() - p1.y()) * (p3.x() - p2.x()) - (p2.x() - p1.x()) * (p3.y() - p2.y());
        if (val == 0)
            return 0; // Collinear
        else if (val > 0)
            return 1; // Clockwise
        else
            return 2; // Counterclockwise
    };

    // Lambda function to check if a point lies within a triangle
    auto isInsideTriangle = [&](Point p1, Point p2, Point p3, Point pt) {
        int ori1 = orientation(p1, p2, pt);
        int ori2 = orientation(p2, p3, pt);
        int ori3 = orientation(p3, p1, pt);

        return (ori1 == ori2 && ori2 == ori3);
    };

    // Check if any vertex of the second triangle lies inside the first triangle
    if (isInsideTriangle(p1, p2, p3, p4) || isInsideTriangle(p1, p2, p3, p5) || isInsideTriangle(p1, p2, p3, p6))
        return true;

    // Check if any vertex of the first triangle lies inside the second triangle
    if (isInsideTriangle(p4, p5, p6, p1) || isInsideTriangle(p4, p5, p6, p2) || isInsideTriangle(p4, p5, p6, p3))
        return true;

    return false;
}

QVector<std::tuple<int, int>> RiveQtPath::findOverlappingTriangles(const QVector<QVector2D> &trianglePoints)
{
    using Point = QVector2D;
    const auto &t = trianglePoints;

    QVector<std::tuple<int, int>> result;

    for (size_t i = 0; i < trianglePoints.size() - 6; i += 3) {
        Point p1 = t.at(i), p2 = t.at(i + 1), p3 = t.at(i + 2);

        for (size_t j = i + 3; j < trianglePoints.size() - 3; j += 3) {
            Point p4 = t.at(j), p5 = t.at(j + 1), p6 = t.at(j + 2);

            if (!doTrianglesOverlap(p1, p2, p3, p4, p5, p6)) {
                continue;
            }
            result.append({ i / 3, j / 3 });
        }
    }
    return result;
}

QVector<QVector2D> RiveQtPath::splitTriangles(const QVector<QVector2D> &trianglePoints)
{
    QVector<QVector2D> tri1 { trianglePoints.at(0), trianglePoints.at(1), trianglePoints.at(2) };
    QVector<QVector2D> tri2 { trianglePoints.at(3), trianglePoints.at(4), trianglePoints.at(5) };

    QVector<QVector2D> poly1(tri1);
    QVector<QVector2D> poly2(tri2);

    using Point = QVector2D;

    // Lambda function to calculate the orientation of three points
    auto orientation = [](Point p1, Point p2, Point p3) {
        double val = (p2.y() - p1.y()) * (p3.x() - p2.x()) - (p2.x() - p1.x()) * (p3.y() - p2.y());
        if (val == 0)
            return 0; // Collinear
        else if (val > 0)
            return 1; // Clockwise
        else
            return 2; // Counterclockwise
    };

    // Lambda function to check if a point lies within a triangle
    auto isInsideTriangle = [&](Point p1, Point p2, Point p3, Point pt) {
        int ori1 = orientation(p1, p2, pt);
        int ori2 = orientation(p2, p3, pt);
        int ori3 = orientation(p3, p1, pt);

        return (ori1 == ori2 && ori2 == ori3);
    };

    poly1.erase(std::remove_if(poly1.begin(), poly1.end(),
                               [&tri2, &isInsideTriangle](auto &p) { return isInsideTriangle(tri2.at(0), tri2.at(1), tri2.at(2), p); }),
                poly1.end());
    poly2.erase(std::remove_if(poly2.begin(), poly2.end(),
                               [&tri1, &isInsideTriangle](auto &p) { return isInsideTriangle(tri1.at(0), tri1.at(1), tri1.at(2), p); }),
                poly2.end());

    if (poly2.size() == 3 && poly1.size() == 3)
        return tri1 + tri2;

    // one triangle is inside the other
    if (poly2.empty()) {
        return tri1;
    }
    if (poly1.empty()) {
        return tri2;
    }

    auto addIntersectionPoints = [](QVector<QVector2D> &poly1, QVector<QVector2D> &poly2, const QVector<QVector2D> &tri1,
                                    const QVector<QVector2D> &tri2) {
        QVector<QVector2D> intersections;
        for (int i = 0; i < tri1.size(); ++i) {
            const auto &p1 = tri1.at(i);
            const auto &p2 = tri1.at((i + 1) % tri1.size());
            for (int j = 0; j < tri2.size(); ++j) {
                const auto &p3 = tri2.at(j);
                const auto &p4 = tri2.at((j + 1) % tri2.size());

                if (const auto pInter = calculateLineIntersection(p1, p2, p3, p4); pInter.has_value()) {
                    intersections.append(pInter.value());
                }
            }
        }

        if (intersections.size() > 2)
            return;
        // At least one point is inside the other triangle and was removed
        if (poly1.size() < 3) {
            poly1 += intersections;
        }
        if (poly2.size() < 3) {
            poly2 += intersections;
        }
    };

    auto triangulate = [](const QVector<QVector2D> poly) {
        QVector<QVector2D> triangles;

        triangles.append(poly.at(0));
        triangles.append(poly.at(1));
        triangles.append(poly.at(2));

        triangles.append(poly.at(0));
        triangles.append(poly.at(2));
        triangles.append(poly.at(3));

        return triangles;
    };

    addIntersectionPoints(poly1, poly2, tri1, tri2);

    if (poly1.size() == 4) {
        poly1 = triangulate(poly1);
    } else if (!poly1.empty()) {
        poly1 = tri1;
    }
    if (poly2.size() == 4) {
        poly2 = triangulate(poly2);
    } else if (!poly2.empty()) {
        poly2 = tri2;
    }

    return poly1 + poly2;
};

void removeOverlappingTriangles(QVector<QVector2D> &triangles)
{
    Q_ASSERT(triangles.size() % 3 == 0);
    Q_ASSERT(triangles.size() > 6);

    auto addIntersectionPoints = [](QVector<QVector2D> &poly1, QVector<QVector2D> &poly2, const QVector<QVector2D> &tri1,
                                    const QVector<QVector2D> &tri2) {
        QVector<QVector2D> intersections;
        for (int i = 0; i < tri1.size(); ++i) {
            const auto &p1 = tri1.at(i);
            const auto &p2 = tri1.at((i + 1) % tri1.size());
            for (int j = 0; j < tri2.size(); ++j) {
                const auto &p3 = tri2.at(j);
                const auto &p4 = tri2.at((j + 1) % tri2.size());

                if (const auto pInter = calculateLineIntersection(p1, p2, p3, p4); pInter.has_value()) {
                    intersections.append(pInter.value());
                }
            }
        }

        if (intersections.size() > 2)
            return;
        // At least one point is inside the other triangle and was removed
        if (poly1.size() < 3) {
            poly1 += intersections;
        }
        if (poly2.size() < 3) {
            poly2 += intersections;
        }
    };

    auto triangulate = [](const QVector<QVector2D> poly) {
        QVector<QVector2D> triangles;

        triangles.append(poly.at(0));
        triangles.append(poly.at(1));
        triangles.append(poly.at(2));

        triangles.append(poly.at(0));
        triangles.append(poly.at(2));
        triangles.append(poly.at(3));

        return triangles;
    };

    const auto &t = triangles;
    QVector<QVector2D> newTriangles;

    //    bool changes;
    //    do {
    //        changes = false;
    //        for (size_t i = 0; i < triangles.size() - 6; i += 3) {
    //            Point p1 = t.at(i), p2 = t.at(i + 1), p3 = t.at(i + 2);

    //            for (size_t j = i + 3; j < triangles.size() - 3; j += 3) {
    //                Point p4 = t.at(j), p5 = t.at(j + 1), p6 = t.at(j + 2);

    //                if (!doTrianglesOverlap(p1, p2, p3, p4, p5, p6)) {
    //                    //                newTriangles += { p1, p2, p3, p4, p5, p6 }; // copy the points.
    //                    continue;
    //                }
    //                //                changes = true;

    //                qDebug() << "Is overlapping" << i / 3 << j / 3 << t.at(i) << t.at(i + 1) << t.at(i + 2) << t.at(j) << t.at(j +
    //                1)
    //                         << t.at(j + 2);

    //                QVector<QVector2D> tri1 { p1, p2, p3 };
    //                QVector<QVector2D> tri2 { p4, p5, p6 };

    //                QVector<QVector2D> poly1(tri1);
    //                QVector<QVector2D> poly2(tri2);

    //                poly1.erase(
    //                    std::remove_if(poly1.begin(), poly1.end(),
    //                                   [&tri2, &isInsideTriangle](auto &p) { return isInsideTriangle(tri2.at(0), tri2.at(1),
    //                                   tri2.at(2), p); }),
    //                    poly1.end());
    //                poly2.erase(
    //                    std::remove_if(poly2.begin(), poly2.end(),
    //                                   [&tri1, &isInsideTriangle](auto &p) { return isInsideTriangle(tri1.at(0), tri1.at(1),
    //                                   tri1.at(2), p); }),
    //                    poly2.end());

    //                if (poly2.size() == 3 && poly1.size() == 3)
    //                    continue;

    //                // one triangle is inside the other
    //                if (poly2.empty()) {
    //                    newTriangles += poly1;
    //                    continue;
    //                }
    //                if (poly1.empty()) {
    //                    newTriangles += poly1;
    //                    continue;
    //                }

    //                addIntersectionPoints(poly1, poly2, tri1, tri2);

    //                if (poly1.size() == 4) {
    //                    poly1 = triangulate(poly1);
    //                } else if (!poly1.empty()) {
    //                    poly1 = tri1;
    //                }
    //                if (poly2.size() == 4) {
    //                    poly2 = triangulate(poly2);
    //                } else if (!poly2.empty()) {
    //                    poly2 = tri2;
    //                }
    //                newTriangles += poly1;
    //                newTriangles += poly2;
    //            }
    //        }
    //        //        if (changes)
    //        triangles = newTriangles;
    //    } while (changes);
}

void RiveQtPath::updatePathOutlineVertices(const QPen &pen)
{
    const qreal lineWidth = pen.widthF();
    const Qt::PenJoinStyle &joinType = pen.joinStyle();
    const Qt::PenCapStyle &capStyle = pen.capStyle();

    for (const auto &pathData : qAsConst(m_pathSegmentsOutlineData)) {
        if (pathData.size() <= 1) {
            continue;
        }

        QVector<QVector2D> lineDataSegment;

        auto appendTriangle = [&lineDataSegment](const QVector2D &p1, const QVector2D &p2, const QVector2D &p3) {
            if (p1 == p2 || p1 == p3 || p2 == p3) {
                return;
            }
            lineDataSegment.append(p1);
            lineDataSegment.append(p2);
            lineDataSegment.append(p3);
        };

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
            appendTriangle(p1 + offset, p1 - offset, p2 + offset2);
            appendTriangle(p2 + offset2, p2 - offset2, p1 - offset);

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
                                appendTriangle(p1 - offset, pM.value(), p2 - offset2);
                            }
                        } else {
                            if (const auto pM = calculateIntersection(p1 + offset, p2 + offset, p3 + offset2, p2 + offset2);
                                pM.has_value()) {
                                appendTriangle(p1 + offset, pM.value(), p2 + offset2);
                            }
                        }
                    }
                    Q_FALLTHROUGH();
                }
                case Qt::PenJoinStyle::BevelJoin:
                    if (turnLeft) {
                        appendTriangle(p1 - offset, p2, p2 - offset2);
                    } else {
                        appendTriangle(p1 + offset, p2, p2 + offset2);
                    }
                    break;
                }
            }
        }

        //        qDebug() << lineDataSegment.size();
        //        removeOverlappingTriangles(lineDataSegment);
        //        qDebug() << lineDataSegment.size();

        m_pathOutlineVertices.append(lineDataSegment);
    }
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
