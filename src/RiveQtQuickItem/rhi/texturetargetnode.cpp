// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "texturetargetnode.h"

#include <QFile>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSGRendererInterface>

#include <private/qrhi_p.h>
#include <private/qsgrendernode_p.h>

TextureTargetNode::TextureTargetNode(QQuickItem *item, QRhiTexture *displayBuffer, const QRectF &viewPortRect,
                                     const QMatrix4x4 *combinedMatrix, const QMatrix4x4 *projectionMatrix)
    : m_item(item)
    , m_combinedMatrix(combinedMatrix)
    , m_projectionMatrix(projectionMatrix)
{
    m_window = item->window();

    QFile file;
    file.setFileName(":/shaders/qt6/drawRiveTextureNode.vert.qsb");
    file.open(QFile::ReadOnly);
    m_pathShader.append(QRhiShaderStage(QRhiShaderStage::Vertex, QShader::fromSerialized(file.readAll())));

    file.close();
    file.setFileName(":/shaders/qt6/drawRiveTextureNode.frag.qsb");
    file.open(QFile::ReadOnly);
    m_pathShader.append(QRhiShaderStage(QRhiShaderStage::Fragment, QShader::fromSerialized(file.readAll())));

    file.close();
    file.setFileName(":/shaders/qt6/blendRiveTextureNode.vert.qsb");
    file.open(QFile::ReadOnly);
    m_blendShaders.append(QRhiShaderStage(QRhiShaderStage::Vertex, QShader::fromSerialized(file.readAll())));

    file.close();
    file.setFileName(":/shaders/qt6/blendRiveTextureNode.frag.qsb");
    file.open(QFile::ReadOnly);

    m_blendShaders.append(QRhiShaderStage(QRhiShaderStage::Fragment, QShader::fromSerialized(file.readAll())));

    m_blendTexCoords.append(QVector2D(0.0f, 0.0f));
    m_blendTexCoords.append(QVector2D(0.0f, 1.0f));
    m_blendTexCoords.append(QVector2D(1.0f, 0.0f));
    m_blendTexCoords.append(QVector2D(1.0f, 1.0f));

    m_blendVertices.append(QVector2D(viewPortRect.x(), viewPortRect.y()));
    m_blendVertices.append(QVector2D(viewPortRect.x(), viewPortRect.y() + viewPortRect.height()));
    m_blendVertices.append(QVector2D(viewPortRect.x() + viewPortRect.width(), viewPortRect.y()));
    m_blendVertices.append(QVector2D(viewPortRect.x() + viewPortRect.width(), viewPortRect.y() + viewPortRect.height()));

    m_bounds = viewPortRect;

    auto *renderInterface = m_window->rendererInterface();
    auto *rhi = static_cast<QRhi *>(renderInterface->getResource(m_window, QSGRendererInterface::RhiResource));

    Q_ASSERT(displayBuffer);
    m_displayBuffer = displayBuffer;

    // TODO: make it so that we are not limited to MAX_VERTICES vertices,
    // adjust the buffer dynamically on demand

    if (!m_vertexBuffer) {
        // some large buffer we have here
        m_vertexBuffer = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::VertexBuffer, m_maximumVerticies * sizeof(QVector2D));
        m_cleanupList.append(m_vertexBuffer);
        m_vertexBuffer->create();
    }

    if (!m_clippingVertexBuffer) {
        // some large buffer we have here
        m_clippingVertexBuffer =
            rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::VertexBuffer, m_maximumClippingVerticies * sizeof(QVector2D));
        m_cleanupList.append(m_clippingVertexBuffer);
        m_clippingVertexBuffer->create();
    }

    m_clearData.resize(m_maximumVerticies * sizeof(QVector2D));
    memset(m_clearData.data(), 0, m_maximumVerticies * sizeof(QVector2D));
}

TextureTargetNode::~TextureTargetNode()
{
    while (!m_cleanupList.empty()) {
        auto *resource = m_cleanupList.first();
        m_cleanupList.removeAll(resource);
        resource->destroy();
        delete resource;
    }
}

void TextureTargetNode::recycle()
{
    // this will not revert the pipelines or buffers
    // those will only be reset on need
    m_clip = false;
    m_geometryData.clear();
    m_clippingData.clear();
    m_texCoordData.clear();
    m_indicesData.clear();
    m_recycled = true;
}

void TextureTargetNode::releaseResources()
{
    while (!m_cleanupList.empty()) {
        auto *resource = m_cleanupList.first();
        m_cleanupList.removeAll(resource);
        resource->destroy();
        delete resource;
    }

    m_vertexBuffer = nullptr;
    m_clippingVertexBuffer = nullptr;
    m_uniformBuffer = nullptr;
    m_drawPipeLine = nullptr;
    m_resourceBindings = nullptr;
    m_displayBufferTarget = nullptr;
    m_displayBufferTargetDescriptor = nullptr;

    m_uniformBuffer = nullptr;
    m_texCoordBuffer = nullptr;
    m_indicesBuffer = nullptr;
    m_clippingVertexBuffer = nullptr;
    m_clippingUniformBuffer = nullptr;

    m_clippingResourceBindings = nullptr;

    m_drawPipeLine = nullptr;
    m_blendPipeLine = nullptr;
    m_clipPipeLine = nullptr;

    m_blendTextureRenderTarget = nullptr;
    m_blendRenderDescriptor = nullptr;

    m_stencilClippingBuffer = nullptr;
    m_sampler = nullptr;
    m_qImageTexture = nullptr;

    m_internalDisplayBufferTexture = nullptr;

    m_blendSrc = nullptr;
    m_blendDest = nullptr;

    m_blendVertexBuffer = nullptr;
    m_blendTexCoordBuffer = nullptr;
    m_blendUniformBuffer = nullptr;
    m_blendResourceBindings = nullptr;
    m_blendSampler = nullptr;
    m_resourceUpdates = nullptr;
    m_blendResourceUpdates = nullptr;
}

