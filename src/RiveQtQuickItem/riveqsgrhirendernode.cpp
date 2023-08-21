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
{
    QFile file;
    file.setFileName(":/shaders/qt6/finalDraw.vert.qsb");
    file.open(QFile::ReadOnly);
    m_finalDrawShader.append(QRhiShaderStage(QRhiShaderStage::Vertex, QShader::fromSerialized(file.readAll())));

    file.close();
    file.setFileName(":/shaders/qt6/finalDraw.frag.qsb");
    file.open(QFile::ReadOnly);

    m_finalDrawShader.append(QRhiShaderStage(QRhiShaderStage::Fragment, QShader::fromSerialized(file.readAll())));

    file.close();
    file.setFileName(":/shaders/qt6/drawRiveTextureNode.vert.qsb");
    file.open(QFile::ReadOnly);
    m_pathShader.append(QRhiShaderStage(QRhiShaderStage::Vertex, QShader::fromSerialized(file.readAll())));

    file.close();
    file.setFileName(":/shaders/qt6/drawRiveTextureNode.frag.qsb");
    file.open(QFile::ReadOnly);
    m_pathShader.append(QRhiShaderStage(QRhiShaderStage::Fragment, QShader::fromSerialized(file.readAll())));
    file.close();

    file.setFileName(":/shaders/qt6/clipRiveTextureNode.vert.qsb");
    file.open(QFile::ReadOnly);
    m_clipShader.append(QRhiShaderStage(QRhiShaderStage::Vertex, QShader::fromSerialized(file.readAll())));

    file.close();
    file.setFileName(":/shaders/qt6/clipRiveTextureNode.frag.qsb");
    file.open(QFile::ReadOnly);
    m_clipShader.append(QRhiShaderStage(QRhiShaderStage::Fragment, QShader::fromSerialized(file.readAll())));
    file.close();

    file.setFileName(":/shaders/qt6/blendRiveTextureNode.vert.qsb");
    file.open(QFile::ReadOnly);
    m_blendShaders.append(QRhiShaderStage(QRhiShaderStage::Vertex, QShader::fromSerialized(file.readAll())));

    file.close();
    file.setFileName(":/shaders/qt6/blendRiveTextureNode.frag.qsb");
    file.open(QFile::ReadOnly);

    m_blendShaders.append(QRhiShaderStage(QRhiShaderStage::Fragment, QShader::fromSerialized(file.readAll())));
    file.close();

    setRect(geometry);

    m_texCoords.append(QVector2D(0.0f, 0.0f));
    m_texCoords.append(QVector2D(0.0f, 1.0f));
    m_texCoords.append(QVector2D(1.0f, 0.0f));
    m_texCoords.append(QVector2D(1.0f, 1.0f));

    m_renderer = new RiveQtRhiRenderer(window, this);

    m_renderer->updateViewPort(m_rect);
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
    m_renderSurfaceA.cleanUp();
    m_renderSurfaceB.cleanUp();
    m_renderSurfaceIntern.cleanUp();

    m_currentRenderSurface = nullptr;

    if (m_stencilClippingBuffer) {
        m_cleanupList.removeAll(m_stencilClippingBuffer);
        m_stencilClippingBuffer->destroy();
        m_stencilClippingBuffer->deleteLater();
        m_stencilClippingBuffer = nullptr;
    }

    if (m_cleanUpTextureTarget) {
        m_cleanupList.removeAll(m_cleanUpTextureTarget);
        m_cleanUpTextureTarget->destroy();
        m_cleanUpTextureTarget->deleteLater();
        m_cleanUpTextureTarget = nullptr;
    }

    if (m_finalDrawResourceBindingsA) {
        m_cleanupList.removeAll(m_finalDrawResourceBindingsA);
        m_finalDrawResourceBindingsA->destroy();
        m_finalDrawResourceBindingsA->deleteLater();
        m_finalDrawResourceBindingsA = nullptr;
    }

    if (m_finalDrawResourceBindingsB) {
        m_cleanupList.removeAll(m_finalDrawResourceBindingsB);
        m_finalDrawResourceBindingsB->destroy();
        m_finalDrawResourceBindingsB->deleteLater();
        m_finalDrawResourceBindingsB = nullptr;
    }

    m_verticesDirty = true;

    RiveQSGBaseNode::setRect(bounds);
    markDirty(QSGNode::DirtyGeometry);
}

void RiveQSGRHIRenderNode::setFillMode(const RiveRenderSettings::FillMode mode)
{
    m_fillMode = mode;
}

