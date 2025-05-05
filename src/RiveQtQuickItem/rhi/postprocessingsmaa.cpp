// SPDX-FileCopyrightText: 2023 Berthold Krevert <berthold.krevert@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "postprocessingsmaa.h"
// lookup textures
#include "textures/AreaTex.h"
#include "textures/SearchTex.h"

#include <QFile>
#include <private/qrhi_p.h>
#include <private/qsgrendernode_p.h>

// quad for our onscreen texture
static float vertexData[] = { // Y up, CCW
    -1.0f, 1.0f, 0.0f, 0.0f, -1.0f, -1.0f, 0.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f
};

static quint16 indexData[] = { 0, 1, 2, 0, 2, 3 };

// resolution (2 * 4) + flip (4)
const int UBUFSIZE = 12;

QShader getShader(const QString &name)
{
    QFile f(name);
    if (f.open(QIODevice::ReadOnly))
        return QShader::fromSerialized(f.readAll());

    return QShader();
}

PostprocessingSMAA::PostprocessingSMAA(QSGRendererInterface::GraphicsApi graphicsApi)
    : m_isInitialized(false)
    , m_api(graphicsApi)
{
    QRhiShaderStage edgesVertex = loadShader(QRhiShaderStage::Vertex, ":/shaders/qt6/edges.vert.qsb");
    QRhiShaderStage edgesFragment = loadShader(QRhiShaderStage::Fragment, ":/shaders/qt6/edges-luma.frag.qsb");

    m_shaders.edgePass = { edgesVertex, edgesFragment };

    QRhiShaderStage weightsVertex = loadShader(QRhiShaderStage::Vertex, ":/shaders/qt6/smaa-weights.vert.qsb");
    QRhiShaderStage weightsFragment = loadShader(QRhiShaderStage::Fragment, ":/shaders/qt6/smaa-weights.frag.qsb");

    m_shaders.weightsPass = { weightsVertex, weightsFragment };

    QRhiShaderStage blendVertex = loadShader(QRhiShaderStage::Vertex, ":/shaders/qt6/smaa-blend.vert.qsb");
    QRhiShaderStage blendFragment = loadShader(QRhiShaderStage::Fragment, ":/shaders/qt6/smaa-blend.frag.qsb");

    m_shaders.blendPass = { blendVertex, blendFragment };
}

PostprocessingSMAA::~PostprocessingSMAA()
{
    // clear initializePostprocessingPipeline part
    cleanup();

    // clear c'tor part
    m_shaders.edgePass.clear();
    m_shaders.weightsPass.clear();
    m_shaders.blendPass.clear();
}

QRhiShaderStage PostprocessingSMAA::loadShader(QRhiShaderStage::Type type, const QString &filename) const
{
    QFile f(filename);
    if (f.open(QIODevice::ReadOnly)) {
        return QRhiShaderStage(type, QShader::fromSerialized(f.readAll()));
    }
    return QRhiShaderStage();
}

QByteArray PostprocessingSMAA::loadAreaTextureAsRGBA8Array()
{
    const unsigned char empty = 0x00;
    const unsigned char full = 0xff;
    QByteArray data;
    const auto textureSize = AREATEX_WIDTH * AREATEX_HEIGHT;
    // texture with two channels
    assert(textureSize * 2 == sizeof(areaTexBytes) / sizeof(areaTexBytes[0]));

    data.resize(textureSize * 4);

    // RHI doesn't support R8G8 textures, so convert to RGBA8
    for (auto k = 0; k < textureSize; k++) {
        data[4 * k + 0] = areaTexBytes[2 * k]; // r (data channel)
        data[4 * k + 1] = areaTexBytes[2 * k + 1]; // g (data channel)
        data[4 * k + 2] = empty; // b (empty)
        data[4 * k + 3] = full; // a
    }

    return data;
}

QByteArray PostprocessingSMAA::loadSearchTextureAsR8Array()
{
    const auto textureSize = SEARCHTEX_WIDTH * SEARCHTEX_HEIGHT;
    // one channel texture
    assert(textureSize == sizeof(searchTexBytes) / sizeof(searchTexBytes[0]));

    return QByteArray(reinterpret_cast<const char *>(searchTexBytes), textureSize);
}

