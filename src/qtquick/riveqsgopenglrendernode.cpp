
// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <QQuickWindow>

#include "riveqsgopenglrendernode.h"
#include "src/qtquick/riveqtquickitem.h"

void RiveQSGOpenGLRenderNode::updateArtboardInstance(rive::ArtboardInstance *artboardInstance)
{
    m_renderer.reset();
    m_artboardInstance = artboardInstance;
}

RiveQSGOpenGLRenderNode::RiveQSGOpenGLRenderNode(rive::ArtboardInstance *artboardInstance, RiveQtQuickItem *item)
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
    if (m_artboardInstance) {
        float devicePixelRatio = m_item->window()->devicePixelRatio();

        QPointF globalPos = m_item->mapToItem(nullptr, QPointF(0, 0));

        auto x = globalPos.x() * devicePixelRatio;
        auto y = globalPos.y() * devicePixelRatio;

        auto itemWidth = m_item->width() * devicePixelRatio;
        auto itemHeight = m_item->height() * devicePixelRatio;

        auto artBoardWidth = m_artboardInstance->width() * devicePixelRatio;
        auto artBoardHeight = m_artboardInstance->height() * devicePixelRatio;

        auto aspectX = itemWidth / artBoardWidth;
        auto aspectY = itemHeight / artBoardHeight;

        // Calculate the uniform scale factor to preserve the aspect ratio
        auto scaleFactor = qMin(aspectX, aspectY);

        // Calculate the new width and height of the item while preserving the aspect ratio
        auto newWidth = artBoardWidth * scaleFactor;
        auto newHeight = artBoardHeight * scaleFactor;

        // Calculate the offsets needed to center the item within its bounding rectangle
        auto offsetX = (itemWidth - newWidth) / 2.0;
        auto offsetY = (itemHeight - newHeight) / 2.0;

        QMatrix4x4 projectionMatrix = *state->projectionMatrix();
        projectionMatrix.scale(1 / devicePixelRatio, 1 / devicePixelRatio);

        QMatrix4x4 modelMatrix;
        modelMatrix.translate(x, y);
        modelMatrix.translate(offsetX, offsetY);
        modelMatrix.scale(scaleFactor * devicePixelRatio, scaleFactor * devicePixelRatio);

        m_renderer.updateViewportSize();
        m_renderer.updateModelMatrix(modelMatrix);
        m_renderer.updateProjectionMatrix(projectionMatrix);

        // TODO: sicssorRect of RenderState is 0x0 width,
        // just create it for now.
        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        int viewportHeight = viewport[3];

        glEnable(GL_SCISSOR_TEST);
        int scissorX = static_cast<int>(x + offsetX);
        int scissorY = static_cast<int>(viewportHeight - y - itemHeight);
        int scissorWidth = static_cast<int>(newWidth);
        int scissorHeight = static_cast<int>(newHeight);
        glScissor(scissorX, scissorY, scissorWidth, scissorHeight);

        // this renders the artboard!
        m_artboardInstance->draw(&m_renderer);

        glDisable(GL_SCISSOR_TEST);
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
}
