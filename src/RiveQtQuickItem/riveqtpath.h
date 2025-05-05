// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#define USE_QPAINTERPATH_STROKER

#include <rive/renderer.hpp>
#include <rive/math/raw_path.hpp>

#include <QPainterPath>
#include <QMatrix4x4>
#include <QVector2D>
#include <QPen>

class RiveQtPath : public rive::RenderPath
{
public:
    RiveQtPath();
    RiveQtPath(const RiveQtPath &other);
    RiveQtPath(const rive::RawPath &rawPath, rive::FillRule fillRule);

    void rewind() override;
    void moveTo(float x, float y) override;
    void lineTo(float x, float y) override;
    void cubicTo(float ox, float oy, float ix, float iy, float x, float y) override;
    void close() override;
    void fillRule(rive::FillRule value) override;
    void addRenderPath(rive::RenderPath *path, const rive::Mat2D &transform) override;
    void addRawPath(const rive::RawPath &path) override;

    void setQPainterPath(const QPainterPath &path);
    bool intersectWith(const QPainterPath &other);
    QPainterPath toQPainterPath() const;

    void setLevelOfDetail(const unsigned lod);

    QVector<QVector<QVector2D>> toVertices();
    QVector<QVector<QVector2D>> toVerticesLine(const QPen &pen);

    void applyMatrix(const QMatrix4x4 &matrix);

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
