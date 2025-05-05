// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "riveqsgsoftwarerendernode.h"

#include <QQuickWindow>

RiveQSGSoftwareRenderNode::RiveQSGSoftwareRenderNode(QQuickWindow *window, std::weak_ptr<rive::ArtboardInstance> artboardInstance,
                                                     const QRectF &geometry)
    : RiveQSGRenderNode(window, artboardInstance, geometry)
{
}

QRectF RiveQSGSoftwareRenderNode::rect() const
{
    return QRectF(0, 0, m_rect.width(), m_rect.height());
}

QSGRenderNode::RenderingFlags RiveQSGSoftwareRenderNode::flags() const
{
    return QSGRenderNode::BoundedRectRendering;
}

QSGRenderNode::StateFlags RiveQSGSoftwareRenderNode::changedStates() const
{
    return QSGRenderNode::BlendState;
}

void RiveQSGSoftwareRenderNode::render(const RenderState *state)
{
    if (!m_window) {
        return;
    }

    auto *renderInterface = m_window->rendererInterface();
    if (renderInterface->graphicsApi() != QSGRendererInterface::GraphicsApi::Software) {
        return;
    }

    QPainter *painter = nullptr;

    void *vPainter = renderInterface->getResource(m_window, QSGRendererInterface::Resource::PainterResource);
    if (vPainter) {
        painter = static_cast<QPainter *>(vPainter);
    } else {
        return;
    }

    paint(painter, state->clipRegion()->boundingRect(), state->projectionMatrix()->toTransform());
}

void RiveQSGSoftwareRenderNode::paint(QPainter *painter, const QRect &bounds, const QTransform &transform)
{
    if (m_artboardInstance.expired()) {
        return;
    }

    const auto artboardInstance = m_artboardInstance.lock();
    const QSizeF artboardSize(artboardInstance->width(), artboardInstance->height());
    const auto aspectX = bounds.width() / artboardSize.width();
    const auto aspectY = bounds.height() / artboardSize.height();
    // Calculate the uniform scale factor to preserve the aspect ratio
    const auto scaleFactor = qMin(aspectX, aspectY);
    // Calculate the new width and height of the item while preserving the aspect ratio
    const QSizeF newSize = artboardSize * scaleFactor;
    // Calculate the offsets needed to center the item within its bounding rectangle
    const auto offsetX = (bounds.width() - newSize.width()) / 2.0;
    const auto offsetY = (bounds.height() - newSize.height()) / 2.0;

    // TODO this only works for PreserveAspectFit
    m_scaleFactorX = scaleFactor;
    m_scaleFactorY = scaleFactor;
    m_topLeftRivePosition.setX(offsetX);
    m_topLeftRivePosition.setY(offsetY);

    QTransform transformation = transform;

    transformation.translate(bounds.x() + offsetX, bounds.y() + offsetY);
    transformation.scale(scaleFactor, scaleFactor);

    painter->save();
    {
        painter->setTransform(transformation, false);

        m_renderer.setPainter(painter);
        artboardInstance->draw(&m_renderer);
    }
    painter->restore();
}