void TextureTargetNode::updateViewport(const QRectF &bounds, QRhiTexture *displayBuffer)
{
    m_displayBuffer = displayBuffer;

    releaseResources();

    m_blendVertices.clear();

    m_blendVertices.append(QVector2D(bounds.x(), bounds.y()));
    m_blendVertices.append(QVector2D(bounds.x(), bounds.y() + bounds.height()));
    m_blendVertices.append(QVector2D(bounds.x() + bounds.width(), bounds.y()));
    m_blendVertices.append(QVector2D(bounds.x() + bounds.width(), bounds.y() + bounds.height()));

    m_bounds = bounds;
    m_blendVerticesDirty = true;
}

void TextureTargetNode::prepareRender(QRhiCommandBuffer *commandBuffer)
{
    QSGRendererInterface *renderInterface = m_window->rendererInterface();
    QRhi *rhi = static_cast<QRhi *>(renderInterface->getResource(m_window, QSGRendererInterface::RhiResource));
    Q_ASSERT(rhi);

    // This configures our main uniform Buffer
    if (!m_uniformBuffer) {
        m_uniformBuffer = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 848);
        m_uniformBuffer->create();
        m_cleanupList.append(m_uniformBuffer);
    }

    if (!m_resourceBindings) {
        m_resourceBindings = rhi->newShaderResourceBindings();

        if (m_qImageTexture && m_sampler) {
            m_resourceBindings->setBindings({
                QRhiShaderResourceBinding::uniformBuffer(
                    0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, m_uniformBuffer),
                QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, m_qImageTexture, m_sampler) //
            });
        } else {
            m_resourceBindings->setBindings({ QRhiShaderResourceBinding::uniformBuffer(
                0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, m_uniformBuffer) });
        }
        m_resourceBindings->create();
        m_cleanupList.append(m_resourceBindings);
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

    // we create a stencil buffer, even if clipping is not on
    // this my change later, but note that we would need to recreate all depending elements such as the RenderTargetDescription
    if (!m_stencilClippingBuffer) {
        m_stencilClippingBuffer = rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, QSize(m_bounds.width(), m_bounds.height()), 1);
        m_stencilClippingBuffer->create();
        m_cleanupList.append(m_stencilClippingBuffer);
    }

    if (!m_displayBufferTarget) {
        if (!m_shaderBlending) {
            QRhiColorAttachment colorAttachment(m_displayBuffer);
            QRhiTextureRenderTargetDescription desc(colorAttachment);
            desc.setDepthStencilBuffer(m_stencilClippingBuffer);
            m_displayBufferTarget = rhi->newTextureRenderTarget(desc, QRhiTextureRenderTarget::PreserveColorContents);
        } else {
            if (!m_internalDisplayBufferTexture) {
                m_internalDisplayBufferTexture = rhi->newTexture(QRhiTexture::RGBA8, QSize(m_bounds.width(), m_bounds.height()), 1,
                                                                 QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource);
                m_internalDisplayBufferTexture->create();
                m_cleanupList.append(m_internalDisplayBufferTexture);
            }

            QRhiColorAttachment colorAttachment(m_internalDisplayBufferTexture);
            QRhiTextureRenderTargetDescription desc(colorAttachment);
            desc.setDepthStencilBuffer(m_stencilClippingBuffer);
            m_displayBufferTarget = rhi->newTextureRenderTarget(desc);
        }

        m_displayBufferTarget->create();
        m_cleanupList.append(m_displayBufferTarget);
    }

    m_resourceUpdates = rhi->nextResourceUpdateBatch();

    if (!m_displayBufferTargetDescriptor) {
        m_displayBufferTargetDescriptor = m_displayBufferTarget->newCompatibleRenderPassDescriptor();
        m_displayBufferTarget->setRenderPassDescriptor(m_displayBufferTargetDescriptor);
        m_cleanupList.append(m_displayBufferTargetDescriptor);
    }

    if (!m_clipPipeLine) {
        m_clipPipeLine = rhi->newGraphicsPipeline();

        m_clipPipeLine->setShaderStages(m_pathShader.cbegin(), m_pathShader.cend());
        m_clipPipeLine->setFlags(QRhiGraphicsPipeline::UsesStencilRef);
        m_clipPipeLine->setDepthTest(true);
        m_clipPipeLine->setDepthWrite(true);

        QRhiGraphicsPipeline::TargetBlend disabledColorWrite;
        disabledColorWrite.colorWrite = QRhiGraphicsPipeline::ColorMask(0);
        m_clipPipeLine->setTargetBlends({ disabledColorWrite });

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
        m_clipPipeLine->setStencilFront(stencilOpState);
        m_clipPipeLine->setStencilBack(stencilOpState);
        m_clipPipeLine->setStencilTest(true);
        m_clipPipeLine->setCullMode(QRhiGraphicsPipeline::None);
        m_clipPipeLine->setTopology(QRhiGraphicsPipeline::Triangles);
        m_clipPipeLine->setVertexInputLayout(inputLayout);
        m_clipPipeLine->setRenderPassDescriptor(m_displayBufferTargetDescriptor);

        m_clipPipeLine->setShaderResourceBindings(m_clippingResourceBindings);
        m_clipPipeLine->create();
        m_cleanupList.append(m_clipPipeLine);
    }

    if (!m_drawPipeLine) {
        // this line causes a lot of memoy being leaked every frame. Apparently, the m_drawPipeline is null at this point, so it is not
        // cleanly destroyed somewhere else.
        // calling a delete on the pointer
        delete m_drawPipeLine;
        m_drawPipeLine = rhi->newGraphicsPipeline();
        //
        // If layer.enabled == true on our QQuickItem, the rendering face is flipped for
        // backends with isYUpInFrameBuffer == true (OpenGL). This does not happen with
        // RHI backends with isYUpInFrameBuffer == false. We swap the triangle winding
        // order to work around this.
        //
        m_drawPipeLine->setFrontFace(rhi->isYUpInFramebuffer() ? QRhiGraphicsPipeline::CW : QRhiGraphicsPipeline::CCW);
        m_drawPipeLine->setCullMode(QRhiGraphicsPipeline::None);
        m_drawPipeLine->setTopology(QRhiGraphicsPipeline::Triangles);

        if (m_blendMode == rive::BlendMode::srcOver) {
            QRhiGraphicsPipeline::TargetBlend blend;
            blend.enable = true;
            blend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
            blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;

            blend.srcAlpha = QRhiGraphicsPipeline::One;
            blend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;
            m_drawPipeLine->setTargetBlends({ blend });
        }

        if (m_blendMode == rive::BlendMode::darken) {
            QRhiGraphicsPipeline::TargetBlend blend;
            blend.enable = true;
            blend.srcColor = QRhiGraphicsPipeline::One;
            blend.dstColor = QRhiGraphicsPipeline::One;
            blend.opColor = QRhiGraphicsPipeline::Min;
            blend.srcAlpha = QRhiGraphicsPipeline::One;
            blend.dstAlpha = QRhiGraphicsPipeline::One;
            blend.opAlpha = QRhiGraphicsPipeline::Min;
            m_drawPipeLine->setTargetBlends({ blend });
        }

        if (m_blendMode == rive::BlendMode::lighten) {
            QRhiGraphicsPipeline::TargetBlend blend;
            blend.enable = true;
            blend.srcColor = QRhiGraphicsPipeline::One;
            blend.dstColor = QRhiGraphicsPipeline::One;
            blend.opColor = QRhiGraphicsPipeline::Max;
            blend.srcAlpha = QRhiGraphicsPipeline::One;
            blend.dstAlpha = QRhiGraphicsPipeline::One;
            blend.opAlpha = QRhiGraphicsPipeline::Max;
            m_drawPipeLine->setTargetBlends({ blend });
        }

        if (m_blendMode == rive::BlendMode::difference) {
            QRhiGraphicsPipeline::TargetBlend blend;
            blend.enable = true;
            blend.srcColor = QRhiGraphicsPipeline::One;
            blend.dstColor = QRhiGraphicsPipeline::One;
            blend.opColor = QRhiGraphicsPipeline::ReverseSubtract;
            blend.srcAlpha = QRhiGraphicsPipeline::One;
            blend.dstAlpha = QRhiGraphicsPipeline::One;
            blend.opAlpha = QRhiGraphicsPipeline::ReverseSubtract;
            m_drawPipeLine->setTargetBlends({ blend });
        }

        if (m_blendMode == rive::BlendMode::multiply) {
            QRhiGraphicsPipeline::TargetBlend blend;
            blend.enable = true;
            blend.srcColor = QRhiGraphicsPipeline::DstColor;
            blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
            blend.opColor = QRhiGraphicsPipeline::Add;
            blend.srcAlpha = QRhiGraphicsPipeline::One;
            blend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;
            blend.opAlpha = QRhiGraphicsPipeline::Add;
            m_drawPipeLine->setTargetBlends({ blend });
        }

        m_drawPipeLine->setShaderResourceBindings(m_resourceBindings);
        m_drawPipeLine->setShaderStages(m_pathShader.cbegin(), m_pathShader.cend());

        QRhiVertexInputLayout inputLayout;
        inputLayout.setBindings({
            { sizeof(QVector2D) },
            { sizeof(QVector2D) },
        });
        inputLayout.setAttributes({
            { 0, 0, QRhiVertexInputAttribute::Float2, 0 }, // Position1
            { 1, 1, QRhiVertexInputAttribute::Float2, 0 } // Texture coordinate
        });

        m_drawPipeLine->setVertexInputLayout(inputLayout);
        m_drawPipeLine->setRenderPassDescriptor(m_displayBufferTargetDescriptor);

        QRhiGraphicsPipeline::StencilOpState stencilOpState = { QRhiGraphicsPipeline::Keep, QRhiGraphicsPipeline::Keep,
                                                                QRhiGraphicsPipeline::Replace, QRhiGraphicsPipeline::Equal };
        m_drawPipeLine->setDepthTest(false);
        m_drawPipeLine->setDepthWrite(false);
        m_drawPipeLine->setStencilFront(stencilOpState);
        m_drawPipeLine->setStencilBack(stencilOpState);
        m_drawPipeLine->setStencilTest(true);
        m_drawPipeLine->setStencilWriteMask(0);
        m_drawPipeLine->setFlags(QRhiGraphicsPipeline::UsesStencilRef);

        m_drawPipeLine->create();
        m_cleanupList.append(m_drawPipeLine);
    }

    if (m_oldBufferSize > m_geometryData.size()) {
        m_resourceUpdates->updateDynamicBuffer(m_vertexBuffer, 0,
                                               qMin((unsigned long long)m_clearData.size(), m_maximumVerticies * sizeof(QVector2D)),
                                               m_clearData.constData());
        m_resourceUpdates->updateDynamicBuffer(m_clippingVertexBuffer, 0,
                                               qMin((unsigned long long)m_clearData.size(), m_maximumClippingVerticies * sizeof(QVector2D)),
                                               m_clearData.constData());
    } else {
        m_resourceUpdates->updateDynamicBuffer(m_vertexBuffer, 0,
                                               qMin((unsigned long long)m_geometryData.size(), m_maximumVerticies * sizeof(QVector2D)),
                                               m_geometryData.constData());
        m_resourceUpdates->updateDynamicBuffer(
            m_clippingVertexBuffer, 0, qMin((unsigned long long)m_clippingData.size(), m_maximumClippingVerticies * sizeof(QVector2D)),
            m_clippingData.constData());
    }

    if (m_qImageTexture) {
        m_resourceUpdates->uploadTexture(m_qImageTexture, m_texture);
        m_resourceUpdates->uploadStaticBuffer(m_texCoordBuffer, m_texCoordData);
        m_resourceUpdates->uploadStaticBuffer(m_indicesBuffer, m_indicesData);
    }

    // now we setup the shader to draw the path
    float opacity = m_opacity;

    int useTexture = m_qImageTexture != nullptr; // 76

    // note: the clipping path is provided in global coordinates, not local like the geometry
    // thats why we need to bind another matrix (without the transform) and thats why we have another UniformBuffer here!
    m_resourceUpdates->updateDynamicBuffer(m_clippingUniformBuffer, 0, 64, (*m_combinedMatrix).constData());
    m_resourceUpdates->updateDynamicBuffer(m_clippingUniformBuffer, 784, 64, QMatrix4x4().constData());

    m_resourceUpdates->updateDynamicBuffer(m_uniformBuffer, 0, 64, (*m_combinedMatrix).constData());
    m_resourceUpdates->updateDynamicBuffer(m_uniformBuffer, 784, 64, m_transform.constData());
    m_resourceUpdates->updateDynamicBuffer(m_uniformBuffer, 64, 4, &opacity);
    m_resourceUpdates->updateDynamicBuffer(m_uniformBuffer, 76, 4, &useTexture);

    int useGradient = m_gradient != nullptr ? 1 : 0; // 72
    m_resourceUpdates->updateDynamicBuffer(m_uniformBuffer, 72, 4, &useGradient);

    if (m_gradient) {
        QVector<QColor> gradientColors; // 144
        QVector<QVector2D> gradientPositions; // 464
        m_resourceUpdates->updateDynamicBuffer(m_uniformBuffer, 116, 4, &m_gradientData.gradientType);
        m_resourceUpdates->updateDynamicBuffer(m_uniformBuffer, 68, 4, &m_gradientData.gradientRadius);
        m_resourceUpdates->updateDynamicBuffer(m_uniformBuffer, 80, 4, &m_gradientData.gradientFocalPointX);
        m_resourceUpdates->updateDynamicBuffer(m_uniformBuffer, 84, 4, &m_gradientData.gradientFocalPointY);
        m_resourceUpdates->updateDynamicBuffer(m_uniformBuffer, 88, 4, &m_gradientData.gradientCenterX);
        m_resourceUpdates->updateDynamicBuffer(m_uniformBuffer, 92, 4, &m_gradientData.gradientCenterY);
        m_resourceUpdates->updateDynamicBuffer(m_uniformBuffer, 96, 4, &m_gradientData.startPointX);
        m_resourceUpdates->updateDynamicBuffer(m_uniformBuffer, 100, 4, &m_gradientData.startPointY);
        m_resourceUpdates->updateDynamicBuffer(m_uniformBuffer, 104, 4, &m_gradientData.endPointX);
        m_resourceUpdates->updateDynamicBuffer(m_uniformBuffer, 108, 4, &m_gradientData.endPointY);
        m_resourceUpdates->updateDynamicBuffer(m_uniformBuffer, 112, 4, &m_gradientData.numberOfStops);

        int startStopColorsOffset = 144;
        int gradientPositionsOffset = 464;
        for (int i = 0; i < m_gradientData.numberOfStops; ++i) {
            float r = m_gradientData.gradientColors[i].redF();
            float g = m_gradientData.gradientColors[i].greenF();
            float b = m_gradientData.gradientColors[i].blueF();
            float a = m_gradientData.gradientColors[i].alphaF();
            m_resourceUpdates->updateDynamicBuffer(m_uniformBuffer, startStopColorsOffset, 4, &r);
            m_resourceUpdates->updateDynamicBuffer(m_uniformBuffer, startStopColorsOffset + 4, 4, &g);
            m_resourceUpdates->updateDynamicBuffer(m_uniformBuffer, startStopColorsOffset + 8, 4, &b);
            m_resourceUpdates->updateDynamicBuffer(m_uniformBuffer, startStopColorsOffset + 12, 4, &a);
            startStopColorsOffset += 16;

            float x = m_gradientData.gradientPositions[i].x();
            float y = m_gradientData.gradientPositions[i].y();
            m_resourceUpdates->updateDynamicBuffer(m_uniformBuffer, gradientPositionsOffset, 4, &x);
            m_resourceUpdates->updateDynamicBuffer(m_uniformBuffer, gradientPositionsOffset + 4, 4, &y);
            gradientPositionsOffset += 16;
        }
    } else {
        float r = m_color.redF();
        float g = m_color.greenF();
        float b = m_color.blueF();
        float a = m_color.alphaF();
        m_resourceUpdates->updateDynamicBuffer(m_uniformBuffer, 128, 4, &r);
        m_resourceUpdates->updateDynamicBuffer(m_uniformBuffer, 132, 4, &g);
        m_resourceUpdates->updateDynamicBuffer(m_uniformBuffer, 136, 4, &b);
        m_resourceUpdates->updateDynamicBuffer(m_uniformBuffer, 140, 4, &a);
    }
}