void RiveQSGRHIRenderNode::setPostprocessingMode(const RiveRenderSettings::PostprocessingMode postprocessingMode)
{
    if (m_window->rendererInterface()->graphicsApi() == QSGRendererInterface::GraphicsApi::VulkanRhi) {
        qWarning() << "Post Processing not supported for Vulkan Backend";
        return;
    }

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
    if (!m_renderSurfaceA.valid() || !m_renderSurfaceB.valid() || !m_renderSurfaceIntern.valid() || m_rect.width() == 0
        || m_rect.height() == 0)
        return;

    if (!m_cleanUpTextureTarget) {
        return;
    }

    QSGRendererInterface *renderInterface = m_window->rendererInterface();
    QRhi *rhi = static_cast<QRhi *>(renderInterface->getResource(m_window, QSGRendererInterface::RhiResource));

    m_currentRenderSurface = &m_renderSurfaceA;

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

    commandBuffer->setGraphicsPipeline(m_finalDrawPipeline);
    QSize renderTargetSize = QSGRenderNodePrivate::get(this)->m_rt.rt->pixelSize();
    commandBuffer->setViewport(QRhiViewport(0, 0, renderTargetSize.width(), renderTargetSize.height()));
    commandBuffer->setShaderResources(isCurrentRenderBufferA() ? m_finalDrawResourceBindingsA : m_finalDrawResourceBindingsB);
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

void RiveQSGRHIRenderNode::switchCurrentRenderBuffer()
{
    if (m_currentRenderSurface == &m_renderSurfaceA) {
        m_currentRenderSurface = &m_renderSurfaceB;
    } else {
        m_currentRenderSurface = &m_renderSurfaceA;
    }
}

QRhiTexture *RiveQSGRHIRenderNode::getCurrentRenderBuffer()
{
    if (m_currentRenderSurface) {
        return m_currentRenderSurface->texture;
    }
    return nullptr;
}

QRhiTextureRenderTarget *RiveQSGRHIRenderNode::currentRenderTarget(bool shaderBlending)
{
    if (shaderBlending) {
        return m_renderSurfaceIntern.target;
    }
    return isCurrentRenderBufferA() ? m_renderSurfaceA.target : m_renderSurfaceB.target;
}

QRhiTextureRenderTarget *RiveQSGRHIRenderNode::currentBlendTarget()
{
    return isCurrentRenderBufferA() ? m_renderSurfaceA.blendTarget : m_renderSurfaceB.blendTarget;
}

QRhiGraphicsPipeline *RiveQSGRHIRenderNode::renderPipeline(bool shaderBlending)
{
    if (shaderBlending) {
        return m_drawPipelineIntern;
    }
    return m_drawPipeline;
}

QRhiGraphicsPipeline *RiveQSGRHIRenderNode::clippingPipeline()
{
    return m_clipPipeline;
}

QRhiGraphicsPipeline *RiveQSGRHIRenderNode::currentBlendPipeline()
{
    return m_blendPipeline;
}

QRhiRenderPassDescriptor *RiveQSGRHIRenderNode::currentRenderPassDescriptor(bool shaderBlending)
{
    if (shaderBlending) {
        return m_renderSurfaceIntern.desc;
    }

    return isCurrentRenderBufferA() ? m_renderSurfaceA.desc : m_renderSurfaceB.desc;
}

QRhiRenderPassDescriptor *RiveQSGRHIRenderNode::currentBlendPassDescriptor()
{
    return isCurrentRenderBufferA() ? m_renderSurfaceA.blendDesc : m_renderSurfaceB.blendDesc;
}

QRhiTexture *RiveQSGRHIRenderNode::getRenderBufferA()
{
    return m_renderSurfaceA.texture;
}

QRhiTexture *RiveQSGRHIRenderNode::getRenderBufferB()
{
    return m_renderSurfaceB.texture;
}

QRhiTexture *RiveQSGRHIRenderNode::getRenderBufferIntern()
{
    return m_renderSurfaceIntern.texture;
}

bool RiveQSGRHIRenderNode::isCurrentRenderBufferA()
{
    return m_currentRenderSurface == &m_renderSurfaceA;
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

    if (!m_stencilClippingBuffer) {
        m_stencilClippingBuffer = rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, QSize(m_rect.width(), m_rect.height()), 1);
        m_stencilClippingBuffer->create();
        m_cleanupList.append(m_stencilClippingBuffer);
    }

    bool textureCreated = m_renderSurfaceA.create(rhi, QSize(m_rect.width(), m_rect.height()), m_stencilClippingBuffer);
    m_renderSurfaceB.create(rhi, QSize(m_rect.width(), m_rect.height()), m_stencilClippingBuffer);
    m_renderSurfaceIntern.create(rhi, QSize(m_rect.width(), m_rect.height()), m_stencilClippingBuffer, {});

    // only set the renderSurface to A in case we created a new texture
    if (textureCreated) {
        m_currentRenderSurface = &m_renderSurfaceA;
    }

    if (!m_drawUniformBuffer) {
        m_drawUniformBuffer = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 848);
        m_drawUniformBuffer->create();
        m_cleanupList.append(m_drawUniformBuffer);
    }

    if (!m_clippingUniformBuffer) {
        m_clippingUniformBuffer = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 848);
        m_clippingUniformBuffer->create();
        m_cleanupList.append(m_clippingUniformBuffer);
    }

    if (!m_clippingResourceBindings) {
        m_clippingResourceBindings = rhi->newShaderResourceBindings();
        m_clippingResourceBindings->setBindings({ QRhiShaderResourceBinding::uniformBuffer(
            0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, m_clippingUniformBuffer) });
        m_clippingResourceBindings->create();
        m_cleanupList.append(m_clippingResourceBindings);
    }

    if (!m_sampler) {
        m_sampler = rhi->newSampler(QRhiSampler::Nearest, QRhiSampler::Nearest, QRhiSampler::None, QRhiSampler::ClampToEdge,
                                    QRhiSampler::ClampToEdge);
        m_sampler->create();
        m_cleanupList.append(m_sampler);
    }

    if (!m_blendSampler) {
        m_blendSampler = rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None, QRhiSampler::ClampToEdge,
                                         QRhiSampler::ClampToEdge);
        m_cleanupList.append(m_blendSampler);
        m_blendSampler->create();
    }

    if (!m_drawPipelineResourceBindings) {
        m_drawPipelineResourceBindings = rhi->newShaderResourceBindings();

        m_drawPipelineResourceBindings->setBindings({
            QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
                                                     m_drawUniformBuffer),
            QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, m_qImageTexture, m_sampler) //
        });
        m_drawPipelineResourceBindings->create();
        m_cleanupList.append(m_drawPipelineResourceBindings);
    }

    if (!m_blendResourceBindingsA) {
        m_blendResourceBindingsA = rhi->newShaderResourceBindings();
        m_blendResourceBindingsA->setBindings({
            QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
                                                     m_blendUniformBuffer),
            QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, m_renderSurfaceB.texture,
                                                      m_blendSampler),
            QRhiShaderResourceBinding::sampledTexture(2, QRhiShaderResourceBinding::FragmentStage, m_renderSurfaceIntern.texture,
                                                      m_blendSampler),
        });

        m_blendResourceBindingsA->create();
        m_cleanupList.append(m_blendResourceBindingsA);
    }

    if (!m_blendResourceBindingsB) {
        m_blendResourceBindingsB = rhi->newShaderResourceBindings();
        m_blendResourceBindingsB->setBindings({
            QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
                                                     m_blendUniformBuffer),
            QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, m_renderSurfaceA.texture,
                                                      m_blendSampler),
            QRhiShaderResourceBinding::sampledTexture(2, QRhiShaderResourceBinding::FragmentStage, m_renderSurfaceIntern.texture,
                                                      m_blendSampler),
        });

        m_blendResourceBindingsB->create();
        m_cleanupList.append(m_blendResourceBindingsB);
    }

    if (!m_clipPipeline) {
        m_clipPipeline = createClipPipeline(rhi, m_renderSurfaceA.desc, m_clippingResourceBindings);
    }

    if (!m_drawPipeline) {
        m_drawPipeline = createDrawPipeline(rhi, true, true, m_renderSurfaceA.desc, QRhiGraphicsPipeline::Triangles, m_pathShader,
                                            m_drawPipelineResourceBindings);
    }

    if (!m_drawPipelineIntern) {
        m_drawPipelineIntern = createDrawPipeline(rhi, false, true, m_renderSurfaceIntern.desc, QRhiGraphicsPipeline::Triangles,
                                                  m_pathShader, m_drawPipelineResourceBindings);
    }

    if (!m_blendPipeline) {
        m_blendPipeline = createBlendPipeline(rhi, m_renderSurfaceA.blendDesc, m_blendResourceBindingsA);
    }

    if (m_renderer) {
        m_renderer->updateViewPort(m_rect);
        m_renderer->setRiveRect({ m_topLeftRivePosition, m_riveSize });
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
        QRhiColorAttachment colorAttachment(m_renderSurfaceA.texture);
        QRhiTextureRenderTargetDescription desc(colorAttachment);
        m_cleanUpTextureTarget = rhi->newTextureRenderTarget(desc);
        QRhiRenderPassDescriptor *renderPassDescriptor = m_cleanUpTextureTarget->newCompatibleRenderPassDescriptor();
        m_cleanupList.append(renderPassDescriptor);
        m_cleanUpTextureTarget->setRenderPassDescriptor(renderPassDescriptor);
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
        if (m_postprocessing) {
            m_postprocessing->initializePostprocessingPipeline(rhi, commandBuffer, QSize(m_rect.width(), m_rect.height()),
                                                               isCurrentRenderBufferA() ? m_renderSurfaceA.texture
                                                                                        : m_renderSurfaceB.texture);
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

    if (!m_finalDrawUniformBuffer) {
        m_finalDrawUniformBuffer = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 88);
        m_finalDrawUniformBuffer->create();
        m_cleanupList.append(m_finalDrawUniformBuffer);
    }

    if (!m_finalDrawResourceBindingsA) {
        m_finalDrawResourceBindingsA = rhi->newShaderResourceBindings();

        m_finalDrawResourceBindingsA->setBindings({
            QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
                                                     m_finalDrawUniformBuffer),
            // binding both buffers and decide in the final draw shader which to use.
            QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, getRenderBufferA(), m_sampler),
        });

        m_finalDrawResourceBindingsA->create();
        m_cleanupList.append(m_finalDrawResourceBindingsA);
    }

    if (!m_finalDrawResourceBindingsB) {
        m_finalDrawResourceBindingsB = rhi->newShaderResourceBindings();

        m_finalDrawResourceBindingsB->setBindings({
            QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
                                                     m_finalDrawUniformBuffer),
            // binding both buffers and decide in the final draw shader which to use.
            QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, getRenderBufferB(), m_sampler),
        });

        m_finalDrawResourceBindingsB->create();
        m_cleanupList.append(m_finalDrawResourceBindingsB);
    }

    // this does only seem to work in opengl
    // looks like we need create a postprocess binding and use that in the pipeline and not update the bindings
    if (m_postprocessing) {
        QRhiTexture *postprocessedBuffer = m_postprocessing->getTarget();
        m_finalDrawResourceBindingsA->setBindings({
            QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
                                                     m_finalDrawUniformBuffer),
            QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, postprocessedBuffer, m_sampler),
        });

        m_finalDrawResourceBindingsB->setBindings({
            QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
                                                     m_finalDrawUniformBuffer),
            QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, postprocessedBuffer, m_sampler),
        });
    }

    if (!m_finalDrawPipeline) {
        m_finalDrawPipeline = createDrawPipeline(rhi, true, false, QSGRenderNodePrivate::get(this)->m_rt.rpDesc,
                                                 QRhiGraphicsPipeline::TriangleStrip, m_finalDrawShader, m_finalDrawResourceBindingsA);
    }

    QMatrix4x4 mvp = *projectionMatrix();
    QMatrix4x4 modelMatrix = *matrix();

    mvp = mvp * modelMatrix;

    mvp.translate(-m_rect.x(), -m_rect.y());

    float opacity = inheritedOpacity();
    int flipped = rhi->isYUpInFramebuffer() ? 1 : 0;

    // TODO: is that so? for postprocessing we only have one final texture we use, thats binded to A.
    resourceUpdates->updateDynamicBuffer(m_finalDrawUniformBuffer, 0, 64, mvp.constData());
    resourceUpdates->updateDynamicBuffer(m_finalDrawUniformBuffer, 64, 4, &opacity);
    resourceUpdates->updateDynamicBuffer(m_finalDrawUniformBuffer, 68, 4, &flipped);
    resourceUpdates->updateDynamicBuffer(m_finalDrawUniformBuffer, 72, 4, &left);
    resourceUpdates->updateDynamicBuffer(m_finalDrawUniformBuffer, 76, 4, &right);
    resourceUpdates->updateDynamicBuffer(m_finalDrawUniformBuffer, 80, 4, &top);
    resourceUpdates->updateDynamicBuffer(m_finalDrawUniformBuffer, 84, 4, &bottom);

    commandBuffer->resourceUpdate(resourceUpdates);

    // postprocess display buffer
    if (m_postprocessing) {
        m_postprocessing->postprocess(rhi, commandBuffer, getCurrentRenderBuffer());
    }
}

