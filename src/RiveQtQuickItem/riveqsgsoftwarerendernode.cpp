
// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <QQuickWindow>

#include "riveqtquickitem.h"
#include "riveqsgsoftwarerendernode.h"

RiveQSGSoftwareRenderNode::RiveQSGSoftwareRenderNode(QQuickWindow *window, std::weak_ptr<rive::ArtboardInstance> artboardInstance,
                                                     RiveQtQuickItem *item)
    : RiveQSGRenderNode(artboardInstance, item)
    , m_window(window)
{
}

QRectF RiveQSGSoftwareRenderNode::rect() const
{
    if (!m_item) {
        return QRectF(0, 0, 0, 0);
    }
    return QRectF(0, 0, m_item->width(), m_item->height());
}

void RiveQSGSoftwareRenderNode::render(const RenderState *state)
{
    renderSoftware(state);
}

QTransform matrix4x4ToTransform(const QMatrix4x4 &matrix)
{
    return QTransform(matrix(0, 0), matrix(0, 1), matrix(0, 3), matrix(1, 0), matrix(1, 1), matrix(1, 3), matrix(3, 0), matrix(3, 1),
                      matrix(3, 3));
}

void RiveQSGSoftwareRenderNode::renderSoftware(const RenderState *state)
{
    if (m_artboardInstance.expired()) {
        return;
    }

    if (!m_item) {
        return;
    }

    auto artboardInstance = m_artboardInstance.lock();

    QPointF globalPos = m_item->mapToItem(nullptr, QPointF(0, 0));
    auto x = globalPos.x();
    auto y = globalPos.y();

    auto aspectX = m_item->width() / (artboardInstance->width());
    auto aspectY = m_item->height() / (artboardInstance->height());

    // Calculate the uniform scale factor to preserve the aspect ratio
    auto scaleFactor = qMin(aspectX, aspectY);

    // Calculate the new width and height of the item while preserving the aspect ratio
    auto newWidth = artboardInstance->width() * scaleFactor;
    auto newHeight = artboardInstance->height() * scaleFactor;

    // Calculate the offsets needed to center the item within its bounding rectangle
    auto offsetX = (m_item->width() - newWidth) / 2.0;
    auto offsetY = (m_item->height() - newHeight) / 2.0;

    // TODO this only works for PreserverAspectFit
    m_scaleFactorX = scaleFactor;
    m_scaleFactorY = scaleFactor;
    m_topLeftRivePosition.setX(offsetX);
    m_topLeftRivePosition.setY(offsetY);

    auto *renderInterface = m_window->rendererInterface();

    QPainter *painter = nullptr;
    if (renderInterface->graphicsApi() == QSGRendererInterface::GraphicsApi::Software) {
        void *vPainter = renderInterface->getResource(m_window, QSGRendererInterface::Resource::PainterResource);
        if (vPainter) {
            painter = static_cast<QPainter *>(vPainter);
        } else {
            return;
        }
    } else {
        return;
    }

    if (m_item) {
        painter->save();
        {
            //  Set the model-view matrix and apply the translation and scale
            QMatrix4x4 matrix = *state->projectionMatrix();
            QTransform modelViewTransform = matrix4x4ToTransform(matrix);

            // Apply transformations in the correct order
            modelViewTransform.translate(x, y);
            modelViewTransform.translate(offsetX, offsetY);
            modelViewTransform.scale(scaleFactor, scaleFactor);

            painter->setTransform(modelViewTransform, false);
        }

        m_renderer.setPainter(painter);

        painter->save();
        {
            artboardInstance->draw(&m_renderer);
        }
        painter->restore();
    }
    painter->restore();
}
