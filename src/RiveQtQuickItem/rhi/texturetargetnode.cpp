// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "texturetargetnode.h"

#include <QFile>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include "riveqsgrhirendernode.h"

#include <private/qrhi_p.h>
#include <private/qsgrendernode_p.h>

TextureTargetNode::TextureTargetNode(QQuickWindow *window, RiveQSGRHIRenderNode *node, const QRectF &viewPortRect,
                                     const QMatrix4x4 *combinedMatrix, const QMatrix4x4 *projectionMatrix)
    : m_combinedMatrix(combinedMatrix)
    , m_projectionMatrix(projectionMatrix)
    , m_window(window)
    , m_node(node)
{
    m_blendTexCoords.append(QVector2D(0.0f, 0.0f));
    m_blendTexCoords.append(QVector2D(0.0f, 1.0f));
    m_blendTexCoords.append(QVector2D(1.0f, 0.0f));
    m_blendTexCoords.append(QVector2D(1.0f, 1.0f));

    m_blendVertices.append(QVector2D(viewPortRect.x(), viewPortRect.y()));
    m_blendVertices.append(QVector2D(viewPortRect.x(), viewPortRect.y() + viewPortRect.height()));
    m_blendVertices.append(QVector2D(viewPortRect.x() + viewPortRect.width(), viewPortRect.y()));
    m_blendVertices.append(QVector2D(viewPortRect.x() + viewPortRect.width(), viewPortRect.y() + viewPortRect.height()));

    m_rect = viewPortRect;

    auto *renderInterface = m_window->rendererInterface();
    auto *rhi = static_cast<QRhi *>(renderInterface->getResource(m_window, QSGRendererInterface::RhiResource));

    Q_ASSERT(m_node);

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
    m_geometryData.clear();
    m_clippingData.clear();
    m_texCoordData.clear();
    m_indicesData.clear();
    useGradient = 0;
    m_blendMode = rive::BlendMode::srcOver;
    m_opacity = 1.0f;
    m_clip = false;
    m_shaderBlending = false;

    if (m_qImageTexture && m_useTexture) {
        m_useTexture = false;

        if (m_sampler) {
            m_cleanupList.removeAll(m_sampler);
            m_sampler->destroy();
            delete m_sampler;
            m_sampler = nullptr;
        }

        if (m_qImageTexture) {
            m_cleanupList.removeAll(m_qImageTexture);
            m_qImageTexture->destroy();
            delete m_qImageTexture;
            m_qImageTexture = nullptr;
        }
    }
    m_recycled = true;
}

void TextureTargetNode::releaseResources()
{
    while (!m_cleanupList.empty()) {
        auto *resource = m_cleanupList.first();
        m_cleanupList.removeAll(resource);
        resource->destroy();
        delete resource;
        resource = nullptr;
    }
}

void TextureTargetNode::updateViewport(const QRectF &rect)
{
    releaseResources();

    m_blendVertices.clear();

    m_blendVertices.append(QVector2D(rect.x(), rect.y()));
    m_blendVertices.append(QVector2D(rect.x(), rect.y() + rect.height()));
    m_blendVertices.append(QVector2D(rect.x() + rect.width(), rect.y()));
    m_blendVertices.append(QVector2D(rect.x() + rect.width(), rect.y() + rect.height()));

    m_rect = rect;
    m_blendVerticesDirty = true;
}