void TextureTargetNode::render(QRhiCommandBuffer *commandBuffer)
{
    Q_ASSERT(commandBuffer);

    if (m_recycled) {
        //    commandBuffer->beginPass(m_displayBufferTarget, QColor(0, 0, 0, 0), { 1.0f, 0 });
        //    commandBuffer->endPass();
        return;
    }

    prepareRender(commandBuffer);

    commandBuffer->beginPass(m_displayBufferTarget, QColor(0, 0, 0, 0), { 1.0f, 0 }, m_resourceUpdates);

    const QSize &renderTargetSize = m_displayBufferTarget->pixelSize();

    if (m_clip) {
        // Pass 1
        commandBuffer->setGraphicsPipeline(m_clipPipeLine);
        commandBuffer->setStencilRef(1);
        commandBuffer->setViewport(QRhiViewport(0, 0, renderTargetSize.width(), renderTargetSize.height()));
        commandBuffer->setShaderResources(m_clippingResourceBindings);
        QRhiCommandBuffer::VertexInput clipVertexBindings[] = { { m_clippingVertexBuffer, 0 } };
        commandBuffer->setVertexInput(0, 1, clipVertexBindings);
        commandBuffer->draw(m_clippingData.size() / sizeof(QVector2D));
        commandBuffer->setStencilRef(0);
    }

    // Pass 2
    commandBuffer->setGraphicsPipeline(m_drawPipeLine);
    commandBuffer->setViewport(QRhiViewport(0, 0, renderTargetSize.width(), renderTargetSize.height()));
    commandBuffer->setShaderResources(m_resourceBindings);

    if (m_qImageTexture) {
        QRhiCommandBuffer::VertexInput vertexBindings[] = { { m_vertexBuffer, 0 }, { m_texCoordBuffer, 0 } };
        commandBuffer->setVertexInput(0, 2, vertexBindings, m_indicesBuffer, 0, QRhiCommandBuffer::IndexUInt16);
    } else {
        QRhiCommandBuffer::VertexInput vertexBindings[] = { { m_vertexBuffer, 0 } };
        commandBuffer->setVertexInput(0, 1, vertexBindings);
    }

    if (m_clip) {
        commandBuffer->setStencilRef(1);
    } else {
        commandBuffer->setStencilRef(0);
    }

    if (m_qImageTexture) {
        commandBuffer->drawIndexed(m_indicesBuffer->size() / sizeof(uint16_t));
    } else {
        commandBuffer->draw(m_geometryData.size() / sizeof(QVector2D));
    }

    commandBuffer->endPass();

    renderBlend(commandBuffer);
}

