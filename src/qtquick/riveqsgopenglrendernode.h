// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QElapsedTimer>
#include <QQuickItem>
#include <QQuickPaintedItem>
#include <QSGRenderNode>
#include <QSGTextureProvider>

#include "rive/artboard.hpp"
#include "riveqtfactory.h"
#include "riveqsgrendernode.h"
#include "riveqtopenglrenderer.h"

class RiveQtQuickItem;

class RiveQSGOpenGLRenderNode : public RiveQSGRenderNode, public QOpenGLFunctions
{
public:
  RiveQSGOpenGLRenderNode(rive::ArtboardInstance *artboardInstance, RiveQtQuickItem *item);

  void render(const RenderState *state) override;

  virtual void updateArtboardInstance(rive::ArtboardInstance *artboardInstance) override;

protected:
  void renderOpenGL(const RenderState *state);
  RiveQtOpenGLRenderer m_renderer;
};