QRhiGraphicsPipeline *RiveQSGRHIRenderNode::createClipPipeline(QRhi *rhi, QRhiRenderPassDescriptor *renderPassDescriptor,
                                                               QRhiShaderResourceBindings *bindings)
{
    QRhiGraphicsPipeline *clipPipeLine = rhi->newGraphicsPipeline();

    clipPipeLine->setShaderStages(m_clipShader.cbegin(), m_clipShader.cend());
    clipPipeLine->setFlags(QRhiGraphicsPipeline::UsesStencilRef);
    clipPipeLine->setDepthTest(true);
    clipPipeLine->setDepthWrite(true);

    QRhiGraphicsPipeline::TargetBlend disabledColorWrite;
    disabledColorWrite.colorWrite = QRhiGraphicsPipeline::ColorMask(0);
    clipPipeLine->setTargetBlends({ disabledColorWrite });

    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({
        { sizeof(QVector2D) },
        { sizeof(QVector2D) },
    });
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float2, 0 },
        { 1, 1, QRhiVertexInputAttribute::Float2, 0 },
    });

    // Configure stencil operations for writing stencil values
    QRhiGraphicsPipeline::StencilOpState stencilOpState = { QRhiGraphicsPipeline::Keep, QRhiGraphicsPipeline::Keep,
                                                            QRhiGraphicsPipeline::Replace, QRhiGraphicsPipeline::Always };
    clipPipeLine->setStencilFront(stencilOpState);
    clipPipeLine->setStencilBack(stencilOpState);
    clipPipeLine->setStencilTest(true);
    clipPipeLine->setCullMode(QRhiGraphicsPipeline::None);
    clipPipeLine->setTopology(QRhiGraphicsPipeline::Triangles);
    clipPipeLine->setVertexInputLayout(inputLayout);
    clipPipeLine->setRenderPassDescriptor(renderPassDescriptor);

    clipPipeLine->setShaderResourceBindings(bindings);
    clipPipeLine->create();
    m_cleanupList.append(clipPipeLine);

    return clipPipeLine;
}