void TextureTargetNode::renderBlend(QRhiCommandBuffer *commandBuffer)
{
    Q_ASSERT(commandBuffer);

    QSGRendererInterface *renderInterface = m_window->rendererInterface();
    QRhi *rhi = static_cast<QRhi *>(renderInterface->getResource(m_window, QSGRendererInterface::RhiResource));
    Q_ASSERT(rhi);

    if (!m_shaderBlending) {
        return;
    }

    m_blendResourceUpdates = rhi->nextResourceUpdateBatch();

    if (m_blendVerticesDirty) {
        if (m_blendVertexBuffer) {
            delete m_blendVertexBuffer;
            m_blendVertexBuffer = nullptr;
        }
        m_blendVerticesDirty = false;
    }

    if (!m_blendSrc) {
        m_blendSrc = rhi->newTexture(QRhiTexture::RGBA8, QSize(m_bounds.width(), m_bounds.height()), 1,
                                     QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource);
        m_blendSrc->create();
        m_cleanupList.append(m_blendSrc);
    }
    m_blendResourceUpdates->copyTexture(m_blendSrc, m_displayBuffer);

    if (!m_blendDest) {
        m_blendDest = rhi->newTexture(QRhiTexture::RGBA8, QSize(m_bounds.width(), m_bounds.height()), 1,
                                      QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource);
        m_blendDest->create();
        m_cleanupList.append(m_blendDest);
    }

    if (m_blendDest && m_internalDisplayBufferTexture) {
        m_blendResourceUpdates->copyTexture(m_blendDest, m_internalDisplayBufferTexture);
    }

    if (!m_blendTextureRenderTarget) {
        QRhiColorAttachment colorAttachment(m_displayBuffer);

        QRhiTextureRenderTargetDescription desc(colorAttachment);
        // desc.setDepthStencilBuffer(m_stencilClippingBuffer);
        m_blendTextureRenderTarget = rhi->newTextureRenderTarget(desc);

        m_cleanupList.append(m_blendTextureRenderTarget);
        m_blendTextureRenderTarget->create();
    }

    if (!m_blendRenderDescriptor) {
        m_blendRenderDescriptor = m_blendTextureRenderTarget->newCompatibleRenderPassDescriptor();
        m_blendTextureRenderTarget->setRenderPassDescriptor(m_blendRenderDescriptor);
        m_cleanupList.append(m_blendRenderDescriptor);
    }

    if (!m_blendVertexBuffer) {
        int blendVertexCount = m_blendVertices.count();

        int blendPositionBufferSize = blendVertexCount * sizeof(QVector2D);
        m_blendVertexBuffer = rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, blendPositionBufferSize);
        m_cleanupList.append(m_blendVertexBuffer);
        m_blendVertexBuffer->create();

        QByteArray blendPositionData;
        blendPositionData.resize(blendPositionBufferSize);
        memcpy(blendPositionData.data(), m_blendVertices.constData(), blendPositionBufferSize);
        m_blendResourceUpdates->uploadStaticBuffer(m_blendVertexBuffer, blendPositionData);

        int blendTexCoordBufferSize = blendVertexCount * sizeof(QVector2D);
        m_blendTexCoordBuffer = rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, blendTexCoordBufferSize);
        m_cleanupList.append(m_blendTexCoordBuffer);
        m_blendTexCoordBuffer->create();

        QByteArray blendTexCoordData;
        blendTexCoordData.resize(blendTexCoordBufferSize);
        memcpy(blendTexCoordData.data(), m_blendTexCoords.constData(), blendTexCoordBufferSize);
        m_blendResourceUpdates->uploadStaticBuffer(m_blendTexCoordBuffer, blendTexCoordData);
    }

    if (!m_blendSampler) {
        m_blendSampler = rhi->newSampler(QRhiSampler::Nearest, QRhiSampler::Nearest, QRhiSampler::None, QRhiSampler::ClampToEdge,
                                         QRhiSampler::ClampToEdge);
        m_cleanupList.append(m_blendSampler);
        m_blendSampler->create();
    }

    if (!m_blendUniformBuffer) {
        m_blendUniformBuffer = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 80);
        m_cleanupList.append(m_blendUniformBuffer);
        m_blendUniformBuffer->create();
    }

    if (!m_blendResourceBindings) {
        m_blendResourceBindings = rhi->newShaderResourceBindings();
        m_blendResourceBindings->setBindings({
            QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
                                                     m_blendUniformBuffer),
            QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, m_blendSrc, m_blendSampler),
            QRhiShaderResourceBinding::sampledTexture(2, QRhiShaderResourceBinding::FragmentStage, m_blendDest, m_blendSampler),
        });

        m_blendResourceBindings->create();
        m_cleanupList.append(m_blendResourceBindings);
    }

    if (!m_blendPipeLine) {
        m_blendPipeLine = rhi->newGraphicsPipeline();
        m_cleanupList.append(m_blendPipeLine);
        //
        // If layer.enabled == true on our QQuickItem, the rendering face is flipped for
        // backends with isYUpInFrameBuffer == true (OpenGL). This does not happen with
        // RHI backends with isYUpInFrameBuffer == false. We swap the triangle winding
        // order to work around this.
        //
        m_blendPipeLine->setFrontFace(rhi->isYUpInFramebuffer() ? QRhiGraphicsPipeline::CW : QRhiGraphicsPipeline::CCW);
        m_blendPipeLine->setCullMode(QRhiGraphicsPipeline::None);
        m_blendPipeLine->setTopology(QRhiGraphicsPipeline::TriangleStrip);

        m_blendPipeLine->setStencilTest(false);
        m_blendPipeLine->setShaderResourceBindings(m_blendResourceBindings);
        m_blendPipeLine->setShaderStages(m_blendShaders.cbegin(), m_blendShaders.cend());

        QRhiVertexInputLayout inputLayout;
        inputLayout.setBindings({
            { sizeof(QVector2D) },
            { sizeof(QVector2D) },
        });
        inputLayout.setAttributes({
            { 0, 0, QRhiVertexInputAttribute::Float2, 0 }, // Position1
            { 1, 1, QRhiVertexInputAttribute::Float2, 0 } // Texture coordinate
        });

        m_blendPipeLine->setVertexInputLayout(inputLayout);
        m_blendPipeLine->setRenderPassDescriptor(m_blendRenderDescriptor);

        m_blendPipeLine->create();
    }

    if (m_shaderBlending) {
        QMatrix4x4 mvp = (*m_projectionMatrix);
        mvp.translate(-m_bounds.x(), -m_bounds.y());
        int flipped = rhi->isYUpInFramebuffer() ? 1 : 0;
        m_blendResourceUpdates->updateDynamicBuffer(m_blendUniformBuffer, 0, 64, mvp.constData());
        m_blendResourceUpdates->updateDynamicBuffer(m_blendUniformBuffer, 64, 4, &m_blendMode);
        m_blendResourceUpdates->updateDynamicBuffer(m_blendUniformBuffer, 68, 4, &flipped);
    }

    commandBuffer->beginPass(m_blendTextureRenderTarget, QColor(0, 0, 0, 0), { 1.0f, 0 }, m_blendResourceUpdates);
    QSize blendRenderTargetSize = m_blendTextureRenderTarget->pixelSize();

    commandBuffer->setGraphicsPipeline(m_blendPipeLine);
    commandBuffer->setViewport(QRhiViewport(0, 0, blendRenderTargetSize.width(), blendRenderTargetSize.height()));
    commandBuffer->setShaderResources(m_blendResourceBindings);
    QRhiCommandBuffer::VertexInput blendVertexBindings[] = { { m_blendVertexBuffer, 0 }, { m_blendTexCoordBuffer, 0 } };
    commandBuffer->setVertexInput(0, 2, blendVertexBindings);

    commandBuffer->draw(m_blendVertices.count());
    commandBuffer->endPass();
}

