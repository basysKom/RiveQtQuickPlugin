// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QPainterPath>
#include <QMatrix4x4>
#include <QPen>

#include <rive/renderer.hpp>
#include <rive/math/raw_path.hpp>

class RiveQtPath : public rive::RenderPath
{
public:
    RiveQtPath(const unsigned segmentCount);
    RiveQtPath(const RiveQtPath &other);
    RiveQtPath(const rive::RawPath &rawPath, rive::FillRule fillRule, const unsigned segmentCount);

    void rewind() override;
    void moveTo(float x, float y) override { m_qPainterPath.moveTo(x, y); }
    void lineTo(float x, float y) override { m_qPainterPath.lineTo(x, y); }
    void cubicTo(float ox, float oy, float ix, float iy, float x, float y) override { m_qPainterPath.cubicTo(ox, oy, ix, iy, x, y); }
    void close() override { m_qPainterPath.closeSubpath(); }
    void fillRule(rive::FillRule value) override;
    void addRenderPath(rive::RenderPath *path, const rive::Mat2D &transform) override;

    void setQPainterPath(QPainterPath path);
    QPainterPath toQPainterPath() const { return m_qPainterPath; }
    QPainterPath toQPainterPaths(const QMatrix4x4 &t);

    void setSegmentCount(const unsigned segmentCount);

    QVector<QVector<QVector2D>> toVertices();
    QVector<QVector<QVector2D>> toVerticesLine(const QPen &pen);

private:
    struct PathDataPoint
    {
        QVector2D point;
        QVector2D tangent; // only used in cubic bezier
    };

    static QPointF cubicBezier(const QPointF &startPoint, const QPointF &controlPoint1, const QPointF &controlPoint2,
                               const QPointF &endPoint, qreal t);

    void updatePathSegmentsData();
    void updatePathSegmentsOutlineData();
    void updatePathOutlineVertices(const QPen &pen);

    QPainterPath m_qPainterPath;
    QVector<QVector<PathDataPoint>> m_pathSegmentsOutlineData;

    QVector<QVector<QVector2D>> m_pathVertices;
    QVector<QVector<QVector2D>> m_pathOutlineVertices;

    bool m_pathSegmentDataDirty { true };
    bool m_pathSegmentOutlineDataDirty { true };
    unsigned m_segmentCount { 10 };
};
