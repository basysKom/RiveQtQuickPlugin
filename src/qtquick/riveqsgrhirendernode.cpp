// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <QQuickWindow>
#include <QFile>

#include <private/qrhi_p.h>
#include <private/qsgrendernode_p.h>

#include "riveqsgrhirendernode.h"
#include "src/qtquick/riveqtquickitem.h"
#include "src/renderer/riveqtrhirenderer.h"

void RiveQSGRHIRenderNode::updateArtboardInstance(rive::ArtboardInstance *artboardInstance)
{
    m_artboardInstance = artboardInstance;
}

RiveQSGRHIRenderNode::RiveQSGRHIRenderNode(rive::ArtboardInstance *artboardInstance, RiveQtQuickItem *item)
    : RiveQSGRenderNode(artboardInstance, item)
    , m_displayBuffer(nullptr)
{
    m_window = item->window();

    QFile file;
    file.setFileName(":/shaders/qt6/finalDraw.vert.qsb");
    file.open(QFile::ReadOnly);
    m_shaders.append(QRhiShaderStage(QRhiShaderStage::Vertex, QShader::fromSerialized(file.readAll())));

    file.close();
    file.setFileName(":/shaders/qt6/finalDraw.frag.qsb");
    file.open(QFile::ReadOnly);

    m_shaders.append(QRhiShaderStage(QRhiShaderStage::Fragment, QShader::fromSerialized(file.readAll())));

    setRect(QRectF(item->x(), item->y(), item->width(), item->height()));

    m_texCoords.append(QVector2D(0.0f, 0.0f));
    m_texCoords.append(QVector2D(0.0f, 1.0f));
    m_texCoords.append(QVector2D(1.0f, 0.0f));
    m_texCoords.append(QVector2D(1.0f, 1.0f));

    m_renderer = new RiveQtRhiRenderer(item);
    m_renderer->updateViewPort(m_rect, m_displayBuffer);
}

RiveQSGRHIRenderNode::~RiveQSGRHIRenderNode()
{
    // tell my item that I'm gone.
    if (m_item) {
        if (m_item->m_renderNode == this) {
            m_item->m_renderNode = nullptr;
        }
    }

    while (!m_cleanupList.empty()) {
        auto *resource = m_cleanupList.first();
        m_cleanupList.removeAll(resource);
        resource->destroy();
        delete resource;
    }

    delete m_renderer;
}

void RiveQSGRHIRenderNode::setRect(const QRectF &bounds)
{
    m_rect = bounds;

    m_vertices.clear();

    m_vertices.append(QVector2D(bounds.x(), bounds.y()));
    m_vertices.append(QVector2D(bounds.x(), bounds.y() + bounds.height()));
    m_vertices.append(QVector2D(bounds.x() + bounds.width(), bounds.y()));
    m_vertices.append(QVector2D(bounds.x() + bounds.width(), bounds.y() + bounds.height()));

    // todo this is not yet fully correct. Resize is super expensive due to resource destruction
    // TODO: maybe we should only do this in case the texture gets larger and stays larger for some time
    // that may cost us quality but will save us a lot of issues
    if (m_displayBuffer) {
        m_cleanupList.removeAll(m_displayBuffer);
        m_displayBuffer->destroy();
        m_displayBuffer = nullptr;
    }

    if (m_sampler) {
        m_cleanupList.removeAll(m_sampler);
        m_sampler->destroy();
        m_sampler = nullptr;
    }

    if (m_cleanUpTextureTarget) {
        m_cleanupList.removeAll(m_cleanUpTextureTarget);
        m_cleanUpTextureTarget->destroy();
        m_cleanUpTextureTarget = nullptr;
    }

    // this potentialy crashes :/
    if (m_resourceBindings) {
        m_cleanupList.removeAll(m_resourceBindings);
        m_resourceBindings->destroy();
        m_resourceBindings = nullptr;
    }

    m_verticesDirty = true;

    markDirty(QSGNode::DirtyGeometry);
}

void RiveQSGRHIRenderNode::setFillMode(const RenderSettings::FillMode fillMode)
{
    m_renderer->setFillMode(fillMode);
}

void RiveQSGRHIRenderNode::renderOffscreen()
{
    if (!m_displayBuffer || m_rect.width() == 0 || m_rect.height() == 0 || !m_item)
        return;

    QSGRendererInterface *renderInterface = m_window->rendererInterface();
    QRhi *rhi = static_cast<QRhi *>(renderInterface->getResource(m_window, QSGRendererInterface::RhiResource));

    QRhiCommandBuffer *cb;
    rhi->beginOffscreenFrame(&cb);
    {
        // clean our main texture
        cb->beginPass(m_cleanUpTextureTarget, QColor(0, 0, 0, 0), { 1.0f, 0 });
        cb->endPass();
        // draw elements to our shared texture
        m_renderer->render(cb);
    }
    rhi->endOffscreenFrame();
}

