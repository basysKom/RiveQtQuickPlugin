// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <QQuickWindow>
#include <QFile>

#include <private/qrhi_p.h>
#include <private/qsgrendernode_p.h>

#include "riveqsgrhirendernode.h"
#include "riveqtquickitem.h"
#include "renderer/riveqtrhirenderer.h"

#include "rhi/postprocessingsmaa.h"

RiveQSGRHIRenderNode::RiveQSGRHIRenderNode(QQuickWindow *window, std::weak_ptr<rive::ArtboardInstance> artboardInstance,
                                           const QRectF &geometry)
    : RiveQSGRenderNode(window, artboardInstance, geometry)
    , m_displayBuffer(nullptr)
{
    QFile file;
    file.setFileName(":/shaders/qt6/finalDraw.vert.qsb");
    file.open(QFile::ReadOnly);
    m_shaders.append(QRhiShaderStage(QRhiShaderStage::Vertex, QShader::fromSerialized(file.readAll())));

    file.close();
    file.setFileName(":/shaders/qt6/finalDraw.frag.qsb");
    file.open(QFile::ReadOnly);

    m_shaders.append(QRhiShaderStage(QRhiShaderStage::Fragment, QShader::fromSerialized(file.readAll())));

    setRect(geometry);

    m_texCoords.append(QVector2D(0.0f, 0.0f));
    m_texCoords.append(QVector2D(0.0f, 1.0f));
    m_texCoords.append(QVector2D(1.0f, 0.0f));
    m_texCoords.append(QVector2D(1.0f, 1.0f));

    m_renderer = new RiveQtRhiRenderer(window);
    m_renderer->updateViewPort(m_rect, m_displayBuffer);
    m_renderer->setRiveRect({ m_topLeftRivePosition, m_riveSize });

    setPostprocessingMode(RiveRenderSettings::SMAA);
}

RiveQSGRHIRenderNode::~RiveQSGRHIRenderNode()
{
    delete m_renderer;

    if (m_postprocessing) {
        m_postprocessing->cleanup();
        delete m_postprocessing;
        m_postprocessing = nullptr;
    }

    releaseResources();
}

void RiveQSGRHIRenderNode::setRect(const QRectF &bounds)
{
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
        m_displayBuffer->deleteLater();
        m_displayBuffer = nullptr;
    }

    if (m_sampler) {
        m_cleanupList.removeAll(m_sampler);
        m_sampler->destroy();
        m_sampler->deleteLater();
        m_sampler = nullptr;
    }

    if (m_cleanUpTextureTarget) {
        m_cleanupList.removeAll(m_cleanUpTextureTarget);
        m_cleanUpTextureTarget->destroy();
        m_cleanUpTextureTarget->deleteLater();
        m_cleanUpTextureTarget = nullptr;
    }

    // this potentialy crashes :/
    if (m_resourceBindings) {
        m_cleanupList.removeAll(m_resourceBindings);
        m_resourceBindings->destroy();
        m_resourceBindings->deleteLater();
        m_resourceBindings = nullptr;
    }

    m_verticesDirty = true;

    RiveQSGBaseNode::setRect(bounds);
    markDirty(QSGNode::DirtyGeometry);
}

void  RiveQSGRHIRenderNode::setPostprocessingMode(const RiveRenderSettings::PostprocessingMode postprocessingMode)
{
    if (postprocessingMode == RiveRenderSettings::None) {
        if (m_postprocessing) {
            m_postprocessing->cleanup();
            delete m_postprocessing;
            m_postprocessing = nullptr;
        }
    } else { // if (postprocessingMode == RiveRenderSettings::SMAA) {
        if (!m_postprocessing) {
            m_postprocessing = new PostprocessingSMAA(); 
        } 
    } 
}

