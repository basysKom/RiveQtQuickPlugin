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

#include <private/qrhi_p.h>

#include <rive/renderer.hpp>
#include <rive/math/raw_path.hpp>

#include "datatypes.h"
#include "riveqtpath.h"

class RhiSubPath;
class QSGRenderNode;
class TextureTargetNode;

struct RhiRenderState
{
    QMatrix4x4 transform;
    float opacity { 1.0 };
    RiveQtPath clipPath { };
    QVector<QVector<QVector2D>> clippingGeometry;
    QVector<TextureTargetNode *> stackNodes;
};

class RiveQtRhiRenderer : public rive::Renderer
{
public:
    RiveQtRhiRenderer(QQuickWindow *window);
    virtual ~RiveQtRhiRenderer();
    void setRiveRect(const QRectF &bounds) { m_riveRect = bounds; }

    void save() override;
    void restore() override;
    void transform(const rive::Mat2D &transform) override;
    void drawPath(rive::RenderPath *path, rive::RenderPaint *paint) override;
    void clipPath(rive::RenderPath *path) override;
    void drawImage(const rive::RenderImage *image, rive::BlendMode blendMode, float opacity) override;
    void drawImageMesh(const rive::RenderImage *image, rive::rcp<rive::RenderBuffer> vertices_f32,
                       rive::rcp<rive::RenderBuffer> uvCoords_f32, rive::rcp<rive::RenderBuffer> indices_u16, rive::BlendMode blendMode,
                       float opacity) override;

    void setProjectionMatrix(const QMatrix4x4 *projectionMatrix, const QMatrix4x4 *combinedMatrix);
    void updateArtboardSize(const QSize &artboardSize) { m_artboardSize = artboardSize; }
    void updateViewPort(const QRectF &viewportRect, QRhiTexture *displayBuffer);
    void recycleRiveNodes();

    void render(QRhiCommandBuffer *cb);

private:
    TextureTargetNode *getRiveDrawTargetNode();

    const QMatrix4x4 &transformMatrix() const;
    float currentOpacity();

    QVector<RhiRenderState> m_rhiRenderStack;
    QVector<TextureTargetNode *> m_renderNodes;

    QQuickWindow *m_window;
    QRhiTexture *m_displayBuffer;

    QMatrix4x4 m_projectionMatrix;
    QMatrix4x4 m_combinedMatrix;

    QSize m_artboardSize;
    QRectF m_viewportRect;
    QRectF m_riveRect;
};