void RiveQSGRHIRenderNode::render(const RenderState *state)
{
    if (!m_artboardInstance) {
        return;
    }

    QSGRendererInterface *renderInterface = m_window->rendererInterface();
    QRhiSwapChain *swapChain =
        static_cast<QRhiSwapChain *>(renderInterface->getResource(m_window, QSGRendererInterface::RhiSwapchainResource));
    Q_ASSERT(swapChain);

    QRhiCommandBuffer *commandBuffer = swapChain->currentFrameCommandBuffer();
    Q_ASSERT(commandBuffer);

    commandBuffer->setGraphicsPipeline(m_pipeLine);
    QSize renderTargetSize = QSGRenderNodePrivate::get(this)->m_rt.rt->pixelSize();
    commandBuffer->setViewport(QRhiViewport(0, 0, renderTargetSize.width(), renderTargetSize.height()));
    commandBuffer->setShaderResources(m_resourceBindings);
    QRhiCommandBuffer::VertexInput vertexBindings[] = { { m_vertexBuffer, 0 }, { m_texCoordBuffer, 0 } };
    commandBuffer->setVertexInput(0, 2, vertexBindings);

    commandBuffer->draw(m_vertices.count());
}

void RiveQSGRHIRenderNode::releaseResources()
{
    while (!m_cleanupList.empty()) {
        auto *resource = m_cleanupList.first();
        m_cleanupList.removeAll(resource);
        resource->destroy();
        delete resource;
    }

    m_vertexBuffer = nullptr;
    m_texCoordBuffer = nullptr;
    m_uniformBuffer = nullptr;
    m_pipeLine = nullptr;
    m_resourceBindings = nullptr;
    m_sampler = nullptr;
    m_displayBuffer = nullptr;
}

QSGRenderNode::RenderingFlags RiveQSGRHIRenderNode::flags() const
{
    // We are rendering 2D content directly into the scene graph
    return { QSGRenderNode::NoExternalRendering };
}

QSGRenderNode::StateFlags RiveQSGRHIRenderNode::changedStates() const
{
    return { QSGRenderNode::StateFlag::ViewportState | QSGRenderNode::StateFlag::CullState | QSGRenderNode::RenderTargetState
             | QSGRenderNode::ScissorState };
}

