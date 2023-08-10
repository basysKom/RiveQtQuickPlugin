// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 Jonas Kalinka <jonas.kalinka@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <array>
#include <optional>
#include <set>

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

std::optional<QVector2D> calculateIntersection(const QVector2D &p1, const QVector2D &p2, const QVector2D &p3, const QVector2D &p4)
{
    const float x1 = p1.x(), y1 = p1.y();
    const float x2 = p2.x(), y2 = p2.y();
    const float x3 = p3.x(), y3 = p3.y();
    const float x4 = p4.x(), y4 = p4.y();

    const float denominator = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);

    // If the lines are parallel or coincident, return an invalid point
    if (std::abs(denominator) < 0.001f) {
        return {};
    }

    const float factor1 = (x1 * y2 - y1 * x2);
    const float factor2 = (x3 * y4 - y3 * x4);

    const float intersectX = (factor1 * (x3 - x4) - (x1 - x2) * factor2) / denominator;
    const float intersectY = (factor1 * (y3 - y4) - (y1 - y2) * factor2) / denominator;
    return QVector2D(intersectX, intersectY);
}

std::optional<QVector2D> RiveQtPath::calculateLineIntersection(const QVector2D &p1, const QVector2D &p2, const QVector2D &p3,
                                                               const QVector2D &p4)
{
    using Point = QVector2D;

    auto areCollinear = [](const Point &p1, const Point &p2, const Point &p3) {
        // Check if the three points are collinear using cross product
        return std::abs((p2.x() - p1.x()) * (p3.y() - p1.y()) - (p2.y() - p1.y()) * (p3.x() - p1.x()))
            < std::numeric_limits<float>::epsilon();
    };

    // Check if the points are on one of the edges
    if (areCollinear(p1, p2, p3) || areCollinear(p1, p2, p4) || areCollinear(p3, p4, p1) || areCollinear(p3, p4, p2))
        return {};

    const auto &intersection = calculateIntersection(p1, p2, p3, p4);

    if (!intersection.has_value())
        return {};

    const auto x = intersection.value().x();
    const auto y = intersection.value().y();

    // Fuzzy comparison function to check if two floating-point values are approximately equal
    auto fuzzyCompare = [](float a, float b) {
        const float epsilon = 1e-4;
        return std::fabs(a - b) < epsilon;
    };

    // Fuzzy comparison function to check if a value is within a range
    auto fuzzyInRange = [](float value, float minVal, float maxVal) {
        const float epsilon = 1e-4;
        return value >= (minVal - epsilon) && value <= (maxVal + epsilon);
    };

    auto fuzzyComparePoints = [&fuzzyInRange](float x, float y, const QVector2D &p1, const QVector2D &p2, const QVector2D &p3,
                                              const QVector2D &p4) {
        return (fuzzyInRange(x, std::min(p1.x(), p2.x()), std::max(p1.x(), p2.x()))
                && fuzzyInRange(y, std::min(p1.y(), p2.y()), std::max(p1.y(), p2.y()))
                && fuzzyInRange(x, std::min(p3.x(), p4.x()), std::max(p3.x(), p4.x()))
                && fuzzyInRange(y, std::min(p3.y(), p4.y()), std::max(p3.y(), p4.y())));
    };
    // Check if the intersection point lies within the line segments
    if (fuzzyComparePoints(x, y, p1, p2, p3, p4))
        return QVector2D(x, y);

    return {};
}

float crossProduct(const QVector2D &a, const QVector2D &b)
{
    return (a.x() * b.y()) - (a.y() * b.x());
}

int RiveQtPath::orientation(const QVector<QVector2D> &points)
{
    float ori { 0.f };

    for (int i = 0; i < points.size(); i++) {
        int nextIdx = (i + 1) % points.size();
        ori += crossProduct(points[i], points[nextIdx]);
    }

    if (std::abs(ori) < 0.001f)
        return 0; // Collinear
    else if (ori < 0)
        return 1; // Clockwise
    else
        return 2; // Counterclockwise
};

void RiveQtPath::FixWinding(QVector2D &p1, QVector2D &p2, QVector2D &p3)
{
    QVector<QVector2D> p { p1, p2, p3 };
    if (orientation(p) == 1) {
        QVector2D a = p3;
        p3 = p2;
        p2 = a;
    }
}

inline double Det2D(const QVector2D &p1, const QVector2D &p2, const QVector2D &p3)
{
    return +p1.x() * (p2.y() - p3.y()) + p2.x() * (p3.y() - p1.y()) + p3.x() * (p1.y() - p2.y());
}