void TextureTargetNode::setColor(const QColor &color)
{
    m_color = color;
    m_gradient = nullptr;
}

void TextureTargetNode::setOpacity(const float opacity)
{
    m_opacity = opacity;
}

void TextureTargetNode::setClipping(const bool clip)
{
    m_clip = clip;
}

void TextureTargetNode::setGradient(const QGradient *gradient)
{
    m_gradient = gradient;
    m_gradientData.gradientColors.clear();
    m_gradientData.gradientPositions.clear();

    QGradientStops gradientStops = gradient->stops();
    for (const auto &stop : gradientStops) {
        QColor color = stop.second;
        m_gradientData.gradientColors.append(color);
        m_gradientData.gradientPositions.append(QVector2D(stop.first, 0.0f));
    }

    m_gradientData.numberOfStops = gradientStops.count();

    if (gradient->type() == QGradient::LinearGradient) {
        const QLinearGradient *linearGradient = static_cast<const QLinearGradient *>(gradient);
        m_gradientData.startPointX = linearGradient->start().x();
        m_gradientData.startPointY = linearGradient->start().y();
        m_gradientData.endPointX = linearGradient->finalStop().x();
        m_gradientData.endPointY = linearGradient->finalStop().y();
        m_gradientData.gradientType = 0;

    } else if (gradient->type() == QGradient::RadialGradient) {
        const QRadialGradient *radialGradient = static_cast<const QRadialGradient *>(gradient);
        m_gradientData.gradientCenterX = radialGradient->center().x();
        m_gradientData.gradientCenterY = radialGradient->center().y();
        m_gradientData.gradientFocalPointX = radialGradient->focalPoint().x();
        m_gradientData.gradientFocalPointY = radialGradient->focalPoint().y();
        m_gradientData.gradientRadius = radialGradient->radius();
        m_gradientData.gradientType = 1;
    }
}

