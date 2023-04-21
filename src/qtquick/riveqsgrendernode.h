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

#pragma once

#include <QElapsedTimer>
#include <QQuickItem>
#include <QQuickPaintedItem>
#include <QSGRenderNode>
#include <QSGTextureProvider>

#include "rive/artboard.hpp"
#include "riveqtfactory.h"

class RiveQtQuickItem;

class RiveQSGRenderNode : public QSGRenderNode
{
public:
  RiveQSGRenderNode(rive::ArtboardInstance *artboardInstance, RiveQtQuickItem *item);

  QRectF rect() const override;

  StateFlags changedStates() const override { return QSGRenderNode::BlendState; }
  RenderingFlags flags() const override { return QSGRenderNode::BoundedRectRendering | QSGRenderNode::DepthAwareRendering; }

  virtual void updateArtboardInstance(rive::ArtboardInstance *artboardInstance);

protected:
  QPointF globalPosition(QQuickItem *item);

  rive::ArtboardInstance *m_artboardInstance { nullptr };
  RiveQtQuickItem *m_item { nullptr };
};