// void PostprocessingSMAA::initializePostprocessingPipeline(QRhi* rhi, const QSizeF &size, std::weak_ptr<QRhiTexture> frameTexture)
void PostprocessingSMAA::initializePostprocessingPipeline(QRhi *rhi, QRhiCommandBuffer *commandBuffer, const QSize &size,
                                                          QRhiTexture *frameTextureA, QRhiTexture *frameTextureB)
{

    // maybe cleanup (check in method)
    m_targetSize = size;
    QRhiResourceUpdateBatch *resourceUpdates = rhi->nextResourceUpdateBatch();

    if (!m_common.quadVertexBuffer) {
        m_common.quadVertexBuffer = rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(vertexData));
        m_releasePool << m_common.quadVertexBuffer;
        m_common.quadVertexBuffer->create();
        resourceUpdates->uploadStaticBuffer(m_common.quadVertexBuffer, 0, sizeof(vertexData), vertexData);
    }

    if (!m_common.quadIndexBuffer) {
        m_common.quadIndexBuffer = rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, sizeof(indexData));
        m_releasePool << m_common.quadIndexBuffer;
        m_common.quadIndexBuffer->create();
    }

    if (!m_common.quadUbuffer) {
        m_common.quadUbuffer = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, UBUFSIZE);
        m_releasePool << m_common.quadUbuffer;
        m_common.quadUbuffer->create();
        resourceUpdates->uploadStaticBuffer(m_common.quadIndexBuffer, indexData);
    }

    if (!m_frameSampler) {
        m_frameSampler = rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None, QRhiSampler::ClampToEdge,
                                         QRhiSampler::ClampToEdge);
        m_releasePool << m_frameSampler;
        m_frameSampler->create();
    }

    if (!m_edgesPass.edgesTexture) {
        // Target for edges
        m_edgesPass.edgesTexture = rhi->newTexture(QRhiTexture::RGBA8, size, 1, QRhiTexture::RenderTarget);
        m_releasePool << m_edgesPass.edgesTexture;
        m_edgesPass.edgesTexture->create();
    }

    if (!m_edgesPass.edgesSampler) {
        m_edgesPass.edgesSampler = rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None, QRhiSampler::ClampToEdge,
                                                   QRhiSampler::ClampToEdge);
        m_releasePool << m_edgesPass.edgesSampler;
        m_edgesPass.edgesSampler->create();
    }

    if (!m_edgesPass.edgesTarget) {
        // rendering pass for edges
        m_edgesPass.edgesTarget = rhi->newTextureRenderTarget({ m_edgesPass.edgesTexture });
        m_releasePool << m_edgesPass.edgesTarget;

        m_edgesPass.edgesRenderPassDescriptor = m_edgesPass.edgesTarget->newCompatibleRenderPassDescriptor();
        m_releasePool << m_edgesPass.edgesRenderPassDescriptor;

        m_edgesPass.edgesTarget->setRenderPassDescriptor(m_edgesPass.edgesRenderPassDescriptor);
        m_edgesPass.edgesTarget->create();
    }

    if (!m_edgesPass.edgesResourceBindingsA) {
        m_edgesPass.edgesResourceBindingsA = rhi->newShaderResourceBindings();
        m_edgesPass.edgesResourceBindingsA->setBindings(
            { QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
                                                       m_common.quadUbuffer, 0, UBUFSIZE),
              QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, frameTextureA, m_frameSampler) });

        m_releasePool << m_edgesPass.edgesResourceBindingsA;
        m_edgesPass.edgesResourceBindingsA->create();
    }

    if (!m_edgesPass.edgesResourceBindingsB) {
        m_edgesPass.edgesResourceBindingsB = rhi->newShaderResourceBindings();
        m_edgesPass.edgesResourceBindingsB->setBindings(
            { QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
                                                       m_common.quadUbuffer, 0, UBUFSIZE),
              QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, frameTextureB, m_frameSampler) });

        m_releasePool << m_edgesPass.edgesResourceBindingsB;
        m_edgesPass.edgesResourceBindingsB->create();
    }

    if (!m_edgesPass.edgesPipeline) {
        m_edgesPass.edgesPipeline = rhi->newGraphicsPipeline();
        m_releasePool << m_edgesPass.edgesPipeline;
        m_edgesPass.edgesPipeline->setFrontFace(rhi->isYUpInFramebuffer() ? QRhiGraphicsPipeline::CW : QRhiGraphicsPipeline::CCW);
        m_edgesPass.edgesPipeline->setShaderStages(m_shaders.edgePass.cbegin(), m_shaders.edgePass.cend());

        QRhiVertexInputLayout quadInputLayout;
        quadInputLayout.setBindings({ { 4 * sizeof(float) } });
        quadInputLayout.setAttributes(
            { { 0, 0, QRhiVertexInputAttribute::Float2, 0 }, { 0, 1, QRhiVertexInputAttribute::Float2, quint32(2 * sizeof(float)) } });

        m_edgesPass.edgesPipeline->setVertexInputLayout(quadInputLayout);
        m_edgesPass.edgesPipeline->setShaderResourceBindings(m_edgesPass.edgesResourceBindingsA);
        m_edgesPass.edgesPipeline->setRenderPassDescriptor(m_edgesPass.edgesRenderPassDescriptor);
        m_edgesPass.edgesPipeline->create();
    }

    if (!m_weightsPass.weightsTexture) {
        // Target for weights
        m_weightsPass.weightsTexture = rhi->newTexture(QRhiTexture::RGBA8, size, 1, QRhiTexture::RenderTarget);
        m_releasePool << m_weightsPass.weightsTexture;
        m_weightsPass.weightsTexture->create();
    }

    if (!m_weightsPass.weightsSampler) {
        m_weightsPass.weightsSampler = rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                                       QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge);
        m_releasePool << m_weightsPass.weightsSampler;
        m_weightsPass.weightsSampler->create();
    }

    if (!m_lookup.areaTexture) {
        // load Search and Area textures for weight processing (second pass)
        // Qt doesnt support RG8 16 bit format, so just vonvert it to RGBA8 for now
        m_lookup.areaTexture = rhi->newTexture(QRhiTexture::RGBA8, QSize(AREATEX_WIDTH, AREATEX_HEIGHT));
        m_releasePool << m_lookup.areaTexture;
        m_lookup.areaTexture->create();

        if (!m_lookup.areaSampler) {
            m_lookup.areaSampler = rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None, QRhiSampler::ClampToEdge,
                                                QRhiSampler::ClampToEdge);
            m_releasePool << m_lookup.areaSampler;
            m_lookup.areaSampler->create();
        }

        const auto areaTextureData = loadAreaTextureAsRGBA8Array();

        // QImage areaDebug(reinterpret_cast<const unsigned char*>(areaTextureData.constData()), AREATEX_WIDTH, AREATEX_HEIGHT,
        // QImage::Format_RGBA8888); areaDebug.save("AreaTest.png"); qDebug() << "Area Texture size" << areaTextureData.size();

        QRhiTextureUploadDescription areaTextureDesc({ 0, 0, { areaTextureData.constData(), quint32(areaTextureData.size()) } });
        resourceUpdates->uploadTexture(m_lookup.areaTexture, areaTextureDesc);
    }

    if (!m_lookup.searchTexture) {
        m_lookup.searchTexture = rhi->newTexture(QRhiTexture::R8, QSize(SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT));
        m_releasePool << m_lookup.searchTexture;
        m_lookup.searchTexture->create();

        if (!m_lookup.searchSampler) {
            m_lookup.searchSampler =
                rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None, QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge);
            m_releasePool << m_lookup.searchSampler;
            m_lookup.searchSampler->create();
        }

        const auto searchTextureData = loadSearchTextureAsR8Array();
        // QImage searchDebug(reinterpret_cast<const unsigned char*>(searchTextureData.constData()), SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT,
        // QImage::Format_Grayscale8); searchDebug.save("SearchTest.png"); qDebug() << "Search Texture size" << searchTextureData.size();
        QRhiTextureUploadDescription searchTextureDesc({ 0, 0, { searchTextureData.constData(), quint32(searchTextureData.size()) } });
        resourceUpdates->uploadTexture(m_lookup.searchTexture, searchTextureDesc);
    }

    if (!m_weightsPass.weightsTarget) {
        // rendering pass for weights
        m_weightsPass.weightsTarget = rhi->newTextureRenderTarget({ m_weightsPass.weightsTexture });
        m_releasePool << m_weightsPass.weightsTarget;
        m_weightsPass.weightsRenderPassDescriptor = m_weightsPass.weightsTarget->newCompatibleRenderPassDescriptor();
        m_releasePool << m_weightsPass.weightsRenderPassDescriptor;
        m_weightsPass.weightsTarget->setRenderPassDescriptor(m_weightsPass.weightsRenderPassDescriptor);
        m_weightsPass.weightsTarget->create();
    }

    if (!m_weightsPass.weightsResourceBindings) {
        m_weightsPass.weightsResourceBindings = rhi->newShaderResourceBindings();
        m_releasePool << m_weightsPass.weightsResourceBindings;
        m_weightsPass.weightsResourceBindings->setBindings(
            { QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
                                                       m_common.quadUbuffer, 0, UBUFSIZE),
              QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, m_edgesPass.edgesTexture,
                                                        m_edgesPass.edgesSampler),
              QRhiShaderResourceBinding::sampledTexture(2, QRhiShaderResourceBinding::FragmentStage, m_lookup.areaTexture,
                                                        m_lookup.areaSampler),
              QRhiShaderResourceBinding::sampledTexture(3, QRhiShaderResourceBinding::FragmentStage, m_lookup.searchTexture,
                                                        m_lookup.searchSampler) });
        m_weightsPass.weightsResourceBindings->create();
    }

    if (!m_weightsPass.weightsPipeline) {
        m_weightsPass.weightsPipeline = rhi->newGraphicsPipeline();
        m_releasePool << m_weightsPass.weightsPipeline;
        m_weightsPass.weightsPipeline->setFrontFace(rhi->isYUpInFramebuffer() ? QRhiGraphicsPipeline::CW : QRhiGraphicsPipeline::CCW);
        m_weightsPass.weightsPipeline->setShaderStages(m_shaders.weightsPass.cbegin(), m_shaders.weightsPass.cend());

        QRhiVertexInputLayout quadInputLayout;
        quadInputLayout.setBindings({ { 4 * sizeof(float) } });
        quadInputLayout.setAttributes(
            { { 0, 0, QRhiVertexInputAttribute::Float2, 0 }, { 0, 1, QRhiVertexInputAttribute::Float2, quint32(2 * sizeof(float)) } });

        m_weightsPass.weightsPipeline->setVertexInputLayout(quadInputLayout);
        m_weightsPass.weightsPipeline->setShaderResourceBindings(m_weightsPass.weightsResourceBindings);
        m_weightsPass.weightsPipeline->setRenderPassDescriptor(m_weightsPass.weightsRenderPassDescriptor);
        m_weightsPass.weightsPipeline->create();
    }

    if (!m_blendPass.blendTexture) {
        // Target for blending pass
        m_blendPass.blendTexture = rhi->newTexture(QRhiTexture::RGBA8, size, 1, QRhiTexture::RenderTarget);
        m_releasePool << m_blendPass.blendTexture;
        m_blendPass.blendTexture->create();
    }

    if (!m_blendPass.blendTarget) {
        m_blendPass.blendTarget = rhi->newTextureRenderTarget({ m_blendPass.blendTexture });
        m_releasePool << m_blendPass.blendTarget;
        m_blendPass.blendRenderPassDescriptor = m_blendPass.blendTarget->newCompatibleRenderPassDescriptor();
        m_releasePool << m_blendPass.blendRenderPassDescriptor;
        m_blendPass.blendTarget->setRenderPassDescriptor(m_blendPass.blendRenderPassDescriptor);
        m_blendPass.blendTarget->create();
    }

    if (!m_blendPass.blendResourceBindingsA) {
        // rendering pass for blending
        m_blendPass.blendResourceBindingsA = rhi->newShaderResourceBindings();
        m_blendPass.blendResourceBindingsA->setBindings(
            { QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
                                                       m_common.quadUbuffer, 0, UBUFSIZE),
              QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, frameTextureA, m_frameSampler),
              QRhiShaderResourceBinding::sampledTexture(2, QRhiShaderResourceBinding::FragmentStage, m_weightsPass.weightsTexture,
                                                        m_weightsPass.weightsSampler) });
        m_releasePool << m_blendPass.blendResourceBindingsA;
        m_blendPass.blendResourceBindingsA->create();
    }

    if (!m_blendPass.blendResourceBindingsB) {
        // rendering pass for blending
        m_blendPass.blendResourceBindingsB = rhi->newShaderResourceBindings();
        m_blendPass.blendResourceBindingsB->setBindings(
            { QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
                                                       m_common.quadUbuffer, 0, UBUFSIZE),
              QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, frameTextureB, m_frameSampler),
              QRhiShaderResourceBinding::sampledTexture(2, QRhiShaderResourceBinding::FragmentStage, m_weightsPass.weightsTexture,
                                                        m_weightsPass.weightsSampler) });
        m_releasePool << m_blendPass.blendResourceBindingsB;
        m_blendPass.blendResourceBindingsB->create();
    }

    if (!m_blendPass.blendPipeline) {
        m_blendPass.blendPipeline = rhi->newGraphicsPipeline();
        m_releasePool << m_blendPass.blendPipeline;
        m_blendPass.blendPipeline->setFrontFace(rhi->isYUpInFramebuffer() ? QRhiGraphicsPipeline::CW : QRhiGraphicsPipeline::CCW);
        m_blendPass.blendPipeline->setShaderStages(m_shaders.blendPass.cbegin(), m_shaders.blendPass.cend());

        QRhiVertexInputLayout quadInputLayout;
        quadInputLayout.setBindings({ { 4 * sizeof(float) } });
        quadInputLayout.setAttributes(
            { { 0, 0, QRhiVertexInputAttribute::Float2, 0 }, { 0, 1, QRhiVertexInputAttribute::Float2, quint32(2 * sizeof(float)) } });

        m_blendPass.blendPipeline->setVertexInputLayout(quadInputLayout);
        m_blendPass.blendPipeline->setShaderResourceBindings(m_blendPass.blendResourceBindingsA);
        m_blendPass.blendPipeline->setRenderPassDescriptor(m_blendPass.blendRenderPassDescriptor);
        m_blendPass.blendPipeline->create();
    }
    // set resolution
    float resolution[] = { static_cast<float>(m_targetSize.width()), static_cast<float>(m_targetSize.height()) };
    resourceUpdates->updateDynamicBuffer(m_common.quadUbuffer, 0, 8, &resolution);

    qint32 flip = rhi->isYUpInFramebuffer() ? 1 : 0;

    // hardcode this to 1 for vulkan backend
    if (m_api == QSGRendererInterface::GraphicsApi::VulkanRhi) {
        flip = 1;
    }
    resourceUpdates->updateDynamicBuffer(m_common.quadUbuffer, 8, 4, &flip);

    commandBuffer->resourceUpdate(resourceUpdates);
    m_postprocessingRenderTexture = m_blendPass.blendTexture;
    m_isInitialized = true;
}

