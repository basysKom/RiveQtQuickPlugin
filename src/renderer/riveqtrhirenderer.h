
// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QPainterPath>
#include <QQuickItem>
#include <QBrush>
#include <QPen>
#include <QLinearGradient>

#include "private/qrhi_p.h"

#include <rive/renderer.hpp>
#include <rive/math/raw_path.hpp>

#include "src/qtquick/datatypes.h"

class RhiSubPath;
class QSGRenderNode;
class TextureTargetNode;

class RiveQtRhiGLPath : public rive::RenderPath
{
public:
    RiveQtRhiGLPath(const unsigned segmentCount);
    RiveQtRhiGLPath(const RiveQtRhiGLPath &other);
    RiveQtRhiGLPath(rive::RawPath &rawPath, rive::FillRule fillRule, const unsigned segmentCount);

    void rewind() override;
    void moveTo(float x, float y) override { m_path.moveTo(x, y); }
    void lineTo(float x, float y) override { m_path.lineTo(x, y); }
    void cubicTo(float ox, float oy, float ix, float iy, float x, float y) override { m_path.cubicTo(ox, oy, ix, iy, x, y); }
    void close() override { m_path.closeSubpath(); }
    void fillRule(rive::FillRule value) override;
    void addRenderPath(RenderPath *path, const rive::Mat2D &transform) override;

    void setQPainterPath(QPainterPath path);
    QPainterPath toQPainterPath() const { return m_path; }
    QPainterPath toQPainterPaths(const QMatrix4x4 &t);

    void setSegmentCount(const unsigned segmentCount);

    QVector<QVector<QVector2D>> toVertices();
    QVector<QVector<QVector2D>> toVerticesLine(const QPen &pen);

private:
    void generateVertices();
    void generateOutlineVertices();

    QVector<QVector2D> qpainterPathToVector2D(const QPainterPath &path);
    QVector<QVector2D> qpainterPathToOutlineVector2D(const QPainterPath &path);
    QPointF cubicBezier(const QPointF &startPoint, const QPointF &controlPoint1, const QPointF &controlPoint2, const QPointF &endPoint,
                        qreal t);

    QPainterPath m_path;
    std::vector<RhiSubPath> m_subPaths;
    QVector<QVector<QVector2D>> m_pathSegmentsData;
    QVector<QVector<QVector2D>> m_pathSegmentsOutlineData;
    QVector<QVector<QVector2D>> m_pathOutlineData;

    bool m_isClosed { false };
    bool m_pathSegmentDataDirty { true };
    bool m_pathSegmentOutlineDataDirty { true };

    unsigned m_segmentCount { 10 };
};

class RhiSubPath
{
public:
    RhiSubPath(RiveQtRhiGLPath *path, const QMatrix4x4 &transform);

    RiveQtRhiGLPath *path() const;
    QMatrix4x4 transform() const;

private:
    RiveQtRhiGLPath *m_Path;
    QMatrix4x4 m_Transform;
};

struct RhiRenderState
{
    QMatrix4x4 transform;
    float opacity { 1.0 };
    RiveQtRhiGLPath clipPath { 10u };
    QVector<QVector<QVector2D>> clippingGeometry;
    QVector<TextureTargetNode *> stackNodes;
};

class RiveQtRhiRenderer : public rive::Renderer
{
public:
    RiveQtRhiRenderer(QQuickItem *item);
    virtual ~RiveQtRhiRenderer();

    void setFillMode(const RenderSettings::FillMode fillMode) { m_fillMode = fillMode; };

    void save() override;
    void restore() override;
    void transform(const rive::Mat2D &transform) override;
    void drawPath(rive::RenderPath *path, rive::RenderPaint *paint) override;
    void clipPath(rive::RenderPath *path) override;
    void drawImage(const rive::RenderImage *image, rive::BlendMode blendMode, float opacity) override;
    void drawImageMesh(const rive::RenderImage *image, rive::rcp<rive::RenderBuffer> vertices_f32,
                       rive::rcp<rive::RenderBuffer> uvCoords_f32, rive::rcp<rive::RenderBuffer> indices_u16, rive::BlendMode blendMode,
                       float opacity) override;

    void setProjectionMatrix(const QMatrix4x4 *projectionMatrix, const QMatrix4x4 *modelMatrix);
    void updateArtboardSize(const QSize &artboardSize) { m_artboardSize = artboardSize; }
    void updateViewPort(const QRectF &viewportRect, QRhiTexture *displayBuffer);
    void recycleRiveNodes();

    void render(QRhiCommandBuffer *cb);

private:
    TextureTargetNode *getRiveDrawTargetNode();

    const QMatrix4x4 &transformMatrix() const;
    float currentOpacity();

    QQuickItem *m_item;

    QVector<RhiRenderState> m_rhiRenderStack;
    QVector<TextureTargetNode *> m_renderNodes;

    QRhiTexture *m_displayBuffer;

    QMatrix4x4 m_projectionMatrix;
    QMatrix4x4 m_modelMatrix;
    QMatrix4x4 m_combinedMatrix;

    QSize m_artboardSize;
    QRectF m_viewportRect;

    RenderSettings::FillMode m_fillMode;
};
