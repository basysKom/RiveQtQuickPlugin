
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
    RiveQSGBaseNode::updateArtboardInstance(artboardInstance);
}

void RiveQSGOpenGLRenderNode::setArtboardRect(const QRectF &bounds)
{
    RiveQSGBaseNode::setArtboardRect(bounds);
    m_scaleFactorX *= m_window->devicePixelRatio();
    m_scaleFactorY *= m_window->devicePixelRatio();
}

void RiveQSGOpenGLRenderNode::setRect(const QRectF &bounds)
{
    qCDebug(rqqpRendering) << "Bounds" << bounds;
    RiveQSGBaseNode::setRect(bounds);
}

RiveQSGOpenGLRenderNode::RiveQSGOpenGLRenderNode(QQuickWindow *window, std::weak_ptr<rive::ArtboardInstance> artboardInstance,
                                                 const QRectF &geometry)
    : RiveQSGRenderNode(window, artboardInstance, geometry)
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

    const float devicePixelRatio = m_window->devicePixelRatio();
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    const int viewportHeight = viewport[3];

    QMatrix4x4 mvp = *state->projectionMatrix();
    QMatrix4x4 modelMatrix = *matrix();

    const auto itemWidth = m_rect.width() * devicePixelRatio;
    const auto itemHeight = m_rect.height() * devicePixelRatio;

    const auto scissorX = static_cast<int>(modelMatrix(0, 3));
    const auto scissorY = static_cast<int>(viewportHeight - modelMatrix(1, 3) - itemHeight);

    modelMatrix.translate(m_topLeftRivePosition.x(), m_topLeftRivePosition.y());
    modelMatrix.scale(m_scaleFactorX, m_scaleFactorY);

    m_renderer.reset();

    m_renderer.updateViewportSize();
    m_renderer.updateModelMatrix(modelMatrix);
    m_renderer.updateProjectionMatrix(mvp);

    glEnable(GL_SCISSOR_TEST);
    glScissor(scissorX, scissorY, itemWidth, itemHeight);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    // this renders the artboard!
    m_artboardInstance.lock()->draw(&m_renderer);
    glDisable(GL_SCISSOR_TEST);
}