void PostprocessingSMAA::postprocess(QRhi *rhi, QRhiCommandBuffer *commandBuffer, bool useTextureBufferA)
{
    if (!m_isInitialized) {
        return;
    }

    QRhiCommandBuffer::VertexInput quadBinding(m_common.quadVertexBuffer, 0);

    // first postprocess pass: edge detection
    commandBuffer->beginPass(m_edgesPass.edgesTarget, Qt::black, { 1.0f, 0 });

    commandBuffer->setGraphicsPipeline(m_edgesPass.edgesPipeline);
    commandBuffer->setViewport(
        { 0, 0, float(m_edgesPass.edgesTexture->pixelSize().width()), float(m_edgesPass.edgesTexture->pixelSize().height()) });
    if (useTextureBufferA) {
        commandBuffer->setShaderResources(m_edgesPass.edgesResourceBindingsA);
    } else {
        commandBuffer->setShaderResources(m_edgesPass.edgesResourceBindingsB);
    }
    commandBuffer->setVertexInput(0, 1, &quadBinding, m_common.quadIndexBuffer, 0, QRhiCommandBuffer::IndexUInt16);
    commandBuffer->drawIndexed(6);
    commandBuffer->endPass();

    // second postprocess pass: weights calculation
    commandBuffer->beginPass(m_weightsPass.weightsTarget, Qt::black, { 1.0f, 0 });
    commandBuffer->setGraphicsPipeline(m_weightsPass.weightsPipeline);
    commandBuffer->setViewport(
        { 0, 0, float(m_weightsPass.weightsTexture->pixelSize().width()), float(m_weightsPass.weightsTexture->pixelSize().height()) });
    commandBuffer->setShaderResources();
    commandBuffer->setVertexInput(0, 1, &quadBinding, m_common.quadIndexBuffer, 0, QRhiCommandBuffer::IndexUInt16);
    commandBuffer->drawIndexed(6);
    commandBuffer->endPass();

    // third postprocess pass: blending frame and weights
    commandBuffer->beginPass(m_blendPass.blendTarget, Qt::black, { 1.0f, 0 });
    commandBuffer->setGraphicsPipeline(m_blendPass.blendPipeline);
    commandBuffer->setViewport(
        { 0, 0, float(m_blendPass.blendTexture->pixelSize().width()), float(m_blendPass.blendTexture->pixelSize().height()) });
    if (useTextureBufferA) {
        commandBuffer->setShaderResources(m_blendPass.blendResourceBindingsA);
    } else {
        commandBuffer->setShaderResources(m_blendPass.blendResourceBindingsB);
    }
    commandBuffer->setVertexInput(0, 1, &quadBinding, m_common.quadIndexBuffer, 0, QRhiCommandBuffer::IndexUInt16);
    commandBuffer->drawIndexed(6);
    commandBuffer->endPass();
}