void RiveQSGRHIRenderNode::renderOffscreen()
{
    if (!m_displayBuffer || m_rect.width() == 0 || m_rect.height() == 0)
        return;

    if (!m_cleanUpTextureTarget) {
        return;
    }

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
    if (m_artboardInstance.expired()) {
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
        resource->destroy();
        resource->deleteLater();
        m_cleanupList.removeAll(resource);
        resource = nullptr;
    }

    Q_ASSERT(m_cleanupList.empty());
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
    if (!m_window) {
        return;
    }

    QSGRendererInterface *renderInterface = m_window->rendererInterface();
    QRhiSwapChain *swapChain =
        static_cast<QRhiSwapChain *>(renderInterface->getResource(m_window, QSGRendererInterface::RhiSwapchainResource));
    QRhi *rhi = static_cast<QRhi *>(renderInterface->getResource(m_window, QSGRendererInterface::RhiResource));
    Q_ASSERT(swapChain);
    Q_ASSERT(rhi);

    QRhiCommandBuffer *commandBuffer = swapChain->currentFrameCommandBuffer();

    if (!m_displayBuffer) {
        m_displayBuffer = rhi->newTexture(QRhiTexture::RGBA8, QSize(m_rect.width(), m_rect.height()), 1,
                                          QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource);
        m_displayBuffer->create();
        m_cleanupList.append(m_displayBuffer);

        if (m_renderer) {
            m_renderer->updateViewPort(m_rect, m_displayBuffer);
            m_renderer->setRiveRect({ m_topLeftRivePosition, m_riveSize });
        }

        if (m_postprocessing) {
            // ToDo: m_display_buffer should be std::shared_ptr
            m_postprocessing->initializePostprocessingPipeline(rhi, commandBuffer, QSize(m_rect.width(), m_rect.height()), m_displayBuffer);
        }

    }

    if (m_artboardInstance.expired()) {
        return;
    }

    auto artboardInstance = m_artboardInstance.lock();

    if (!m_renderer) {
        qWarning() << "Renderer is null";
        return;
    }

    m_renderer->recycleRiveNodes();

    m_renderer->updateArtboardSize(QSize(artboardInstance->width(), artboardInstance->height()));

    // value range of 0 to 1 to describe the edges of the artboard on the final render ("clip the final texture")
    float left = 0; // 0-1
    float right = 1; // 0-1
    float top = 0; // 0-1
    float bottom = 1; // 0-1

    { // update projection matrix
        QMatrix4x4 projMatrix = *projectionMatrix();

        const auto window2itemScaleX = m_window->width() / m_rect.width();
        const auto window2itemScaleY = m_window->height() / m_rect.height();

        projMatrix.scale(window2itemScaleX, window2itemScaleY);

        QMatrix4x4 combinedMatrix = projMatrix;

        combinedMatrix.translate(m_topLeftRivePosition.x(), m_topLeftRivePosition.y());

        const auto item2artboardScaleX = m_rect.width() / artboardInstance->width();
        const auto item2artboardScaleY = m_rect.height() / artboardInstance->height();

        switch (m_fillMode) {
        case RiveRenderSettings::Stretch: {
            combinedMatrix.scale(item2artboardScaleX, item2artboardScaleY);
            break;
        }
        case RiveRenderSettings::PreserveAspectCrop: {
            const auto scaleFactor = qMax(item2artboardScaleX, item2artboardScaleY);
            combinedMatrix.scale(scaleFactor, scaleFactor);
            break;
        }
        default:
        case RiveRenderSettings::PreserveAspectFit: {
            const auto scaleFactor = qMin(item2artboardScaleX, item2artboardScaleY);

            float widthFactor = m_rect.width() - artboardInstance->width() * scaleFactor;
            float heightFactor = m_rect.height() - artboardInstance->height() * scaleFactor;

            if (widthFactor > 0) {
                // diff through 2 since its centered horizontal
                widthFactor = (widthFactor / 2.0f) * 1.0f / m_rect.width();
                left = widthFactor;
                right = 1.0 - widthFactor;
            }

            if (heightFactor > 0) {
                // diff through 2 since its centered vertical
                heightFactor = (heightFactor / 2.0f) * 1.0f / m_rect.height();
                top = heightFactor;
                bottom = 1.0 - heightFactor;
            }

            combinedMatrix.scale(scaleFactor, scaleFactor);
            break;
        }
        }

        m_renderer->setProjectionMatrix(&projMatrix, &combinedMatrix);
    }

    artboardInstance->draw(m_renderer);

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
            m_cleanupList.removeAll(m_vertexBuffer);
            m_vertexBuffer->destroy();
            m_vertexBuffer->deleteLater();
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
        m_uniformBuffer = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 88);
        m_uniformBuffer->create();
        m_cleanupList.append(m_uniformBuffer);
    }

    if (!m_resourceBindings) {
        m_resourceBindings = rhi->newShaderResourceBindings();
        if (m_postprocessing) {
            QRhiTexture *postprocessedBuffer = m_postprocessing->getTarget();
            m_resourceBindings->setBindings({
                QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
                                                        m_uniformBuffer),
                QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, postprocessedBuffer, m_sampler),
            });
        } else {
            m_resourceBindings->setBindings({
                QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
                                                        m_uniformBuffer),
                QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, m_displayBuffer, m_sampler),
            });
        }

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
    QMatrix4x4 modelMatrix = *matrix();

    mvp = mvp * modelMatrix;

    mvp.translate(-m_rect.x(), -m_rect.y());

    float opacity = inheritedOpacity();
    int flipped = rhi->isYUpInFramebuffer() ? 1 : 0;

    resourceUpdates->updateDynamicBuffer(m_uniformBuffer, 0, 64, mvp.constData());
    resourceUpdates->updateDynamicBuffer(m_uniformBuffer, 64, 4, &opacity);
    resourceUpdates->updateDynamicBuffer(m_uniformBuffer, 68, 4, &flipped);
    resourceUpdates->updateDynamicBuffer(m_uniformBuffer, 72, 4, &left);
    resourceUpdates->updateDynamicBuffer(m_uniformBuffer, 76, 4, &right);
    resourceUpdates->updateDynamicBuffer(m_uniformBuffer, 80, 4, &top);
    resourceUpdates->updateDynamicBuffer(m_uniformBuffer, 84, 4, &bottom);

    commandBuffer->resourceUpdate(resourceUpdates);

    // postprocess display buffer
    if (m_postprocessing) {
        m_postprocessing->postprocess(rhi, commandBuffer);
    }
}