QRhiGraphicsPipeline *RiveQSGRHIRenderNode::createDrawPipeline(QRhi *rhi, bool srcOverBlend, bool stencilBuffer,
                                                               QRhiRenderPassDescriptor *renderPassDescriptor,
                                                               QRhiGraphicsPipeline::Topology t, const QList<QRhiShaderStage> &shader,
                                                               QRhiShaderResourceBindings *bindings)
{
    QRhiGraphicsPipeline *drawPipeLine = rhi->newGraphicsPipeline();

    //
    // If layer.enabled == true on our QQuickItem, the rendering face is flipped for
    // backends with isYUpInFrameBuffer == true (OpenGL). This does not happen with
    // RHI backends with isYUpInFrameBuffer == false. We swap the triangle winding
    // order to work around this.
    //
    drawPipeLine->setFrontFace(rhi->isYUpInFramebuffer() ? QRhiGraphicsPipeline::CW : QRhiGraphicsPipeline::CCW);
    drawPipeLine->setCullMode(QRhiGraphicsPipeline::None);
    drawPipeLine->setTopology(t);

    if (srcOverBlend) {
        QRhiGraphicsPipeline::TargetBlend blend;
        blend.enable = true;
        blend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
        blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
        blend.srcAlpha = QRhiGraphicsPipeline::One;
        blend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;
        drawPipeLine->setTargetBlends({ blend });
    }

    drawPipeLine->setShaderResourceBindings(bindings);
    drawPipeLine->setShaderStages(shader.cbegin(), shader.cend());

    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({
        { sizeof(QVector2D) }, // Stride for position buffer
        { sizeof(QVector2D) }, // Stride for texture coordinate buffer
    });
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float2, 0 }, // Position1
        { 1, 1, QRhiVertexInputAttribute::Float2, 0 } // Texture coordinate
    });

    drawPipeLine->setVertexInputLayout(inputLayout);
    drawPipeLine->setRenderPassDescriptor(renderPassDescriptor);

    if (stencilBuffer) {
        QRhiGraphicsPipeline::StencilOpState stencilOpState = { QRhiGraphicsPipeline::Keep, QRhiGraphicsPipeline::Keep,
                                                                QRhiGraphicsPipeline::Replace, QRhiGraphicsPipeline::Equal };
        drawPipeLine->setDepthTest(false);
        drawPipeLine->setDepthWrite(false);
        drawPipeLine->setStencilFront(stencilOpState);
        drawPipeLine->setStencilBack(stencilOpState);
        drawPipeLine->setStencilTest(true);
        drawPipeLine->setStencilWriteMask(0);
        drawPipeLine->setFlags(QRhiGraphicsPipeline::UsesStencilRef);
    }

    drawPipeLine->create();
    m_cleanupList.append(drawPipeLine);
    return drawPipeLine;
}