void CheckTriWinding(QVector2D &p1, QVector2D &p2, QVector2D &p3)
{
    double detTri = Det2D(p1, p2, p3);
    if (detTri < 0.0) {
        QVector2D a = p3;
        p3 = p2;
        p2 = a;
    }
}

bool BoundaryCollideChk(QVector2D &p1, QVector2D &p2, QVector2D &p3, double eps)
{
    return Det2D(p1, p2, p3) < eps;
}

bool BoundaryDoesntCollideChk(QVector2D &p1, QVector2D &p2, QVector2D &p3, double eps)
{
    return Det2D(p1, p2, p3) <= eps;
}

// Adapted from
// https://gist.github.com/TimSC/5ba18ae21c4459275f90
bool TriTri2D(QVector2D *t1, QVector2D *t2, double eps = 0.0, bool onBoundary = true)
{
    // Trangles must be expressed anti-clockwise
    CheckTriWinding(t1[0], t1[1], t1[2]);
    CheckTriWinding(t2[0], t2[1], t2[2]);

    bool (*chkEdge)(QVector2D &, QVector2D &, QVector2D &, double) = nullptr;
    if (onBoundary) // Points on the boundary are considered as colliding
        chkEdge = BoundaryCollideChk;
    else // Points on the boundary are not considered as colliding
        chkEdge = BoundaryDoesntCollideChk;

    // For edge E of trangle 1,
    for (int i = 0; i < 3; i++) {
        int j = (i + 1) % 3;

        // Check all points of trangle 2 lay on the external side of the edge E. If
        // they do, the triangles do not collide.
        if (chkEdge(t1[i], t1[j], t2[0], eps) && chkEdge(t1[i], t1[j], t2[1], eps) && chkEdge(t1[i], t1[j], t2[2], eps))
            return false;
    }

    // For edge E of trangle 2,
    for (int i = 0; i < 3; i++) {
        int j = (i + 1) % 3;

        // Check all points of trangle 1 lay on the external side of the edge E. If
        // they do, the triangles do not collide.
        if (chkEdge(t2[i], t2[j], t1[0], eps) && chkEdge(t2[i], t2[j], t1[1], eps) && chkEdge(t2[i], t2[j], t1[2], eps))
            return false;
    }

    // The triangles collide
    return true;
}

bool RiveQtPath::doTrianglesOverlap(const QVector2D &p1, const QVector2D &p2, const QVector2D &p3, const QVector2D &p4, const QVector2D &p5,
                                    const QVector2D &p6)
{
    QVector2D a[3] = { p1, p2, p3 };
    QVector2D b[3] = { p4, p5, p6 };

    return TriTri2D(a, b, 0.001, false);
}

QVector<std::pair<size_t, size_t>> RiveQtPath::findOverlappingTriangles(const QVector<QVector2D> &trianglePoints)
{
    if (trianglePoints.size() < 6) {
        return {};
    }

    Q_ASSERT(!(trianglePoints.size() % 3));

    using Point = QVector2D;
    const auto &t = trianglePoints;

    QVector<std::pair<size_t, size_t>> result;

    for (size_t i = 0; i < trianglePoints.size() - 3; i += 3) {
        Point p1 = t.at(i), p2 = t.at(i + 1), p3 = t.at(i + 2);

        for (size_t j = i + 3; j < trianglePoints.size(); j += 3) {
            Point p4 = t.at(j), p5 = t.at(j + 1), p6 = t.at(j + 2);

            if (!doTrianglesOverlap(p1, p2, p3, p4, p5, p6)) {
                continue;
            }
            result.append({ i / 3, j / 3 });
        }
    }
    return result;
}

bool checkPermutation(const std::array<QVector2D, 3> &points1, const std::array<QVector2D, 3> &points2)
{
    // Check if both sets of arrays have the same size
    if (points1.size() != points2.size()) {
        return false;
    }

    // Sort the arrays
    std::array<QVector2D, 3> sorted_points1 = points1;
    std::array<QVector2D, 3> sorted_points2 = points2;

    auto compare = [](const auto &a, const auto &b) { return a.x() == b.x() ? a.y() < b.y() : a.x() < b.x(); };

    std::sort(sorted_points1.begin(), sorted_points1.end(), compare);
    std::sort(sorted_points2.begin(), sorted_points2.end(), compare);

    // Compare the sorted arrays
    return sorted_points1 == sorted_points2;
}

