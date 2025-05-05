// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "riveqsgsoftwarerendernode.h"
#include "riveqtquickitem.h"

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
    renderSoftware(state);
}

void RiveQSGSoftwareRenderNode::paint(QPainter *painter)
{
    if (!painter) {
        return;
    }

    if (m_artboardInstance.expired()) {
        return;
    }

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    auto artboardInstance = m_artboardInstance.lock();

    auto x = 0;
    auto y = 0;

    auto aspectX = painter->device()->width() / (artboardInstance->width());
    auto aspectY = painter->device()->height() / (artboardInstance->height());

    // Calculate the uniform scale factor to preserve the aspect ratio
    auto scaleFactor = qMin(aspectX, aspectY);

    // Calculate the new width and height of the item while preserving the aspect ratio
    auto newWidth = artboardInstance->width() * scaleFactor;
    auto newHeight = artboardInstance->height() * scaleFactor;

    // Calculate the offsets needed to center the item within its bounding rectangle
    auto offsetX = (painter->device()->width() - newWidth) / 2.0;
    auto offsetY = (painter->device()->height() - newHeight) / 2.0;

    // TODO this only works for PreserverAspectFit
    m_scaleFactorX = scaleFactor;
    m_scaleFactorY = scaleFactor;
    m_topLeftRivePosition.setX(offsetX);
    m_topLeftRivePosition.setY(offsetY);

    m_modelViewTransform = QTransform();
    // Apply transformations in the correct order
    m_modelViewTransform.translate(x, y);
    m_modelViewTransform.translate(offsetX, offsetY);
    m_modelViewTransform.scale(scaleFactor, scaleFactor);
#endif

    painter->save();
    {
        auto artboardInstance = m_artboardInstance.lock();

        painter->setTransform(m_modelViewTransform, false);

        m_renderer.setPainter(painter);

        painter->save();
        {
            if (artboardInstance) {
                artboardInstance->draw(&m_renderer);
            }
        }
        painter->restore();
    }
    painter->restore();
}

QTransform matrix4x4ToTransform(const QMatrix4x4 &matrix)
{
    return QTransform(matrix(0, 0), matrix(0, 1), matrix(0, 3), matrix(1, 0), matrix(1, 1), matrix(1, 3), matrix(3, 0), matrix(3, 1),
                      matrix(3, 3));
}

void RiveQSGSoftwareRenderNode::renderSoftware(const RenderState *state)
{

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
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

    //  Set the model-view matrix and apply the translation and scale
    m_matrix = *state->projectionMatrix();
    m_modelViewTransform = matrix4x4ToTransform(m_matrix);
    // Apply transformations in the correct order
    m_modelViewTransform.translate(x, y);
    m_modelViewTransform.translate(offsetX, offsetY);
    m_modelViewTransform.scale(scaleFactor, scaleFactor);

    QPainter *painter = nullptr;

    void *vPainter = renderInterface->getResource(m_window, QSGRendererInterface::Resource::PainterResource);
    if (vPainter) {
        painter = static_cast<QPainter *>(vPainter);
    } else {
        return;
    }
    paint(painter);
#endif
    // in qt5 paint will be called by paint method of paintedItem
}
