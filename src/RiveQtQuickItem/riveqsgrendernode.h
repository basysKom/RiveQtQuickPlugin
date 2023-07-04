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

class RiveQtQuickItem;

class RiveQSGBaseNode
{
public:
    RiveQSGBaseNode(std::weak_ptr<rive::ArtboardInstance> artboardInstance, RiveQtQuickItem *item);

    virtual void renderOffscreen() { }
    virtual void setRect(const QRectF &bounds);
    virtual QPointF topLeft() const;
    virtual float scaleFactorX() const;
    virtual float scaleFactorY() const;

    virtual void updateArtboardInstance(std::weak_ptr<rive::ArtboardInstance> artboardInstance) { m_artboardInstance = artboardInstance; }

    virtual void setArtboardRect(const QRectF &bounds);

protected:
    std::weak_ptr<rive::ArtboardInstance> m_artboardInstance;
    RiveQtQuickItem *m_item { nullptr };
    QRectF m_rect;
    QPointF m_topLeftRivePosition { 0.f, 0.f };
    QSizeF m_riveSize { 0.f, 0.f };

    float m_scaleFactorX { 1.0f };
    float m_scaleFactorY { 1.0f };
};

class RiveQSGRenderNode : public QSGRenderNode, public RiveQSGBaseNode
{
public:
    RiveQSGRenderNode(std::weak_ptr<rive::ArtboardInstance> artboardInstance, RiveQtQuickItem *item);

    StateFlags changedStates() const override { return QSGRenderNode::BlendState; }
    RenderingFlags flags() const override { return QSGRenderNode::BoundedRectRendering | QSGRenderNode::DepthAwareRendering; }
    QRectF rect() const override;
};
