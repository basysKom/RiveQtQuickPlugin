
// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <QQuickWindow>

#include "riveqtquickitem.h"
#include "riveqsgsoftwarerendernode.h"

RiveQSGSoftwareRenderNode::RiveQSGSoftwareRenderNode(QQuickWindow *window, std::weak_ptr<rive::ArtboardInstance> artboardInstance,
                                                     const QRectF &geometry)
    : RiveQSGRenderNode(window, artboardInstance, geometry)
{
}

QRectF RiveQSGSoftwareRenderNode::rect() const
{
    return QRectF(0, 0, m_rect.width(), m_rect.height());
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

    if (!m_window) {
        return;
    }

    auto *renderInterface = m_window->rendererInterface();
    if (renderInterface->graphicsApi() != QSGRendererInterface::GraphicsApi::Software) {
        return;
    }

    auto artboardInstance = m_artboardInstance.lock();

    const QRect &boundingRect = state->clipRegion()->boundingRect();
    auto x = boundingRect.x();
    auto y = boundingRect.y();

    auto aspectX = boundingRect.width() / (artboardInstance->width());
    auto aspectY = boundingRect.height() / (artboardInstance->height());

    // Calculate the uniform scale factor to preserve the aspect ratio
    auto scaleFactor = qMin(aspectX, aspectY);

    // Calculate the new width and height of the item while preserving the aspect ratio
    auto newWidth = artboardInstance->width() * scaleFactor;
    auto newHeight = artboardInstance->height() * scaleFactor;

    // Calculate the offsets needed to center the item within its bounding rectangle
    auto offsetX = (boundingRect.width() - newWidth) / 2.0;
    auto offsetY = (boundingRect.height() - newHeight) / 2.0;

    // TODO this only works for PreserverAspectFit
    m_scaleFactorX = scaleFactor;
    m_scaleFactorY = scaleFactor;
    m_topLeftRivePosition.setX(offsetX);
    m_topLeftRivePosition.setY(offsetY);

    QPainter *painter = nullptr;

    void *vPainter = renderInterface->getResource(m_window, QSGRendererInterface::Resource::PainterResource);
    if (vPainter) {
        painter = static_cast<QPainter *>(vPainter);
    } else {
        return;
    }

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

        m_renderer.setPainter(painter);

        painter->save();
        {
            artboardInstance->draw(&m_renderer);
        }
        painter->restore();
    }
    painter->restore();
}
