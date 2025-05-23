// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "riveqsgrendernode.h"
#include "riveqsgrhirendernode.h"
#include "riveqsgsoftwarerendernode.h"

#include <QQuickWindow>

RiveQSGBaseNode::RiveQSGBaseNode(QQuickWindow *window, std::weak_ptr<rive::ArtboardInstance> artboardInstance, const QRectF &geometry)
    : m_artboardInstance(artboardInstance)
    , m_window(window)
{
    Q_ASSERT(m_window);
}

void RiveQSGBaseNode::setRect(const QRectF &bounds)
{
    m_rect = bounds;
}

QPointF RiveQSGBaseNode::topLeft() const
{
    return m_topLeftRivePosition;
}

float RiveQSGBaseNode::scaleFactorX() const
{
    return m_scaleFactorX;
}

float RiveQSGBaseNode::scaleFactorY() const
{
    return m_scaleFactorY;
}

void RiveQSGBaseNode::updateArtboardInstance(std::weak_ptr<rive::ArtboardInstance> artboardInstance)
{
    m_artboardInstance = artboardInstance;
}

void RiveQSGBaseNode::setArtboardRect(const QRectF &bounds)
{
    m_topLeftRivePosition = bounds.topLeft();
    m_riveSize = bounds.size();

    if (auto artboardInstance = m_artboardInstance.lock(); artboardInstance) {
        m_scaleFactorX = bounds.width() / artboardInstance->width();
        m_scaleFactorY = bounds.height() / artboardInstance->height();
    }
}

RiveQSGRenderNode::RiveQSGRenderNode(QQuickWindow *window, std::weak_ptr<rive::ArtboardInstance> artboardInstance, const QRectF &geometry)
    : RiveQSGBaseNode(window, artboardInstance, geometry)
{
}

QSGRenderNode::RenderingFlags RiveQSGRenderNode::flags() const
{
    return QSGRenderNode::BoundedRectRendering | QSGRenderNode::DepthAwareRendering;
}

QSGRenderNode::StateFlags RiveQSGRenderNode::changedStates() const
{
    return QSGRenderNode::BlendState;
}

QRectF RiveQSGRenderNode::rect() const
{
    return m_rect;
}