void PostprocessingSMAA::cleanup()
{
    if (m_isInitialized) {

        qDeleteAll(m_releasePool);
        m_releasePool.clear();

        m_common.quadVertexBuffer = nullptr;
        m_common.quadIndexBuffer = nullptr;
        m_common.quadUbuffer = nullptr;

        m_lookup.areaTexture = nullptr;
        m_lookup.areaSampler = nullptr;
        m_lookup.searchTexture = nullptr;
        m_lookup.searchSampler = nullptr;

        m_edgesPass.edgesRenderPassDescriptor = nullptr;
        m_edgesPass.edgesTarget = nullptr;
        m_edgesPass.edgesTexture = nullptr;
        m_edgesPass.edgesSampler = nullptr;
        m_edgesPass.edgesResourceBindingsA = nullptr;
        m_edgesPass.edgesResourceBindingsB = nullptr;
        m_edgesPass.edgesPipeline = nullptr;

        m_weightsPass.weightsRenderPassDescriptor = nullptr;
        m_weightsPass.weightsTarget = nullptr;
        m_weightsPass.weightsTexture = nullptr;
        m_weightsPass.weightsSampler = nullptr;
        m_weightsPass.weightsResourceBindings = nullptr;
        m_weightsPass.weightsPipeline = nullptr;

        m_blendPass.blendRenderPassDescriptor = nullptr;
        m_blendPass.blendTarget = nullptr;
        m_blendPass.blendTexture = nullptr;
        m_blendPass.blendResourceBindingsA = nullptr;
        m_blendPass.blendResourceBindingsB = nullptr;
        m_blendPass.blendPipeline = nullptr;

        m_frameSampler = nullptr;
        m_postprocessingRenderTexture = nullptr;

        m_isInitialized = false;
    }
}
