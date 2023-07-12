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

#include "rqqplogging.h"
#include "riveqtpath.h"
#include "riveqtutils.h"

RiveQtPath::RiveQtPath(const unsigned segmentCount)
{
    m_path.setFillRule(Qt::FillRule::WindingFill);
    setSegmentCount(segmentCount);
    m_pathSegmentOutlineDataDirty = true;
    m_pathSegmentDataDirty = true;
}

RiveQtPath::RiveQtPath(const RiveQtPath &other)
{
    m_path = other.m_path;
    m_pathSegmentsData = other.m_pathSegmentsData;
    m_pathSegmentsOutlineData = other.m_pathSegmentsOutlineData;
    m_pathOutlineData = other.m_pathOutlineData;
    m_segmentCount = other.m_segmentCount;
}

RiveQtPath::RiveQtPath(const rive::RawPath &rawPath, rive::FillRule fillRule, const unsigned segmentCount)
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
            break;
        default:
            break;
        }
    }
    m_pathSegmentOutlineDataDirty = true;
    m_pathSegmentDataDirty = true;
}

void RiveQtPath::rewind()
{
    m_pathSegmentsData.clear();
    m_pathSegmentsOutlineData.clear();
    m_path.clear();
    m_pathSegmentOutlineDataDirty = true;
    m_pathSegmentDataDirty = true;
}

void RiveQtPath::fillRule(rive::FillRule value)
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

void RiveQtPath::addRenderPath(RenderPath *path, const rive::Mat2D &transform)
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

    // calculate vertices directly when we add a path
    generateVertices();
}

void RiveQtPath::setQPainterPath(QPainterPath path)
{
    m_path = path;
}

std::optional<QVector2D> calculateIntersection(const QVector2D &p1, const QVector2D &p2, const QVector2D &p3, const QVector2D &p4)
{
    const float x1 = p1.x(), y1 = p1.y();
    const float x2 = p2.x(), y2 = p2.y();
    const float x3 = p3.x(), y3 = p3.y();
    const float x4 = p4.x(), y4 = p4.y();

    float denominator = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);

    // If the lines are parallel or coincident, return an invalid point
    if (denominator == 0) {
        return {};
    }

    const float intersectX = ((x1 * y2 - y1 * x2) * (x3 - x4) - (x1 - x2) * (x3 * y4 - y3 * x4)) / denominator;
    const float intersectY = ((x1 * y2 - y1 * x2) * (y3 - y4) - (y1 - y2) * (x3 * y4 - y3 * x4)) / denominator;
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
        generateVertices();
    }
    return m_pathSegmentsData;
}

QVector<QVector<QVector2D>> RiveQtPath::toVerticesLine(const QPen &pen)
{
    if (m_pathSegmentOutlineDataDirty) {
        generateOutlineVertices();
        m_pathOutlineData.clear();
    }

    if (m_pathSegmentsOutlineData.isEmpty()) {
        return m_pathOutlineData;
    }

    if (!m_pathOutlineData.isEmpty()) {
        return m_pathOutlineData;
    }

    const qreal lineWidth = pen.widthF();
    const Qt::PenJoinStyle &joinType = pen.joinStyle();
    const Qt::PenCapStyle &capStyle = pen.capStyle();

    for (QVector<QVector2D> pathData : qAsConst(m_pathSegmentsOutlineData)) {
        if (pathData.size() <= 1) {
            continue;
        }

        QVector<QVector2D> lineDataSegment;

        bool closed = false;

        if (pathData.first() == pathData.last()) {
            closed = true;
        }

        const int endIndex = closed ? pathData.count() : pathData.count() - 1;

        for (int i = 0; i < endIndex; ++i) {
            const QVector2D &p1 = pathData[i];
            const QVector2D &p2 = pathData[(i + 1) % (pathData.count())]; // if endIndex, take 0
            const QVector2D diff = p2 - p1;

            const QVector2D normal = QVector2D(-diff.y(), diff.x()).normalized();

            const QVector2D offset = normal * (lineWidth / 2.0);

            // add the line segment
            lineDataSegment.append(p1 + offset);
            lineDataSegment.append(p1 - offset);
            lineDataSegment.append(p2 + offset);

            lineDataSegment.append(p2 + offset);
            lineDataSegment.append(p2 - offset);
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

                    QVector2D currentOffset = offset;
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
                    const auto direction = diff.normalized();
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

                QVector2D p3 = pathData[(i + 2) % pathData.count()];

                if (closed && (i + 2) == pathData.count())
                    p3 = pathData[(i + 3) % pathData.count()];

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

        m_pathOutlineData.append(lineDataSegment);
    }

    m_pathSegmentOutlineDataDirty = false;

    return m_pathOutlineData;
}

QPointF RiveQtPath::cubicBezier(const QPointF &startPoint, const QPointF &controlPoint1, const QPointF &controlPoint2,
                                const QPointF &endPoint, qreal t) const
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

QVector<QVector2D> RiveQtPath::qpainterPathToVector2D(const QPainterPath &path) const
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

void RiveQtPath::updatePathSegmentsOutlineData()
{
    QVector<QVector2D> pathData;
    pathData.reserve(m_path.elementCount());

    if (m_path.isEmpty()) {
        m_pathSegmentsOutlineData.clear();
        return;
    }

    const QPointF &point = m_path.elementAt(0);
    const QVector2D &centerPoint = QVector2D(point.x(), point.y());

    // Add the current point
    pathData.append(centerPoint);

    for (int i = 1; i < m_path.elementCount(); ++i) {
        QPainterPath::Element element = m_path.elementAt(i);

        switch (element.type) {
        case QPainterPath::MoveToElement:
            m_pathSegmentsOutlineData.append(pathData);
            pathData.clear(); // start a new vector
            pathData.append(QVector2D(element.x, element.y));
            break;

        case QPainterPath::LineToElement:
            pathData.append(QVector2D(element.x, element.y));
            break;

        case QPainterPath::CurveToElement: {
            const QPointF &startPoint = pathData.last().toPointF();
            const QPointF &controlPoint1 = element;
            const QPointF &controlPoint2 = m_path.elementAt(i + 1);
            const QPointF &endPoint = m_path.elementAt(i + 2);

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

    m_pathSegmentsOutlineData.append(pathData);
}

void RiveQtPath::generateVertices()
{
    if (!m_pathSegmentDataDirty) {
        return;
    }

    if (m_path.isEmpty()) {
        m_pathSegmentsData.clear();
        return;
    }

    m_pathSegmentsData.clear();
    m_pathSegmentsData.append(qpainterPathToVector2D(m_path));

    m_pathSegmentDataDirty = false;
}

void RiveQtPath::generateOutlineVertices()
{
    if (!m_pathSegmentOutlineDataDirty) {
        return;
    }

    if (m_path.isEmpty()) {
        m_pathSegmentsOutlineData.clear();
        return;
    }

    m_pathSegmentsOutlineData.clear();
    updatePathSegmentsOutlineData();
}
