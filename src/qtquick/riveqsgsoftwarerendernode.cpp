
// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <QQuickWindow>

#include "riveqsgsoftwarerendernode.h"
#include "src/qtquick/riveqtquickitem.h"

RiveQSGSoftwareRenderNode::RiveQSGSoftwareRenderNode(QQuickWindow *window, rive::ArtboardInstance *artboardInstance, RiveQtQuickItem *item)
    : RiveQSGRenderNode(artboardInstance, item)
    , m_window(window)
{
}

QRectF RiveQSGSoftwareRenderNode::rect() const
{
    if (!m_item) {
        return QRectF(0, 0, 0, 0);
    }
    return QRectF(m_item->x(), m_item->y(), m_item->width(), m_item->height());
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
    if (m_artboardInstance && m_item) {
        QPointF globalPos = m_item->mapToItem(nullptr, QPointF(0, 0));
        auto x = globalPos.x();
        auto y = globalPos.y();

        auto aspectX = m_item->width() / (m_artboardInstance->width());
        auto aspectY = m_item->height() / (m_artboardInstance->height());

        // Calculate the uniform scale factor to preserve the aspect ratio
        auto scaleFactor = qMin(aspectX, aspectY);

        // Calculate the new width and height of the item while preserving the aspect ratio
        auto newWidth = m_artboardInstance->width() * scaleFactor;
        auto newHeight = m_artboardInstance->height() * scaleFactor;

        // Calculate the offsets needed to center the item within its bounding rectangle
        auto offsetX = (m_item->width() - newWidth) / 2.0;
        auto offsetY = (m_item->height() - newHeight) / 2.0;

        void *vPainter = m_window->rendererInterface()->getResource(m_window, QSGRendererInterface::Resource::PainterResource);

        if (!m_artboardInstance || !vPainter) {
            return;
        }
        auto *painter = static_cast<QPainter *>(vPainter);

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
                m_artboardInstance->draw(&m_renderer);
            }
            painter->restore();
        }
        painter->restore();
    }
}
