// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#define USE_QPAINTERPATH_STROKER

#include <QPainterPath>
#include <QMatrix4x4>
#include <QPen>

#include <rive/renderer.hpp>
#include <rive/math/raw_path.hpp>

class RiveQtPath : public rive::RenderPath
{
public:
    RiveQtPath();
    RiveQtPath(const RiveQtPath &other);
    RiveQtPath(const rive::RawPath &rawPath, rive::FillRule fillRule);

    void rewind() override;
    void moveTo(float x, float y) override { m_qPainterPath.moveTo(x, y); }
    void lineTo(float x, float y) override { m_qPainterPath.lineTo(x, y); }
    void cubicTo(float ox, float oy, float ix, float iy, float x, float y) override { m_qPainterPath.cubicTo(ox, oy, ix, iy, x, y); }
    void close() override { m_qPainterPath.closeSubpath(); }
    void fillRule(rive::FillRule value) override;
    void addRenderPath(rive::RenderPath *path, const rive::Mat2D &transform) override;

    void setQPainterPath(QPainterPath path);
    void intersectWith(const QPainterPath &other);
    QPainterPath toQPainterPath() const { return m_qPainterPath; }
    QPainterPath toQPainterPaths(const QMatrix4x4 &t);

    void setLevelOfDetail(const unsigned lod);

    QVector<QVector<QVector2D>> toVertices();
    QVector<QVector<QVector2D>> toVerticesLine(const QPen &pen);

private:

#if !defined(USE_QPAINTERPATH_STROKER)
    struct PathDataPoint
    {
        QVector2D point;
        QVector2D tangent; // only used in cubic bezier
        int stepIndex;
    };

    static QPointF cubicBezier(const QPointF &startPoint, const QPointF &controlPoint1, const QPointF &controlPoint2,
                               const QPointF &endPoint, qreal t);

    void updatePathSegmentsOutlineData();
    unsigned m_segmentCount { 10 };
    QVector<QVector<PathDataPoint>> m_pathSegmentsOutlineData;
#endif


    void updatePathSegmentsData();
    void updatePathOutlineVertices(const QPen &pen);

    QPainterPath m_qPainterPath;

    QVector<QVector<QVector2D>> m_pathVertices;
    QVector<QVector<QVector2D>> m_pathOutlineVertices;

    bool m_pathSegmentDataDirty { true };
    bool m_pathSegmentOutlineDataDirty { true };
    unsigned m_lod { 1 };

};