void TextureTargetNode::prepareRender()
{
    QSGRendererInterface *renderInterface = m_window->rendererInterface();
    QRhi *rhi = static_cast<QRhi *>(renderInterface->getResource(m_window, QSGRendererInterface::RhiResource));
    Q_ASSERT(rhi);

    m_resourceUpdates = rhi->nextResourceUpdateBatch();

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

    if (!m_useTexture) {
        m_qImageTexture = m_node->getDummyTexture();
    }

    if (m_useTexture && m_qImageTexture) {
        m_resourceUpdates->uploadTexture(m_qImageTexture, m_texture);
    }

    if (m_texCoordBuffer) {
        m_resourceUpdates->uploadStaticBuffer(m_texCoordBuffer, m_texCoordData);
    }

    if (m_indicesBuffer) {
        m_resourceUpdates->uploadStaticBuffer(m_indicesBuffer, m_indicesData);
    }

    // shared buffers / bindings will transfer information into multiple passes
    // this is why each objects needs its own buffers
    if (!m_drawUniformBuffer) {
        m_drawUniformBuffer = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 848);
        m_drawUniformBuffer->create();
        m_cleanupList.append(m_drawUniformBuffer);
    }

    if (!m_clippingUniformBuffer) {
        m_clippingUniformBuffer = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 128);
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
        m_sampler = rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None, QRhiSampler::ClampToEdge,
                                    QRhiSampler::ClampToEdge);
        m_sampler->create();
        m_cleanupList.append(m_sampler);
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

    auto *uniformBuffer = m_drawUniformBuffer;

    // now we setup the shader to draw the path
    float opacity = m_opacity;

    int useTexture = m_qImageTexture != nullptr && m_useTexture; // 76

    // note: the clipping path is provided in global coordinates, not local like the geometry
    // thats why we need to bind another matrix (without the transform) and thats why we have another UniformBuffer here!
    m_resourceUpdates->updateDynamicBuffer(m_clippingUniformBuffer, 0, 64, (*m_combinedMatrix).constData());
    m_resourceUpdates->updateDynamicBuffer(m_clippingUniformBuffer, 64, 64, QMatrix4x4().constData());

    m_resourceUpdates->updateDynamicBuffer(m_drawUniformBuffer, 0, 64, (*m_combinedMatrix).constData());
    m_resourceUpdates->updateDynamicBuffer(m_drawUniformBuffer, 784, 64, m_transform.constData());
    m_resourceUpdates->updateDynamicBuffer(m_drawUniformBuffer, 64, 4, &opacity);
    m_resourceUpdates->updateDynamicBuffer(m_drawUniformBuffer, 76, 4, &useTexture);

    int useGradient = m_gradient != nullptr ? 1 : 0; // 72
    m_resourceUpdates->updateDynamicBuffer(m_drawUniformBuffer, 72, 4, &useGradient);

    if (m_gradient) {
        QVector<QColor> gradientColors; // 144
        QVector<QVector2D> gradientPositions; // 464
        m_resourceUpdates->updateDynamicBuffer(m_drawUniformBuffer, 116, 4, &m_gradientData.gradientType);
        m_resourceUpdates->updateDynamicBuffer(m_drawUniformBuffer, 68, 4, &m_gradientData.gradientRadius);
        m_resourceUpdates->updateDynamicBuffer(m_drawUniformBuffer, 80, 4, &m_gradientData.gradientFocalPointX);
        m_resourceUpdates->updateDynamicBuffer(m_drawUniformBuffer, 84, 4, &m_gradientData.gradientFocalPointY);
        m_resourceUpdates->updateDynamicBuffer(m_drawUniformBuffer, 88, 4, &m_gradientData.gradientCenterX);
        m_resourceUpdates->updateDynamicBuffer(m_drawUniformBuffer, 92, 4, &m_gradientData.gradientCenterY);
        m_resourceUpdates->updateDynamicBuffer(m_drawUniformBuffer, 96, 4, &m_gradientData.startPointX);
        m_resourceUpdates->updateDynamicBuffer(m_drawUniformBuffer, 100, 4, &m_gradientData.startPointY);
        m_resourceUpdates->updateDynamicBuffer(m_drawUniformBuffer, 104, 4, &m_gradientData.endPointX);
        m_resourceUpdates->updateDynamicBuffer(m_drawUniformBuffer, 108, 4, &m_gradientData.endPointY);
        m_resourceUpdates->updateDynamicBuffer(m_drawUniformBuffer, 112, 4, &m_gradientData.numberOfStops);

        int startStopColorsOffset = 144;
        int gradientPositionsOffset = 464;
        for (int i = 0; i < m_gradientData.numberOfStops; ++i) {
            float r = m_gradientData.gradientColors[i].redF();
            float g = m_gradientData.gradientColors[i].greenF();
            float b = m_gradientData.gradientColors[i].blueF();
            float a = m_gradientData.gradientColors[i].alphaF();
            m_resourceUpdates->updateDynamicBuffer(m_drawUniformBuffer, startStopColorsOffset, 4, &r);
            m_resourceUpdates->updateDynamicBuffer(m_drawUniformBuffer, startStopColorsOffset + 4, 4, &g);
            m_resourceUpdates->updateDynamicBuffer(m_drawUniformBuffer, startStopColorsOffset + 8, 4, &b);
            m_resourceUpdates->updateDynamicBuffer(m_drawUniformBuffer, startStopColorsOffset + 12, 4, &a);
            startStopColorsOffset += 16;

            float x = m_gradientData.gradientPositions[i].x();
            float y = m_gradientData.gradientPositions[i].y();
            m_resourceUpdates->updateDynamicBuffer(m_drawUniformBuffer, gradientPositionsOffset, 4, &x);
            m_resourceUpdates->updateDynamicBuffer(m_drawUniformBuffer, gradientPositionsOffset + 4, 4, &y);
            gradientPositionsOffset += 16;
        }
    } else {
        float r = m_color.redF();
        float g = m_color.greenF();
        float b = m_color.blueF();
        float a = m_color.alphaF();
        m_resourceUpdates->updateDynamicBuffer(m_drawUniformBuffer, 128, 4, &r);
        m_resourceUpdates->updateDynamicBuffer(m_drawUniformBuffer, 132, 4, &g);
        m_resourceUpdates->updateDynamicBuffer(m_drawUniformBuffer, 136, 4, &b);
        m_resourceUpdates->updateDynamicBuffer(m_drawUniformBuffer, 140, 4, &a);
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

    prepareRender();

    auto *currentDisplayBufferTarget = m_node->currentRenderTarget(m_shaderBlending);
    auto *clipPipeline = m_node->clippingPipeline();
    auto *drawPipeline = m_node->renderPipeline(m_shaderBlending);

    // it seems we can alter the pass descriptor (we cant change blendmodes or such)
    drawPipeline->setRenderPassDescriptor(m_node->currentRenderPassDescriptor(m_shaderBlending));
    clipPipeline->setRenderPassDescriptor(m_node->currentRenderPassDescriptor(m_shaderBlending));

    commandBuffer->beginPass(currentDisplayBufferTarget, QColor(0, 0, 0, 0), { 1.0f, 0 }, m_resourceUpdates);
    {
        const QSize renderTargetSize = currentDisplayBufferTarget->pixelSize();

        if (m_clip) {
            commandBuffer->setGraphicsPipeline(clipPipeline);
            commandBuffer->setStencilRef(1);
            commandBuffer->setViewport(QRhiViewport(0, 0, renderTargetSize.width(), renderTargetSize.height()));
            commandBuffer->setShaderResources(m_clippingResourceBindings);
            QRhiCommandBuffer::VertexInput clipVertexBindings[] = { { m_clippingVertexBuffer, 0 } };
            commandBuffer->setVertexInput(0, 1, clipVertexBindings);
            commandBuffer->draw(m_clippingData.size() / sizeof(QVector2D));
            commandBuffer->setStencilRef(0);
        }

        commandBuffer->setGraphicsPipeline(drawPipeline);
        commandBuffer->setViewport(QRhiViewport(0, 0, renderTargetSize.width(), renderTargetSize.height()));
        commandBuffer->setShaderResources(m_drawPipelineResourceBindings);

        if (m_qImageTexture && m_indicesBuffer && m_texCoordBuffer && m_useTexture) {
            QRhiCommandBuffer::VertexInput vertexBindings[] = { { m_vertexBuffer, 0 }, { m_texCoordBuffer, 0 } };
            commandBuffer->setVertexInput(0, 2, vertexBindings, m_indicesBuffer, 0, QRhiCommandBuffer::IndexUInt16);
        } else {
            // Some APIs, such as Metal, may raise complaints when a binding for a vertex attribute is missing;
            // in this specific code path, m_texCoordBuffer is nullptr, so if you attempt to bind this buffer,
            // the application will crash. As a workaround, we bind the texture coordinate attribute to the vertex
            // buffer as well. This way, Metal won't encounter any assertions, and the texture coordinates are
            // not needed in this context anyway.
            QRhiCommandBuffer::VertexInput vertexBindings[] = { { m_vertexBuffer, 0 }, { m_vertexBuffer, 0 } };
            commandBuffer->setVertexInput(0, 2, vertexBindings);
        }

        if (m_clip) {
            commandBuffer->setStencilRef(1);
        } else {
            commandBuffer->setStencilRef(0);
        }

        if (m_qImageTexture && m_indicesBuffer && m_useTexture) {
            commandBuffer->drawIndexed(m_indicesBuffer->size() / sizeof(uint16_t));
        } else {
            commandBuffer->draw(m_geometryData.size() / sizeof(QVector2D));
        }
    }
    commandBuffer->endPass();

    if (m_shaderBlending) {
        renderBlend(commandBuffer);
    }
}

