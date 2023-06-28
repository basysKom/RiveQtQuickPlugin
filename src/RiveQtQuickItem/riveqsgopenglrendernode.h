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

#include <rive/artboard.hpp>

#include "riveqsgrendernode.h"
#include "renderer/riveqtopenglrenderer.h"

class RiveQtQuickItem;

class RiveQSGOpenGLRenderNode : public RiveQSGRenderNode, public QOpenGLFunctions
{
public:
    RiveQSGOpenGLRenderNode(rive::ArtboardInstance *artboardInstance, RiveQtQuickItem *item);

    void render(const RenderState *state) override;

    virtual void updateArtboardInstance(rive::ArtboardInstance *artboardInstance) override;

    void setArtboardRect(const QRectF &bounds) override;
    void setRect(const QRectF &bounds) override;

protected:
    void renderOpenGL(const RenderState *state);
    RiveQtOpenGLRenderer m_renderer;
};