QRhiGraphicsPipeline *RiveQSGRHIRenderNode::createBlendPipeline(QRhi *rhi, QRhiRenderPassDescriptor *renderPass,
                                                                QRhiShaderResourceBindings *bindings)
{
    auto *blendPipeLine = rhi->newGraphicsPipeline();
    m_cleanupList.append(blendPipeLine);
    //
    // If layer.enabled == true on our QQuickItem, the rendering face is flipped for
    // backends with isYUpInFrameBuffer == true (OpenGL). This does not happen with
    // RHI backends with isYUpInFrameBuffer == false. We swap the triangle winding
    // order to work around this.
    //
    blendPipeLine->setFrontFace(rhi->isYUpInFramebuffer() ? QRhiGraphicsPipeline::CW : QRhiGraphicsPipeline::CCW);
    blendPipeLine->setCullMode(QRhiGraphicsPipeline::None);
    blendPipeLine->setTopology(QRhiGraphicsPipeline::TriangleStrip);

    blendPipeLine->setStencilTest(false);
    blendPipeLine->setShaderResourceBindings(bindings);
    blendPipeLine->setShaderStages(m_blendShaders.cbegin(), m_blendShaders.cend());

    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({
        { sizeof(QVector2D) },
        { sizeof(QVector2D) },
    });
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float2, 0 }, // Position1
        { 1, 1, QRhiVertexInputAttribute::Float2, 0 } // Texture coordinate
    });

    blendPipeLine->setVertexInputLayout(inputLayout);
    blendPipeLine->setRenderPassDescriptor(renderPass);

    blendPipeLine->create();
    return blendPipeLine;
}