// Function to check if a point is inside a polygon
bool RiveQtPath::isInsidePolygon(const QVector<QVector2D> &polygon, const QVector2D &p)
{
    int n = polygon.size();
    int intersectCount = 0;

    for (int i = 0; i < n; i++) {
        int j = (i + 1) % n;

        // Check if the point is on the edge of the polygon
        QVector2D edgeVector = polygon[j] - polygon[i];
        QVector2D pointVector = p - polygon[i];
        double cp = crossProduct(edgeVector, pointVector);

        if (qFuzzyCompare(cp, 0.0) && pointVector.lengthSquared() <= edgeVector.lengthSquared()) {
            return false; // The point is on the edge, consider it outside the polygon
        }

        if ((polygon[i].y() > p.y()) != (polygon[j].y() > p.y())
            && p.x() < (polygon[j].x() - polygon[i].x()) * (p.y() - polygon[i].y()) / (polygon[j].y() - polygon[i].y()) + polygon[i].x()) {
            intersectCount++;
        }
    }

    return intersectCount % 2 == 1;
}

void RiveQtPath::concaveHull(const QVector<QVector2D> &t1, const QVector<QVector2D> &t2, QVector<QVector2D> &result, const size_t i)
{
    qDebug() << "Result" << result << "END";
    using Point = QVector2D;

    const auto &current = t1.at(i % t1.size());

    if (!result.empty() && std::abs(result.at(0).x() - current.x()) < 0.001f && std::abs(result.at(0).y() - current.y()) < 0.001f)
        return;

    if (isInsidePolygon(t2, current)) {
        if (i + 1 == t1.size() && result.empty())
            // t1 is completely covered by t2
            result = t2;
        else
            concaveHull(t1, t2, result, i + 1);

        return;
    }

    result.append(current);

    const auto &next = t1.at((i + 1) % t1.size());
    QVector<std::pair<size_t, QVector2D>> intersections;

    for (int j = 0; j < t2.size(); ++j) {
        QVector2D p1 = t2.at(j), p2 = t2.at((j + 1) % t2.size());
        if (auto inter = calculateLineIntersection(p1, p2, current, next); inter.has_value()) {
            intersections.append({ j + 1, inter.value() });
        }
    }

    // continue with the next point in the current polygon, if there is no intersection
    if (intersections.empty()) {
        concaveHull(t1, t2, result, i + 1);
        return;
    }

    // add the closest intersection to the current point and continue with the end point on the other polygon from the intersecting line.
    auto minDistSq = std::numeric_limits<float>::infinity();
    size_t indexMin = 0;
    for (size_t i = 0; i < intersections.size(); ++i) {
        const auto diff = result.last() - intersections.at(i).second;
        const float distSq = diff.x() * diff.x() + diff.y() * diff.y();
        if (distSq < minDistSq) {
            minDistSq = distSq;
            indexMin = i;
        }
    }

    if (result.contains(intersections.at(indexMin).second)) {
        //
        return;
    }
    result.append(intersections.at(indexMin).second);

    // Notice: t2 and t1 are switched
    concaveHull(t2, t1, result, intersections.at(indexMin).first);
    return;
};

template<class T>
class VectorSet
{
public:
    using iterator = typename std::vector<T>::iterator;
    using const_iterator = typename std::vector<T>::const_iterator;
    iterator begin() { return theVector.begin(); }
    iterator end() { return theVector.end(); }
    const_iterator begin() const { return theVector.begin(); }
    const_iterator end() const { return theVector.end(); }
    const T &front() const { return theVector.front(); }
    const T &back() const { return theVector.back(); }
    void insert(const T &item)
    {
        if (theSet.insert(item).second)
            theVector.push_back(item);
    }
    size_t count(const T &item) const { return theSet.count(item); }
    bool empty() const { return theSet.empty(); }
    size_t size() const { return theSet.size(); }

private:
    std::vector<T> theVector;
    std::set<T> theSet;
};