void RiveQSGRHIRenderNode::prepare()
{
    QSGRendererInterface *renderInterface = m_window->rendererInterface();
    QRhiSwapChain *swapChain =
        static_cast<QRhiSwapChain *>(renderInterface->getResource(m_window, QSGRendererInterface::RhiSwapchainResource));
    QRhi *rhi = static_cast<QRhi *>(renderInterface->getResource(m_window, QSGRendererInterface::RhiResource));
    Q_ASSERT(swapChain);
    Q_ASSERT(rhi);

    if (!m_displayBuffer) {
        m_displayBuffer = rhi->newTexture(QRhiTexture::RGBA8, QSize(m_rect.width(), m_rect.height()), 1,
                                          QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource);
        m_displayBuffer->create();
        m_cleanupList.append(m_displayBuffer);

        if (m_renderer) {
            m_renderer->updateViewPort(m_rect, m_displayBuffer);
        }
    }

    if (m_artboardInstance) {
        m_renderer->recycleRiveNodes();

        m_renderer->updateArtboardSize(QSize(m_artboardInstance->width(), m_artboardInstance->height()));
        m_renderer->setProjectionMatrix(projectionMatrix(), matrix());

        m_artboardInstance->draw(m_renderer);

    } else {
        return;
    }

    if (!m_cleanUpTextureTarget) {
        QRhiColorAttachment colorAttachment(m_displayBuffer);
        QRhiTextureRenderTargetDescription desc(colorAttachment);
        m_cleanUpTextureTarget = rhi->newTextureRenderTarget(desc);

        m_cleanUpTextureTarget->create();
        m_cleanupList.append(m_cleanUpTextureTarget);
    }

    QRhiResourceUpdateBatch *resourceUpdates = rhi->nextResourceUpdateBatch();

    if (m_verticesDirty) {
        if (m_vertexBuffer) {
            delete m_vertexBuffer;
            m_vertexBuffer = nullptr;
        }
        m_verticesDirty = false;
    }

    if (!m_vertexBuffer) {
        int vertexCount = m_vertices.count();

        int positionBufferSize = vertexCount * sizeof(QVector2D);
        m_vertexBuffer = rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, positionBufferSize);
        m_vertexBuffer->create();
        m_cleanupList.append(m_vertexBuffer);

        QByteArray positionData;
        positionData.resize(positionBufferSize);
        memcpy(positionData.data(), m_vertices.constData(), positionBufferSize);
        resourceUpdates->uploadStaticBuffer(m_vertexBuffer, positionData);

        int texCoordBufferSize = vertexCount * sizeof(QVector2D);
        m_texCoordBuffer = rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, texCoordBufferSize);
        m_texCoordBuffer->create();
        m_cleanupList.append(m_texCoordBuffer);

        QByteArray texCoordData;
        texCoordData.resize(texCoordBufferSize);
        memcpy(texCoordData.data(), m_texCoords.constData(), texCoordBufferSize);
        resourceUpdates->uploadStaticBuffer(m_texCoordBuffer, texCoordData);
    }

    if (!m_sampler) {
        m_sampler = rhi->newSampler(QRhiSampler::Nearest, QRhiSampler::Nearest, QRhiSampler::None, QRhiSampler::ClampToEdge,
                                    QRhiSampler::ClampToEdge);
        m_sampler->create();
        m_cleanupList.append(m_sampler);
    }

    if (!m_uniformBuffer) {
        m_uniformBuffer = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 80);
        m_uniformBuffer->create();
        m_cleanupList.append(m_uniformBuffer);
    }

    if (!m_resourceBindings) {
        m_resourceBindings = rhi->newShaderResourceBindings();
        m_resourceBindings->setBindings({
            QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
                                                     m_uniformBuffer),
            QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, m_displayBuffer, m_sampler),
        });

        m_resourceBindings->create();
        m_cleanupList.append(m_resourceBindings);
    }

    if (!m_pipeLine) {
        m_pipeLine = rhi->newGraphicsPipeline();
        m_cleanupList.append(m_pipeLine);

        //
        // If layer.enabled == true on our QQuickItem, the rendering face is flipped for
        // backends with isYUpInFrameBuffer == true (OpenGL). This does not happen with
        // RHI backends with isYUpInFrameBuffer == false. We swap the triangle winding
        // order to work around this.
        //
        m_pipeLine->setFrontFace(rhi->isYUpInFramebuffer() ? QRhiGraphicsPipeline::CCW : QRhiGraphicsPipeline::CW);
        m_pipeLine->setCullMode(QRhiGraphicsPipeline::None);
        m_pipeLine->setTopology(QRhiGraphicsPipeline::TriangleStrip);

        // this allows blending with the rest of the QML Scene
        QRhiGraphicsPipeline::TargetBlend blend;
        blend.enable = true;
        blend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
        blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
        blend.srcAlpha = QRhiGraphicsPipeline::One;
        blend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;
        m_pipeLine->setTargetBlends({ blend });

        m_pipeLine->setShaderResourceBindings(m_resourceBindings);
        m_pipeLine->setShaderStages(m_shaders.cbegin(), m_shaders.cend());

        QRhiVertexInputLayout inputLayout;
        inputLayout.setBindings({
            { sizeof(QVector2D) }, // Stride for position buffer
            { sizeof(QVector2D) } // Stride for texture coordinate buffer
        });
        inputLayout.setAttributes({
            { 0, 0, QRhiVertexInputAttribute::Float2, 0 }, // Position
            { 1, 1, QRhiVertexInputAttribute::Float2, 0 } // Texture coordinate
        });
        m_pipeLine->setVertexInputLayout(inputLayout);
        m_pipeLine->setRenderPassDescriptor(QSGRenderNodePrivate::get(this)->m_rt.rpDesc);

        m_pipeLine->create();
    }

    QMatrix4x4 mvp = *projectionMatrix();
    QPointF globalPos = m_item->parentItem()->mapToItem(nullptr, QPointF(0, 0));
    mvp.translate(globalPos.x(), globalPos.y());

    float opacity = inheritedOpacity();
    int flipped = rhi->isYUpInFramebuffer() ? 1 : 0;
    resourceUpdates->updateDynamicBuffer(m_uniformBuffer, 0, 64, mvp.constData());
    resourceUpdates->updateDynamicBuffer(m_uniformBuffer, 64, 4, &opacity);
    resourceUpdates->updateDynamicBuffer(m_uniformBuffer, 68, 4, &flipped);

    swapChain->currentFrameCommandBuffer()->resourceUpdate(resourceUpdates);
}
