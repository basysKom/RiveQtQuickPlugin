// SPDX-FileCopyrightText: 2023 Berthold Krevert <berthold.krevert@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QVector>
#include <QSizeF>

#include <private/qrhi_p.h>
#include <private/qsgrendernode_p.h>

class PostprocessingSMAA
{
public:
    PostprocessingSMAA();
    virtual ~PostprocessingSMAA();

    QRhiTexture *getTarget() const { return m_target; }
    bool isInitialized() const { return m_isInitialized; }

    //void initializePostprocessingPipeline(QRhi *rhi, const QSizeF &size, std::weak_ptr<QRhiTexture> frameTexture);
    void initializePostprocessingPipeline(QRhi *rhi, QRhiCommandBuffer* commandBuffer,
        const QSize &size, QRhiTexture* frameTexture);
    void postprocess(QRhi *rhi, QRhiCommandBuffer* commandBuffer);
    void cleanup();

private:
    QRhiShaderStage loadShader(QRhiShaderStage::Type type, const QString &filename) const;
    QByteArray loadAreaTextureAsRGBA8Array();
    QByteArray loadSearchTextureAsR8Array();

    bool m_isInitialized { false };
    QVector<QRhiResource *> m_releasePool;

    // not owned by us (ToDo: should be a weak pointer)
    QRhiTexture* m_frameTexture;

    // owned by us
    QRhiSampler* m_frameSampler;
    struct {
        QVector<QRhiShaderStage> edgePass;
        QVector<QRhiShaderStage> weightsPass;
        QVector<QRhiShaderStage> blendPass;
    } m_shaders;

    struct {
        QRhiBuffer *quadVertexBuffer = nullptr;
        QRhiBuffer *quadIndexBuffer = nullptr;
        QRhiBuffer *quadUbuffer = nullptr;
    } m_common;

    struct {
        // lookup textures for weight computation
        QRhiTexture *areaTexture = nullptr ;
        QRhiSampler *areaSampler = nullptr;
        QRhiTexture *searchTexture = nullptr ;
        QRhiSampler *searchSampler = nullptr;
    } m_lookup;

    struct {
        // edges target (we are going to render the edges into this texture)
        QRhiRenderPassDescriptor *edgesRenderPassDescriptor = nullptr;
        QRhiTextureRenderTarget *edgesTarget = nullptr;
        QRhiTexture *edgesTexture = nullptr;
        QRhiSampler *edgesSampler = nullptr;

        // rendering pipeline for edges
        QRhiShaderResourceBindings *edgesResourceBindings = nullptr;
        QRhiGraphicsPipeline *edgesPipeline = nullptr;
    } m_edgesPass;

    struct {
        // weights target (we are going to render the weights into this texture)
        QRhiRenderPassDescriptor *weightsRenderPassDescriptor = nullptr;
        QRhiTextureRenderTarget *weightsTarget = nullptr;
        QRhiTexture *weightsTexture = nullptr ;
        QRhiSampler *weightsSampler = nullptr;

        // rendering pipeline for our weight calculation pass
        QRhiShaderResourceBindings *weightsResourceBindings = nullptr;
        QRhiGraphicsPipeline *weightsPipeline = nullptr;
    } m_weightsPass;

    struct {
        // blend target (this is the last pass, so we don't need a sampler for the next stage here)
        QRhiRenderPassDescriptor *blendRenderPassDescriptor = nullptr;
        QRhiTextureRenderTarget *blendTarget = nullptr;
        QRhiTexture *blendTexture = nullptr ;

        // rendering pipeline for blending pass
        QRhiShaderResourceBindings *blendResourceBindings = nullptr;
        QRhiGraphicsPipeline *blendPipeline = nullptr;
    } m_blendPass;

    QSize m_targetSize;
    QRhiTexture *m_target { nullptr };

};