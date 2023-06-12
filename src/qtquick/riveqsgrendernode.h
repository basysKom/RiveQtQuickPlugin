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

#include "rive/artboard.hpp"

class RiveQtQuickItem;

class RiveQSGBaseNode
{
public:
    RiveQSGBaseNode(rive::ArtboardInstance *artboardInstance, RiveQtQuickItem *item);

    virtual void renderOffscreen() { }
    virtual void setRect(const QRectF &bounds);

    virtual void updateArtboardInstance(rive::ArtboardInstance *artboardInstance) { m_artboardInstance = artboardInstance; }

protected:
    rive::ArtboardInstance *m_artboardInstance { nullptr };
    RiveQtQuickItem *m_item { nullptr };
    QRectF m_rect;
};

class RiveQSGRenderNode : public QSGRenderNode, public RiveQSGBaseNode
{
public:
    RiveQSGRenderNode(rive::ArtboardInstance *artboardInstance, RiveQtQuickItem *item);

    StateFlags changedStates() const override { return QSGRenderNode::BlendState; }
    RenderingFlags flags() const override { return QSGRenderNode::BoundedRectRendering | QSGRenderNode::DepthAwareRendering; }
    QRectF rect() const override;
};