void TextureTargetNode::setTexture(const QImage &image, RiveQtBufferF32 *qtVertices, RiveQtBufferF32 *qtUvCoords, RiveQtBufferU16 *indices,
                                   const QMatrix4x4 &transform)
{
    m_texture = image;
    m_transform = transform;

    QSGRendererInterface *renderInterface = m_window->rendererInterface();
    QRhi *rhi = static_cast<QRhi *>(renderInterface->getResource(m_window, QSGRendererInterface::RhiResource));
    Q_ASSERT(rhi);

    if (!m_sampler) {
        m_sampler = rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None, QRhiSampler::ClampToEdge,
                                    QRhiSampler::ClampToEdge);
        m_cleanupList.append(m_sampler);
        m_sampler->create();
    }

    if (!m_qImageTexture) {
        m_qImageTexture = rhi->newTexture(QRhiTexture::BGRA8, image.size(), 1, QRhiTexture::UsedAsTransferSource);
        m_cleanupList.append(m_qImageTexture);
        m_qImageTexture->create();
    }

    if (!qtVertices || !qtUvCoords || !indices) {

        QVector<QVector2D> quadVertices = {
            QVector2D(0.0f, 0.0f), // Bottom-left
            QVector2D(0.0f, image.height()), // Bottom-right
            QVector2D(image.width(), 0.0f), // Top-right
            QVector2D(image.width(), image.height()) // Top-left
        };

        QVector<QVector2D> textureCoords = {
            QVector2D(0.0f, 0.0f), // Bottom-left
            QVector2D(0.0f, 1.0f), // Bottom-right
            QVector2D(1.0f, 0.0f), // Top-right
            QVector2D(1.0f, 1.0f) // Top-left
        };

        QVector<uint16_t> indexArray = {
            0, 1, 2, // First triangle (top-right half of the quad)
            1, 3, 2 // Second triangle (bottom-left half of the quad)
        };

        m_geometryData.resize(quadVertices.count() * sizeof(QVector2D));
        memcpy(m_geometryData.data(), quadVertices.constData(), quadVertices.count() * sizeof(QVector2D));

        m_texCoordData.resize(textureCoords.count() * sizeof(QVector2D));
        memcpy(m_texCoordData.data(), textureCoords.constData(), textureCoords.count() * sizeof(QVector2D));

        m_indicesData.resize(indexArray.count() * sizeof(uint16_t));
        memcpy(m_indicesData.data(), indexArray.data(), indexArray.count() * sizeof(uint16_t));

        if (!m_texCoordBuffer) {
            m_texCoordBuffer = rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, textureCoords.count() * sizeof(QVector2D));
            m_cleanupList.append(m_texCoordBuffer);
            m_texCoordBuffer->create();
        }

        if (!m_indicesBuffer) {
            m_indicesBuffer = rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, indexArray.count() * sizeof(uint16_t));
            m_cleanupList.append(m_indicesBuffer);
            m_indicesBuffer->create();
        }

    } else {
        // TODO COPY float std vector directly... this here is kind of the most stupid way to handle it...
        QVector<QVector2D> quadVertices;
        for (int i = 0; i < qtVertices->count(); i += 2) {
            quadVertices.append(QVector2D(qtVertices->data()[i], qtVertices->data()[i + 1]));
        }

        QVector<QVector2D> textureCoords;
        for (int i = 0; i < qtUvCoords->count(); i += 2) {
            textureCoords.append(QVector2D(qtUvCoords->data()[i], qtUvCoords->data()[i + 1]));
        }

        QVector<uint16_t> indexArray;
        for (int i = 0; i < indices->count(); i++) {
            indexArray.append(indices->data()[i]);
        }

        m_geometryData.resize(quadVertices.count() * sizeof(QVector2D));
        memcpy(m_geometryData.data(), quadVertices.constData(), quadVertices.count() * sizeof(QVector2D));

        m_texCoordData.resize(textureCoords.count() * sizeof(QVector2D));
        memcpy(m_texCoordData.data(), textureCoords.constData(), textureCoords.count() * sizeof(QVector2D));

        m_indicesData.resize(indexArray.count() * sizeof(uint16_t));
        memcpy(m_indicesData.data(), indexArray.data(), indexArray.count() * sizeof(uint16_t));

        if (!m_texCoordBuffer) {
            m_texCoordBuffer = rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, textureCoords.count() * sizeof(QVector2D));
            m_cleanupList.append(m_texCoordBuffer);
            m_texCoordBuffer->create();
        }

        if (!m_indicesBuffer) {
            m_indicesBuffer = rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, indexArray.count() * sizeof(uint16_t));
            m_cleanupList.append(m_indicesBuffer);
            m_indicesBuffer->create();
        }
    }
}

