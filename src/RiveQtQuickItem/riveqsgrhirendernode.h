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
    void setFillMode(const RiveRenderSettings::FillMode mode);
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

    void switchCurrentRenderBuffer();

    QRhiTextureRenderTarget *currentRenderTarget(bool shaderBlending);
    QRhiTextureRenderTarget *currentBlendTarget();
    QRhiGraphicsPipeline *renderPipeline(bool shaderBlending);
    QRhiGraphicsPipeline *clippingPipeline();
    QRhiGraphicsPipeline *currentBlendPipeline();
    QRhiRenderPassDescriptor *currentRenderPassDescriptor(bool shaderBlending);
    QRhiRenderPassDescriptor *currentBlendPassDescriptor();

    QRhiTexture *getCurrentRenderBuffer();
    QRhiTexture *getRenderBufferA();
    QRhiTexture *getRenderBufferB();
    QRhiTexture *getRenderBufferIntern();

    bool isCurrentRenderBufferA();

protected:
    struct RenderSurface
    {
        QRhiTexture *texture { nullptr };
        QRhiRenderPassDescriptor *desc { nullptr };
        QRhiRenderPassDescriptor *blendDesc { nullptr };
        QRhiTextureRenderTarget *target { nullptr };
        QRhiTextureRenderTarget *blendTarget { nullptr };

        ~RenderSurface() { cleanUp(); }

        void cleanUp();

        bool valid() { return texture != nullptr; }

        // this creates all resources needed to have a texture to draw on
        bool create(QRhi *rhi, const QSize &surfaceSize, QRhiRenderBuffer *stencilClippingBuffer,
                    QRhiTextureRenderTarget::Flags flags = QRhiTextureRenderTarget::PreserveColorContents);
    };

    QRhiBuffer *m_vertexBuffer { nullptr };
    QRhiBuffer *m_texCoordBuffer { nullptr };
    QRhiBuffer *m_finalDrawUniformBuffer { nullptr };
    QRhiBuffer *m_drawUniformBuffer { nullptr };
    QRhiBuffer *m_blendUniformBuffer { nullptr };

    QRhiBuffer *m_clippingUniformBuffer { nullptr };

    // shared clipping buffer for all surfaces
    QRhiRenderBuffer *m_stencilClippingBuffer { nullptr };

    // those bind the texture of surfaceA and B for the final draw call
    QRhiShaderResourceBindings *m_finalDrawResourceBindingsA { nullptr };
    QRhiShaderResourceBindings *m_finalDrawResourceBindingsB { nullptr };

    // in order to configure our pipelines we have to provide resourceBindings
    // those are empty - the actual bindings are created/set per texturenode
    // they can not be shared since bindings will transfer information over renderpasses
    // QRhiShaderResourceBindings *m_dummyResourceBindings { nullptr };

    QRhiShaderResourceBindings *m_blendResourceBindingsA { nullptr };
    QRhiShaderResourceBindings *m_blendResourceBindingsB { nullptr };
    QRhiShaderResourceBindings *m_drawPipelineResourceBindings { nullptr };
    QRhiShaderResourceBindings *m_clippingResourceBindings { nullptr };

    QRhiSampler *m_sampler { nullptr };
    QRhiSampler *m_blendSampler { nullptr };

    QList<QRhiShaderStage> m_finalDrawShader;
    QList<QRhiShaderStage> m_blendShaders;
    QList<QRhiShaderStage> m_pathShader;
    QList<QRhiShaderStage> m_clipShader;

    QList<QVector2D> m_vertices;
    QList<QVector2D> m_texCoords;

    QVector<QRhiResource *> m_cleanupList;

    RiveQtRhiRenderer *m_renderer { nullptr };

    // RenderSurface A and B swap during the renderpathes
    // since we can not read and write from a texture at the same time
    // this is used/switch only in case we shader blend in texturenode
    // the renderSurfaceIntern is a special surface used as temporary render target for geometry that needs to blend in the mainscene
    RenderSurface m_renderSurfaceA;
    RenderSurface m_renderSurfaceB;
    RenderSurface m_renderSurfaceIntern;

    QRhiTexture *m_qImageTexture { nullptr };
    // pointer that holds the current render surface (a/b)
    // this will be set to &m_renderSurfaceA or &m_renderSurfaceB by the renderprocess
    RenderSurface *m_currentRenderSurface { nullptr };

    // used to draw the final texture on the qt surface
    QRhiGraphicsPipeline *m_finalDrawPipeline { nullptr };

    // used in the main draw call to paint directly to the renderSurface texture
    QRhiGraphicsPipeline *m_drawPipeline { nullptr };

    // used in the main draw call in case we need to draw a to-be-blend geometry
    QRhiGraphicsPipeline *m_drawPipelineIntern { nullptr };

    // used to shader-blend the to-be-blend geomentry texture to the current renderSurface
    QRhiGraphicsPipeline *m_blendPipeline { nullptr };

    // used to draw into the stencil buffer during the main draw call
    QRhiGraphicsPipeline *m_clipPipeline { nullptr };

    // we need this since our default target preserves colors
    // this is configured to not preserve
    QRhiTextureRenderTarget *m_cleanUpTextureTarget { nullptr };

    bool m_verticesDirty = true;
    RiveRenderSettings::FillMode m_fillMode;

    PostprocessingSMAA *m_postprocessing { nullptr };

private:
    QRhiGraphicsPipeline *createBlendPipeline(QRhi *rhi, QRhiRenderPassDescriptor *renderPass, QRhiShaderResourceBindings *bindings);
    QRhiGraphicsPipeline *createClipPipeline(QRhi *rhi, QRhiRenderPassDescriptor *renderPassDescriptor,
                                             QRhiShaderResourceBindings *bindings);
    QRhiGraphicsPipeline *createDrawPipeline(QRhi *rhi, bool srcOverBlend, bool stencilBuffer,
                                             QRhiRenderPassDescriptor *renderPassDescriptor, QRhiGraphicsPipeline::Topology t,
                                             const QList<QRhiShaderStage> &shader, QRhiShaderResourceBindings *bindings);
};