void RiveQSGRHIRenderNode::RenderSurface::cleanUp()
{
    if (texture) {
        texture->destroy();
        texture->deleteLater();
        texture = nullptr;
    }
    if (desc) {
        desc->destroy();
        desc->deleteLater();
        desc = nullptr;
    }
    if (target) {
        target->destroy();
        target->deleteLater();
        target = nullptr;
    }
    if (blendDesc) {
        blendDesc->destroy();
        blendDesc->deleteLater();
        blendDesc = nullptr;
    }
    if (blendTarget) {
        blendTarget->destroy();
        blendTarget->deleteLater();
        blendTarget = nullptr;
    }
}

bool RiveQSGRHIRenderNode::RenderSurface::create(QRhi *rhi, const QSize &surfaceSize, QRhiRenderBuffer *stencilClippingBuffer,
                                                 QRhiTextureRenderTarget::Flags flags)
{
    bool textureCreated = false;
    if (!texture) {
        texture = rhi->newTexture(QRhiTexture::RGBA8, surfaceSize, 1, QRhiTexture::RenderTarget);
        texture->create();
        textureCreated = true;
    }
    if (!target) {
        QRhiColorAttachment colorAttachment(texture);
        QRhiTextureRenderTargetDescription textureTargetDesc(colorAttachment);
        textureTargetDesc.setDepthStencilBuffer(stencilClippingBuffer);
        target = rhi->newTextureRenderTarget(textureTargetDesc, flags);
        desc = target->newCompatibleRenderPassDescriptor();
        target->setRenderPassDescriptor(desc);
        target->create();
    }
    if (!blendTarget) {
        QRhiColorAttachment colorAttachment(texture);
        QRhiTextureRenderTargetDescription textureTargetDesc(colorAttachment);
        blendTarget = rhi->newTextureRenderTarget(textureTargetDesc);
        blendDesc = blendTarget->newCompatibleRenderPassDescriptor();
        blendTarget->setRenderPassDescriptor(blendDesc);
        blendTarget->create();
    }
    return textureCreated;
}