void TextureTargetNode::setBlendMode(rive::BlendMode blendMode)
{
    if (m_blendMode == blendMode) {
        return;
    }

    bool lastShaderBlending = m_shaderBlending;

    switch (blendMode) {
    case rive::BlendMode::colorDodge:
    case rive::BlendMode::overlay:
    case rive::BlendMode::colorBurn:
    case rive::BlendMode::softLight:
    case rive::BlendMode::hardLight:
    case rive::BlendMode::exclusion:
    case rive::BlendMode::hue:
    case rive::BlendMode::saturation:
    case rive::BlendMode::color:
    case rive::BlendMode::luminosity:
    case rive::BlendMode::screen:
        m_blendMode = blendMode;
        m_shaderBlending = true; // intentional fallthrough
        break;
    case rive::BlendMode::srcOver:
    case rive::BlendMode::darken:
    case rive::BlendMode::lighten:
    case rive::BlendMode::difference:
    case rive::BlendMode::multiply:
    default:
        m_shaderBlending = false;
        m_blendMode = qMax(rive::BlendMode::srcOver, qMin(rive::BlendMode::luminosity, blendMode));
        break;
    }

    if (lastShaderBlending != m_shaderBlending) {
        if (m_drawPipeLine) {
            m_cleanupList.removeAll(m_drawPipeLine);
            m_drawPipeLine->destroy();
            m_drawPipeLine = nullptr;
        }

        if (m_blendPipeLine) {
            m_cleanupList.removeAll(m_blendPipeLine);
            m_blendPipeLine->destroy();
            m_blendPipeLine = nullptr;
        }

        if (m_stencilClippingBuffer) {
            m_cleanupList.removeAll(m_stencilClippingBuffer);
            m_stencilClippingBuffer->destroy();
            m_stencilClippingBuffer = nullptr;
        }

        if (m_displayBufferTarget) {
            m_cleanupList.removeAll(m_displayBufferTarget);
            m_displayBufferTarget->destroy();
            m_displayBufferTarget = nullptr;
        }

        if (m_displayBufferTargetDescriptor) {
            m_cleanupList.removeAll(m_displayBufferTargetDescriptor);
            m_displayBufferTargetDescriptor->destroy();
            m_displayBufferTargetDescriptor = nullptr;
        }

        if (m_clipPipeLine) {
            m_cleanupList.removeAll(m_clipPipeLine);
            m_clipPipeLine->destroy();
            m_clipPipeLine = nullptr;
        }
    }

    // maybe it would be better to have several pipelines ready and just select what we need on render
    // for now destroy on blend change and recreate
    if (!m_shaderBlending) {
        if (m_drawPipeLine) {
            m_cleanupList.removeAll(m_drawPipeLine);
            m_drawPipeLine->destroy();
            m_drawPipeLine = nullptr;
        }
        if (m_blendPipeLine) {
            m_cleanupList.removeAll(m_blendPipeLine);
            m_blendPipeLine->destroy();
            m_blendPipeLine = nullptr;
        }
    }
}

