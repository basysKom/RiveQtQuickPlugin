// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QQuickItem>
#include <QElapsedTimer>
#include <QQuickItem>
#include <QQuickPaintedItem>
#include <QSGRenderNode>
#include <QSGTextureProvider>

#include <private/qrhi_p.h>
#include <private/qsgrendernode_p.h>

#include <rive/artboard.hpp>

#include "datatypes.h"
#include "riveqsgrendernode.h"

//-----------------
class RiveQtQuickItem;
class TextureTargetNode;
class RiveQtRhiRenderer;
class PostprocessingSMAA;

class RiveQSGRHIRenderNode : public RiveQSGRenderNode
{
public:
    RiveQSGRHIRenderNode(QQuickWindow *window, std::weak_ptr<rive::ArtboardInstance> artboardInstance, const QRectF &geometry);
    virtual ~RiveQSGRHIRenderNode();

    void setRect(const QRectF &bounds) override;
    void setFillMode(const RiveRenderSettings::FillMode mode) { m_fillMode = mode; }
    void setPostprocessingMode(const RiveRenderSettings::PostprocessingMode postprocessingMode);

    void renderOffscreen() override;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    void prepare() override;
#else
    void prepare();
#endif
    void render(const RenderState *state) override;
    void releaseResources() override;
    RenderingFlags flags() const override;
    QSGRenderNode::StateFlags changedStates() const override;

protected:
    QRhiBuffer *m_vertexBuffer { nullptr };
    QRhiBuffer *m_texCoordBuffer { nullptr };
    QRhiBuffer *m_uniformBuffer { nullptr };

    QRhiShaderResourceBindings *m_resourceBindings { nullptr };
    QRhiGraphicsPipeline *m_pipeLine { nullptr };
    QRhiSampler *m_sampler { nullptr };

    QList<QRhiShaderStage> m_shaders;

    QList<QVector2D> m_vertices;
    QList<QVector2D> m_texCoords;

    QVector<QRhiResource *> m_cleanupList;

    RiveQtRhiRenderer *m_renderer { nullptr };
    // ToDo: make std::shared_ptr
    QRhiTexture *m_displayBuffer { nullptr };
    QRhiTextureRenderTarget *m_cleanUpTextureTarget { nullptr };

    bool m_verticesDirty = true;
    RiveRenderSettings::FillMode m_fillMode;

    PostprocessingSMAA* m_postprocessing;
};
