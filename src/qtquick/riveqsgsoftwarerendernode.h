
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
#include "riveqtpainterrenderer.h"

class RiveQtQuickItem;
class QQuickWindow;

class RiveQSGSoftwareRenderNode : public RiveQSGRenderNode
{
public:
    RiveQSGSoftwareRenderNode(QQuickWindow *window, rive::ArtboardInstance *artboardInstance, RiveQtQuickItem *item);

    QRectF rect() const override;

    StateFlags changedStates() const override { return QSGRenderNode::BlendState; }
    RenderingFlags flags() const override { return QSGRenderNode::BoundedRectRendering; }

    void render(const RenderState *state) override;

private:
    void renderSoftware(const RenderState *state);

    RiveQtPainterRenderer m_renderer;
    QQuickWindow *m_window { nullptr };

    QPainter m_fallbackPainter;
    QPixmap m_fallbackPixmap;
};
