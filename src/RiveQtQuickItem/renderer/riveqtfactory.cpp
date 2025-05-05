// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "renderer/riveqtfactory.h"
#include "renderer/riveqtpainterrenderer.h"
#include "riveqsgrendernode.h"
#include "riveqsgrhirendernode.h"
#include "riveqsgsoftwarerendernode.h"
#include "riveqtpath.h"
#include "riveqtutils.h"
#include "rqqplogging.h"
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#    include "renderer/riveqtrhirenderer.h"
#endif

#include <QQuickWindow>

#include <utils/factory_utils.hpp>

RiveQtFactory::RiveQtFactory(const RiveRenderSettings &renderSettings)
    : rive::Factory()
    , m_renderSettings(renderSettings)
{
}

void RiveQtFactory::setRenderSettings(const RiveRenderSettings &renderSettings)
{
    m_renderSettings = renderSettings;
}

RiveQSGRenderNode *RiveQtFactory::renderNode(QQuickWindow *window, std::weak_ptr<rive::ArtboardInstance> artboardInstance,
                                             const QRectF &geometry)
{
    switch (window->rendererInterface()->graphicsApi()) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    case QSGRendererInterface::GraphicsApi::OpenGLRhi:
    case QSGRendererInterface::GraphicsApi::MetalRhi:
    case QSGRendererInterface::GraphicsApi::VulkanRhi:
    case QSGRendererInterface::GraphicsApi::Direct3D11Rhi: {
        auto sampleCount = 1;
        QSGRendererInterface *renderInterface = window->rendererInterface();
        QRhi *rhi = static_cast<QRhi *>(renderInterface->getResource(window, QSGRendererInterface::RhiResource));
        const QRhiSwapChain *swapChain =
            static_cast<QRhiSwapChain *>(renderInterface->getResource(window, QSGRendererInterface::RhiSwapchainResource));

        auto sampleCounts = rhi->supportedSampleCounts();

        if (swapChain) {
            sampleCount = swapChain->sampleCount();
        } else {
            // maybe an offscreen render target is active;
            // this is the case if the Rive scene is rendered
            // inside an QQuickWidget
            // try a different way to fetch the sample count
            const auto redirectRenderTarget =
                static_cast<QRhiTextureRenderTarget *>(renderInterface->getResource(window, QSGRendererInterface::RhiRedirectRenderTarget));
            if (redirectRenderTarget) {
                sampleCount = redirectRenderTarget->sampleCount();
            } else {
                qCCritical(rqqpFactory)
                    << "Swap chain or offscreen render target not found for given window: rendering may be faulty.";
            }
        }

        if (sampleCount == 1 || (sampleCount > 1 
            && rhi->isFeatureSupported(QRhi::MultisampleRenderBuffer)
            && sampleCounts.contains(sampleCount))) {
            auto node = new RiveQSGRHIRenderNode(window, artboardInstance, geometry);
            node->setFillMode(m_renderSettings.fillMode);
            node->setPostprocessingMode(m_renderSettings.postprocessingMode);
            return node;
        } else {
            qCCritical(rqqpFactory)
                << "MSAA requested, but requested sample size is not supported - requested sample size:" << sampleCount;
            return nullptr;
        }
    }
#endif
    case QSGRendererInterface::GraphicsApi::Software:
    default:
        return new RiveQSGSoftwareRenderNode(window, artboardInstance, geometry);
    }
}

rive::rcp<rive::RenderBuffer> RiveQtFactory::makeRenderBuffer(rive::RenderBufferType renderBufferType, rive::RenderBufferFlags renderBufferFlags, size_t size)
{
    return rive::make_rcp<rive::DataRenderBuffer>(renderBufferType, renderBufferFlags, size);
}

rive::rcp<rive::RenderShader> RiveQtFactory::makeLinearGradient(float x1, float y1, float x2, float y2, const rive::ColorInt *colors,
                                                                const float *stops, size_t count)
{
    auto shader = new RiveQtLinearGradient(x1, y1, x2, y2, colors, stops, count);
    return rive::rcp<rive::RenderShader>(shader);
}

rive::rcp<rive::RenderShader> RiveQtFactory::makeRadialGradient(float centerX, float centerY, float radius, const rive::ColorInt colors[],
                                                                const float positions[], size_t count)
{
    auto shader = new RiveQtRadialGradient(centerX, centerY, radius, colors, positions, count);
    return rive::rcp<rive::RenderShader>(shader);
}

rive::rcp<rive::RenderPath> RiveQtFactory::makeRenderPath(rive::RawPath &rawPath, rive::FillRule fillRule)
{
    switch (renderType()) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    case RiveQtRenderType::RHIRenderer: {
        rive::rcp<RiveQtPath> riveQtPath = rive::make_rcp<RiveQtPath>(rawPath, fillRule);
        riveQtPath->setLevelOfDetail(levelOfDetail());
        return riveQtPath;
    }
#endif
    case RiveQtRenderType::QPainterRenderer:
    case RiveQtRenderType::None:
    default:
        return rive::make_rcp<RiveQtPainterPath>(rawPath, fillRule); // TODO Add Empty Path
    }
}

rive::rcp<rive::RenderPath> RiveQtFactory::makeEmptyRenderPath()
{
    switch (renderType()) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    case RiveQtRenderType::RHIRenderer: {
        rive::rcp<RiveQtPath> riveQtPath = rive::make_rcp<RiveQtPath>();
        riveQtPath->setLevelOfDetail(levelOfDetail());
        return riveQtPath;
    }
#endif
    case RiveQtRenderType::QPainterRenderer:
    case RiveQtRenderType::None:
    default:
        return rive::make_rcp<RiveQtPainterPath>(); // TODO Add Empty Path
    }
}

rive::rcp<rive::RenderPaint> RiveQtFactory::makeRenderPaint()
{
    return rive::make_rcp<RiveQtPaint>();
}

rive::rcp<rive::RenderImage> RiveQtFactory::decodeImage(rive::Span<const uint8_t> span)
{
    QByteArray imageData(reinterpret_cast<const char *>(span.data()), static_cast<int>(span.size()));
    QImage image = QImage::fromData(imageData);

    if (image.isNull()) {
        return nullptr;
    }
    return rive::rcp<RiveQtImage>(new RiveQtImage(image));
}

unsigned int RiveQtFactory::levelOfDetail()
{
    switch (m_renderSettings.renderQuality) {
    case RiveRenderSettings::Low:
        return 1;
    case RiveRenderSettings::High:
        return 10;
    case RiveRenderSettings::Medium:
    default:
        return 5;
    }
}

RiveQtFactory::RiveQtRenderType RiveQtFactory::renderType()
{
    switch (m_renderSettings.graphicsApi) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    case QSGRendererInterface::GraphicsApi::Direct3D11Rhi:
    case QSGRendererInterface::GraphicsApi::OpenGLRhi:
    case QSGRendererInterface::GraphicsApi::MetalRhi:
    case QSGRendererInterface::GraphicsApi::VulkanRhi:
        return RiveQtFactory::RiveQtRenderType::RHIRenderer;
#endif
    case QSGRendererInterface::GraphicsApi::Software:
    default:
        return RiveQtFactory::RiveQtRenderType::QPainterRenderer;
    }
}
