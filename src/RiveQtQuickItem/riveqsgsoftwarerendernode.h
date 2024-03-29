
// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QElapsedTimer>
#include <QQuickItem>
#include <QQuickPaintedItem>
#include <QSGRenderNode>
#include <QSGTextureProvider>

#include <rive/artboard.hpp>

#include "riveqsgrendernode.h"
#include "renderer/riveqtpainterrenderer.h"

class RiveQtQuickItem;
class QQuickWindow;

class RiveQSGSoftwareRenderNode : public RiveQSGRenderNode
{
public:
    RiveQSGSoftwareRenderNode(QQuickWindow *window, std::weak_ptr<rive::ArtboardInstance> artboardInstance, const QRectF &geometry);

    QRectF rect() const override;

    StateFlags changedStates() const override { return QSGRenderNode::BlendState; }
    RenderingFlags flags() const override { return QSGRenderNode::BoundedRectRendering; }

    void render(const RenderState *state) override;

    void paint(QPainter *painter);

private:
    void renderSoftware(const RenderState *state);

    RiveQtPainterRenderer m_renderer;

    QPainter m_fallbackPainter;
    QPixmap m_fallbackPixmap;

    QMatrix4x4 m_matrix;
    QTransform m_modelViewTransform;
};
