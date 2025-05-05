// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QPainterPath>
#include <QMatrix4x4>

#include <rive/renderer.hpp>
#include <rive/math/raw_path.hpp>

class QRhiCommandBuffer;
class QSGRenderNode;
class QQuickWindow;
class RhiSubPath;
class TextureTargetNode;
class RiveQSGRHIRenderNode;

struct RhiRenderState
{
    QMatrix4x4 transform;
    float opacity { 1.0 };

    QVector<QPair<QPainterPath, QMatrix4x4>> m_allClipPainterPathesApplied;
};

class RiveQtRhiRenderer : public rive::Renderer
{
public:
    RiveQtRhiRenderer(QQuickWindow *window, RiveQSGRHIRenderNode *node);
    virtual ~RiveQtRhiRenderer();

    void save() override;
    void restore() override;
    void transform(const rive::Mat2D &transform) override;
    void drawPath(rive::RenderPath *path, rive::RenderPaint *paint) override;
    void clipPath(rive::RenderPath *path) override;
    void drawImage(const rive::RenderImage *image, rive::BlendMode blendMode, float opacity) override;
    void drawImageMesh(const rive::RenderImage *image, rive::rcp<rive::RenderBuffer> vertices_f32,
                       rive::rcp<rive::RenderBuffer> uvCoords_f32, rive::rcp<rive::RenderBuffer> indices_u16, uint32_t vertexCount,
                       uint32_t indexCount, rive::BlendMode blendMode, float opacity) override;

    void setProjectionMatrix(const QMatrix4x4 *projectionMatrix, const QMatrix4x4 *combinedMatrix);
    void updateArtboardSize(const QSize &artboardSize) { m_artboardSize = artboardSize; }
    void updateViewPort(const QRectF &viewportRect);
    void recycleRiveNodes();
    void setRiveRect(const QRectF &bounds);

    void render(QRhiCommandBuffer *cb) const;

private:
    TextureTargetNode *getRiveDrawTargetNode();

    const QMatrix4x4 &transformMatrix() const;
    float currentOpacity();

    QVector<RhiRenderState> m_rhiRenderStack;
    QVector<TextureTargetNode *> m_renderNodes;

    QQuickWindow *m_window;

    QMatrix4x4 m_projectionMatrix;
    QMatrix4x4 m_combinedMatrix;

    QSize m_artboardSize;
    QRectF m_viewportRect;
    QRectF m_riveRect;

    RiveQSGRHIRenderNode *m_node;
};
