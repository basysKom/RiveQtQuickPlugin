
// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <QQuickWindow>

#include "rqqplogging.h"
#include "riveqsgopenglrendernode.h"
#include "riveqtquickitem.h"

void RiveQSGOpenGLRenderNode::updateArtboardInstance(std::weak_ptr<rive::ArtboardInstance> artboardInstance)
{
    m_renderer.reset();
    m_artboardInstance = artboardInstance;
}

void RiveQSGOpenGLRenderNode::setArtboardRect(const QRectF &bounds)
{
    RiveQSGBaseNode::setArtboardRect(bounds);
    m_scaleFactorX *= m_item->window()->devicePixelRatio();
    m_scaleFactorY *= m_item->window()->devicePixelRatio();
}

void RiveQSGOpenGLRenderNode::setRect(const QRectF &bounds)
{
    qCDebug(rqqpRendering) << "Bounds" << bounds;
    RiveQSGBaseNode::setRect(bounds);
}

RiveQSGOpenGLRenderNode::RiveQSGOpenGLRenderNode(std::weak_ptr<rive::ArtboardInstance> artboardInstance, RiveQtQuickItem *item)
    : RiveQSGRenderNode(artboardInstance, item)
{
    initializeOpenGLFunctions();
    m_renderer.initGL();
}

void RiveQSGOpenGLRenderNode::render(const RenderState *state)
{
    renderOpenGL(state);
}

void RiveQSGOpenGLRenderNode::renderOpenGL(const RenderState *state)
{
    if (m_artboardInstance.expired()) {
        return;
    }

    const float devicePixelRatio = m_item->window()->devicePixelRatio();

    const QPointF globalPos = m_item->mapToItem(nullptr, QPointF(0, 0)) * devicePixelRatio;

    QMatrix4x4 projectionMatrix = *state->projectionMatrix();
    projectionMatrix.scale(1 / devicePixelRatio, 1 / devicePixelRatio);

    QMatrix4x4 modelMatrix;
    modelMatrix.translate(globalPos.x(), globalPos.y());
    modelMatrix.translate(m_topLeftRivePosition.x(), m_topLeftRivePosition.y());
    modelMatrix.scale(m_scaleFactorX, m_scaleFactorY);

    m_renderer.reset();

    m_renderer.updateViewportSize();
    m_renderer.updateModelMatrix(modelMatrix);
    m_renderer.updateProjectionMatrix(projectionMatrix);

    // TODO: scissorRect of RenderState is 0x0 width,
    // just create it for now.
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    const int viewportHeight = viewport[3];

    const auto itemWidth = m_item->width() * devicePixelRatio;
    const auto itemHeight = m_item->height() * devicePixelRatio;

    const auto scissorX = static_cast<int>(globalPos.x());
    const auto scissorY = static_cast<int>(viewportHeight - globalPos.y() - itemHeight);

    glEnable(GL_SCISSOR_TEST);
    glScissor(scissorX, scissorY, itemWidth, itemHeight);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    // this renders the artboard!

    m_artboardInstance.lock()->draw(&m_renderer);
    glDisable(GL_SCISSOR_TEST);
}