void TextureTargetNode::updateGeometry(const QVector<QVector<QVector2D>> &geometry, const QMatrix4x4 &transform)
{
    m_transform = transform;

    m_oldBufferSize = m_geometryData.size();

    int vertexCount = 0;
    for (const auto &segment : qAsConst(geometry)) {
        vertexCount += segment.count();
    }

    // Check if we need to resize the vertex buffer
    if (vertexCount > m_maximumVerticies) {
        auto *renderInterface = m_window->rendererInterface();
        auto *rhi = static_cast<QRhi *>(renderInterface->getResource(m_window, QSGRendererInterface::RhiResource));
        m_maximumVerticies = vertexCount;
        if (m_vertexBuffer) {
            // Destroy old buffer and remove from cleanup list
            m_cleanupList.removeAll(m_vertexBuffer);
            m_vertexBuffer->destroy();
        }

        // Create new buffer with updated size
        m_vertexBuffer = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::VertexBuffer, vertexCount * sizeof(QVector2D));
        m_cleanupList.append(m_vertexBuffer);
        m_vertexBuffer->create();

        // prepare clearing data, make as much as we need to share them
        m_clearData.resize(std::max(m_maximumVerticies, m_maximumClippingVerticies) * sizeof(QVector2D));
        memset(m_clearData.data(), 0, std::max(m_maximumVerticies, m_maximumClippingVerticies) * sizeof(QVector2D));
    }

    m_geometryData.clear();
    m_geometryData.resize(vertexCount * sizeof(QVector2D));

    int offset = 0;
    for (const auto &segment : qAsConst(geometry)) {
        if (segment.empty()) {
            continue;
        }

        memcpy(m_geometryData.data() + offset, segment.constData(), segment.count() * sizeof(QVector2D));
        offset += (segment.count() * sizeof(QVector2D));
    }
}

void TextureTargetNode::updateClippingGeometry(const QVector<QVector<QVector2D>> &clippingGeometry)
{
    setClipping(!clippingGeometry.empty());

    int vertexCount = 0;
    for (const auto &segment : qAsConst(clippingGeometry)) {
        vertexCount += segment.count();
    }

    // Check if we need to resize the vertex buffer
    if (vertexCount > m_maximumClippingVerticies) {
        auto *renderInterface = m_window->rendererInterface();
        auto *rhi = static_cast<QRhi *>(renderInterface->getResource(m_window, QSGRendererInterface::RhiResource));
        m_maximumClippingVerticies = vertexCount;
        if (m_clippingVertexBuffer) {
            // Destroy old buffer and remove from cleanup list
            m_cleanupList.removeAll(m_clippingVertexBuffer);
            m_clippingVertexBuffer->destroy();
        }

        // Create new buffer with updated size
        m_clippingVertexBuffer = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::VertexBuffer, vertexCount * sizeof(QVector2D));
        m_cleanupList.append(m_clippingVertexBuffer);
        m_clippingVertexBuffer->create();

        // prepare clearing data, make as much as we need to share them
        m_clearData.resize(std::max(m_maximumVerticies, m_maximumClippingVerticies) * sizeof(QVector2D));
        memset(m_clearData.data(), 0, std::max(m_maximumVerticies, m_maximumClippingVerticies) * sizeof(QVector2D));
    }

    m_clippingData.resize(vertexCount * sizeof(QVector2D));

    int offset = 0;
    for (const auto &segment : qAsConst(clippingGeometry)) {
        if (segment.empty()) {
            continue;
        }

        memcpy(m_clippingData.data() + offset, segment.constData(), segment.count() * sizeof(QVector2D));
        offset += (segment.count() * sizeof(QVector2D));
    }
}