void TextureTargetNode::renderBlend(QRhiCommandBuffer *cb)
{
    Q_ASSERT(cb);

    QSGRendererInterface *renderInterface = m_window->rendererInterface();
    QRhi *rhi = static_cast<QRhi *>(renderInterface->getResource(m_window, QSGRendererInterface::RhiResource));
    Q_ASSERT(rhi);

    m_blendResourceUpdates = rhi->nextResourceUpdateBatch();

    if (m_blendVerticesDirty) {
        if (m_blendVertexBuffer) {
            m_cleanupList.removeAll(m_blendVertexBuffer);
            m_blendVertexBuffer->destroy();
            delete m_blendVertexBuffer;
            m_blendVertexBuffer = nullptr;
        }
        m_blendVerticesDirty = false;
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
        m_blendSampler = rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None, QRhiSampler::ClampToEdge,
                                         QRhiSampler::ClampToEdge);
        m_cleanupList.append(m_blendSampler);
        m_blendSampler->create();
    }

    if (m_blendResourceBindingsA) {
        m_cleanupList.removeAll(m_blendResourceBindingsA);
        m_blendResourceBindingsA->destroy();
        delete m_blendResourceBindingsA;
        m_blendResourceBindingsA = nullptr;
    }

    if (m_blendResourceBindingsB) {
        m_cleanupList.removeAll(m_blendResourceBindingsB);
        m_blendResourceBindingsB->destroy();
        delete m_blendResourceBindingsB;
        m_blendResourceBindingsB = nullptr;
    }

    if (!m_blendUniformBuffer) {
        m_blendUniformBuffer = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 80);
        m_cleanupList.append(m_blendUniformBuffer);
        m_blendUniformBuffer->create();
    }

    if (!m_blendResourceBindingsA) {
        m_blendResourceBindingsA = rhi->newShaderResourceBindings();
        m_blendResourceBindingsA->setBindings({
            QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
                                                     m_blendUniformBuffer),
            QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, m_node->getRenderBufferB(),
                                                      m_blendSampler),
            QRhiShaderResourceBinding::sampledTexture(2, QRhiShaderResourceBinding::FragmentStage, m_node->getRenderBufferIntern(),
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
            QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, m_node->getRenderBufferA(),
                                                      m_blendSampler),
            QRhiShaderResourceBinding::sampledTexture(2, QRhiShaderResourceBinding::FragmentStage, m_node->getRenderBufferIntern(),
                                                      m_blendSampler),
        });

        m_blendResourceBindingsB->create();
        m_cleanupList.append(m_blendResourceBindingsB);
    }

    // swap buffer
    m_node->switchCurrentRenderBuffer();

    QMatrix4x4 mvp = (*m_projectionMatrix);
    mvp.translate(-m_rect.x(), -m_rect.y());
    int flipped = rhi->isYUpInFramebuffer() ? 1 : 0;
    m_blendResourceUpdates->updateDynamicBuffer(m_blendUniformBuffer, 0, 64, mvp.constData());
    m_blendResourceUpdates->updateDynamicBuffer(m_blendUniformBuffer, 64, 4, &m_blendMode);
    m_blendResourceUpdates->updateDynamicBuffer(m_blendUniformBuffer, 68, 4, &flipped);

    auto *currentDisplayBufferTarget = m_node->currentBlendTarget();
    auto *blendPipeline = m_node->currentBlendPipeline();
    auto *blendResourceBindings = m_node->isCurrentRenderBufferA() ? m_blendResourceBindingsA : m_blendResourceBindingsB;

    blendPipeline->setRenderPassDescriptor(m_node->currentBlendPassDescriptor());

    cb->beginPass(currentDisplayBufferTarget, QColor(0, 0, 0, 0), { 1.0f, 0 }, m_blendResourceUpdates);
    {
        QSize blendRenderTargetSize = currentDisplayBufferTarget->pixelSize();

        cb->setGraphicsPipeline(blendPipeline);
        cb->setViewport(QRhiViewport(0, 0, blendRenderTargetSize.width(), blendRenderTargetSize.height()));
        cb->setShaderResources(blendResourceBindings);
        QRhiCommandBuffer::VertexInput blendVertexBindings[] = { { m_blendVertexBuffer, 0 }, { m_blendTexCoordBuffer, 0 } };
        cb->setVertexInput(0, 2, blendVertexBindings);

        cb->draw(m_blendVertices.count());
    }
    cb->endPass();
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
    if (m_texture.size() != image.size()) {
        if (m_sampler) {
            m_cleanupList.removeAll(m_sampler);
            m_sampler->destroy();
            delete m_sampler;
            m_sampler = nullptr;
        }

        if (m_qImageTexture) {
            m_cleanupList.removeAll(m_qImageTexture);
            m_qImageTexture->destroy();
            delete m_qImageTexture;
            m_qImageTexture = nullptr;
        }

        //        if (m_resourceBindings) {
        //            m_cleanupList.removeAll(m_resourceBindings);
        //            m_resourceBindings->destroy();
        //            delete m_resourceBindings;
        //            m_resourceBindings = nullptr;
        //        }
    }

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
        m_qImageTexture = rhi->newTexture(QRhiTexture::BGRA8, image.size(), 1);
        m_cleanupList.append(m_qImageTexture);
        m_useTexture = true;
        m_qImageTexture->create();
    }

    if (m_texCoordBuffer) {
        m_cleanupList.removeAll(m_texCoordBuffer);
        m_texCoordBuffer->destroy();
        delete m_texCoordBuffer;
        m_texCoordBuffer = nullptr;
    }

    if (m_indicesBuffer) {
        m_cleanupList.removeAll(m_indicesBuffer);
        m_indicesBuffer->destroy();
        delete m_indicesBuffer;
        m_indicesBuffer = nullptr;
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
    case rive::BlendMode::multiply:
    case rive::BlendMode::darken:
    case rive::BlendMode::lighten:
    case rive::BlendMode::difference:
        m_blendMode = blendMode;
        m_shaderBlending = true;
        break;
    case rive::BlendMode::srcOver:
    default:
        m_blendMode = rive::BlendMode::srcOver;
        m_shaderBlending = false;
        break;
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
