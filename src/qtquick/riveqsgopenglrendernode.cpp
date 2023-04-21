/*
 * MIT License
 *
 * Copyright (C) 2023 by Jeremias Bosch
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

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

    QPointF globalPos = globalPosition(m_item);
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

    QMatrix4x4 modelMatrix;
    modelMatrix.translate(x + offsetX, y + offsetY);
    modelMatrix.scale(scaleFactor, scaleFactor);

    m_renderer.updateViewportSize();
    m_renderer.updateModelMatrix(modelMatrix);
    m_renderer.updateProjectionMatrix(*state->projectionMatrix());

    // TODO: sicssorRect of RenderState is 0x0 width,
    // just create it for now.
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    int viewportHeight = viewport[3];

    glEnable(GL_SCISSOR_TEST);
    int scissorX = static_cast<int>(x);
    int scissorY = static_cast<int>(viewportHeight - y - m_item->height());
    int scissorWidth = static_cast<int>(m_item->width());
    int scissorHeight = static_cast<int>(m_item->height());
    glScissor(scissorX, scissorY, scissorWidth, scissorHeight);

    // this renders the artboard!
    m_artboardInstance->draw(&m_renderer);

    glDisable(GL_SCISSOR_TEST);
  }

  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
}
