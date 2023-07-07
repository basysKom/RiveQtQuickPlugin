// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later
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
    m_subPaths = other.m_subPaths;
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
            m_isClosed = true;
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
    m_subPaths.clear();
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

void RiveQtPath::addRenderPath(RenderPath *path, const rive::Mat2D &m)
{
    if (!path) {
        qCDebug(rqqpRendering) << "Skip adding nullptr render path.";
        return;
    }

    RiveQtPath *qtPath = static_cast<RiveQtPath *>(path);
    QMatrix4x4 matrix(m[0], m[2], 0, m[4], m[1], m[3], 0, m[5], 0, 0, 1, 0, 0, 0, 0, 1);

    m_subPaths.emplace_back(SubPath(qtPath, matrix));

    m_pathSegmentOutlineDataDirty = true;
    m_pathSegmentDataDirty = true;

    // calculate vertices directly when we add a path
    generateVertices();
}

void RiveQtPath::setQPainterPath(QPainterPath path)
{
    m_path = path;
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

void RiveQtPath::addMiterJoin(QVector<QVector2D> &lineDataSegment, const QVector2D &p1, const QVector2D &p2, const QVector2D &offset)
{
    lineDataSegment.append(p1 + offset);
    lineDataSegment.append(p2 + offset);
    lineDataSegment.append(p1 - offset);
    lineDataSegment.append(p2 - offset);
};

void RiveQtPath::addRoundJoin(QVector<QVector2D> &lineDataSegment, const float &lineWidth, const QVector2D &p1, const QVector2D &p2,
                              const QVector2D &offset, int numSegments)
{
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

void RiveQtPath::addBevelJoin(QVector<QVector2D> &lineDataSegment, const QVector2D &p1, const QVector2D &p2, const QVector2D &offset)
{
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

void RiveQtPath::addCap(QVector<QVector2D> &lineDataSegment, const float &lineWidth, const Qt::PenCapStyle &capStyle, const QVector2D &p,
                        const QVector2D &offset, const QVector2D &normal, const bool &isStart)
{
    switch (capStyle) {
    case Qt::PenCapStyle::FlatCap:
        // No additional vertices needed for FlatCap
        break;
    case Qt::PenCapStyle::RoundCap: {
        int numSegments = m_segmentCount;
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

void fillBetweenRound(QVector<QVector2D> &output, const QVector2D &centerPoint, const QVector2D &start, const QVector2D &end,
                      const float width, const int segmentCount)
{
}

// TODO: Optimize, this gets called each frame for a stroke/path
// there will be shortcuts and caching possible
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

    qreal lineWidth = pen.widthF();

    const Qt::PenJoinStyle &joinType = pen.joinStyle();
    const Qt::PenCapStyle &capStyle = pen.capStyle();

    for (QVector<QVector2D> pathData : qAsConst(m_pathSegmentsOutlineData)) {
        if (pathData.size() <= 1) {
            continue;
        }

        QVector<QVector2D> lineDataSegment;

        const int endIndex = m_isClosed ? pathData.count() : pathData.count() - 1;

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

            if (!m_isClosed && (i == 0 || i == endIndex - 1)) {
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

                QVector2D p3;
                if (m_isClosed)
                    p3 = pathData[(i + 3) % pathData.count()];
                else
                    p3 = pathData[(i + 2) % pathData.count()];

                const auto diff2 = p3 - p2;
                const auto normal2 = QVector2D(-diff2.y(), diff2.x()).normalized();
                const auto offset2 = normal2 * lineWidth / 2.0;
                switch (joinType) {
                default:
                    // this should never be the case, since we handle all rive types.
                    // fallthrough intended
                case Qt::PenJoinStyle::RoundJoin: {
                    {
                        auto phi = acos(normal.x() * normal2.x() + normal.y() * normal2.y()) / m_segmentCount;

                        bool turnLeft = normal.x() * normal2.y() - normal.y() * normal2.x() > 0;
                        phi = turnLeft ? phi : -phi;
                        QVector2D currentOffset = turnLeft ? -offset : offset;

                        //                        qDebug() << phi;
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
                    }
                    break;
                }
                case Qt::PenJoinStyle::MiterJoin:
                    // Miter Join may not work so well, specifically on tesselated segmented bezier curves
                    // disable it for now,.. we may need to the join specific on the source (like curve)
                    // addMiterJoin(p1, p2, offset);
                    // break;
                    // the fallthrough is intendend here
                case Qt::PenJoinStyle::BevelJoin:
                    // we could potentially save 1 triangle, but it's not worth it atm.
                    lineDataSegment.append(p2 + offset);
                    lineDataSegment.append(p2);
                    lineDataSegment.append(p2 + offset2);

                    lineDataSegment.append(p2 - offset);
                    lineDataSegment.append(p2);
                    lineDataSegment.append(p2 - offset2);
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

QVector<QVector2D> RiveQtPath::qpainterPathToVector2D(const QPainterPath &path)
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

QVector<QVector2D> RiveQtPath::qpainterPathToOutlineVector2D(const QPainterPath &path)
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

    return pathData;
}

void RiveQtPath::generateVertices()
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

    for (SubPath &subPath : m_subPaths) {
        QPainterPath sourcePath = subPath.path()->toQPainterPath();
        QPainterPath transformedPath = RiveQtUtils::transformPathWithMatrix4x4(sourcePath, subPath.transform());

        m_pathSegmentsData.append(subPath.path()->m_pathSegmentsData);
        m_pathSegmentsData.append(qpainterPathToVector2D(transformedPath));
    }
    m_pathSegmentDataDirty = false;
}

void RiveQtPath::generateOutlineVertices()
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

    for (SubPath &subPath : m_subPaths) {
        QPainterPath sourcePath = subPath.path()->toQPainterPath();
        QPainterPath transformedPath = RiveQtUtils::transformPathWithMatrix4x4(sourcePath, subPath.transform());

        m_pathSegmentsOutlineData.append(subPath.path()->m_pathSegmentsOutlineData);
        m_pathSegmentsOutlineData.append(qpainterPathToOutlineVector2D(transformedPath));
    }
}

SubPath::SubPath(RiveQtPath *path, const QMatrix4x4 &transform)
    : m_Path(path)
    , m_Transform(transform)
{
}

RiveQtPath *SubPath::path() const
{
    return m_Path;
}
QMatrix4x4 SubPath::transform() const
{
    return m_Transform;
}