void RiveQtPath::removeOverlappingTriangles(QVector<QVector2D> &triangles)
{
    //    qDebug() << "triangles list" << triangles << "triangles list end";
    auto overlappingTriangles = findOverlappingTriangles(triangles);

    if (overlappingTriangles.empty())
        return;

    QVector<QVector2D> nonOverlappingTriangles;
    nonOverlappingTriangles.reserve(triangles.size() + overlappingTriangles.size() * 3);

    QVector<VectorSet<size_t>> clusters;
    std::set<size_t> overlappingTrianglesIndizes;

    while (!overlappingTriangles.empty()) {
        VectorSet<size_t> cluster;
        cluster.insert(overlappingTriangles.at(0).first);
        cluster.insert(overlappingTriangles.at(0).second);
        for (const auto &p : overlappingTriangles) {
            if (cluster.count(p.first) || cluster.count(p.second)) {
                cluster.insert(p.first);
                cluster.insert(p.second);
                overlappingTrianglesIndizes.insert(p.first);
                overlappingTrianglesIndizes.insert(p.second);
            }
        }
        clusters.append(cluster);
        overlappingTriangles.erase(std::remove_if(overlappingTriangles.begin(), overlappingTriangles.end(),
                                                  [&cluster](const auto &e) { return cluster.count(e.first) || cluster.count(e.second); }),
                                   overlappingTriangles.end());
    }

    for (const auto &cluster : clusters) {
        const auto &indexFirstTriangle = cluster.front();
        QVector<QVector2D> result { triangles.at(indexFirstTriangle * 3), triangles.at(indexFirstTriangle * 3 + 1),
                                    triangles.at(indexFirstTriangle * 3 + 2) };
        FixWinding(result[0], result[1], result[2]);
        Q_ASSERT(orientation(result) == 2);

        qDebug() << "Cluster begin";
        for (auto item : cluster)
            qDebug() << item;

        for (auto it = cluster.begin(); it != cluster.end(); ++it) {
            // we already processed the first, skip it
            if (it == cluster.begin())
                continue;

            // copy the last result, since we need an empty result vector
            const auto poly = result;
            result.clear();

            const auto &currentIndex = *it;
            QVector<QVector2D> triangle { triangles.at(currentIndex * 3), triangles.at(currentIndex * 3 + 1),
                                          triangles.at(currentIndex * 3 + 2) };

            // we need a triangle in counter-clockwise orientation
            FixWinding(triangle[0], triangle[1], triangle[2]);

            qDebug() << "----------------------";

            qDebug() << "Poly" << poly;
            qDebug() << "Tria" << triangle;

            Q_ASSERT(orientation(poly) == 2);
            Q_ASSERT(orientation(triangle) == 2);

            Q_ASSERT(result.empty());

            concaveHull(poly, triangle, result);

            qDebug() << "Result size" << result.size();

            Q_ASSERT(result.size() >= 3);
            if (result.size() >= 3) {
                qDebug() << result;
                Q_ASSERT(orientation(result) == 2);
            }
            qDebug() << "=========================";
        }

        QVector<qreal> polygon;
        polygon.reserve(result.size() * 2);
        for (const auto &p : result) {
            polygon.append(p.x());
            polygon.append(p.y());
        }
        // close the polygon
        polygon.append(result.at(0).x());
        polygon.append(result.at(0).y());
        qDebug() << polygon;

        const auto &triangleList = qTriangulate(polygon.data(), polygon.size() / 2,
                                                QVectorPath::PolygonHint | QVectorPath::WindingFill | QVectorPath::LinesShapeMask);

        QVector<QVector2D> newTriangles;
        newTriangles.reserve(triangleList.indices.size());

        int index;
        for (int i = 0; i < triangleList.indices.size(); i++) {
            if (triangleList.indices.type() == QVertexIndexVector::UnsignedInt) {
                index = static_cast<const quint32 *>(triangleList.indices.data())[i];
            } else {
                index = static_cast<const quint16 *>(triangleList.indices.data())[i];
            }

            const qreal x = triangleList.vertices[2 * index];
            const qreal y = triangleList.vertices[2 * index + 1];

            newTriangles.append(QVector2D(x, y));
        }
        nonOverlappingTriangles += newTriangles;
    }

    for (size_t i = 0; i < triangles.size(); i += 3) {
        if (overlappingTrianglesIndizes.count(i / 3))
            continue;
        nonOverlappingTriangles.append(triangles.at(i));
        nonOverlappingTriangles.append(triangles.at(i + 1));
        nonOverlappingTriangles.append(triangles.at(i + 2));
    }

    nonOverlappingTriangles.shrink_to_fit();
    triangles = std::move(nonOverlappingTriangles);
    qDebug() << "Triangles" << triangles << "Triangles End";
}

QVector2D normalOf(const QVector2D &v)
{
    return QVector2D(-v.y(), v.x()).normalized();
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
                normal = normalOf(diff);
            } else {
                normal = normalOf(pathData.at(i).tangent);
            }
            if (t2.isNull()) {
                normal2 = normal;
            } else {
                normal2 = normalOf(t2);
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
                const auto &normal2 = normalOf(diff2);
                const auto &offset2 = normal2 * lineWidth / 2.0;
                const bool turnLeft = crossProduct(diff, diff2) > 0;

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

                            // FIXME:
                            // The Miter Join is calculated from the point difference, which is wrong in case of a bezier curve.
                            // We need to offset the direction at the last point and then calculate the intersection of that line with
                            // the other point's direction offset...

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

        qDebug() << lineDataSegment.size();
        removeOverlappingTriangles(lineDataSegment);
        qDebug() << lineDataSegment.size();

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
