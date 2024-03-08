
// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later
#include <QQuickWindow>

#include "renderer/riveqtfactory.h"
#include "renderer/riveqtfont.h"
#include "renderer/riveqtpainterrenderer.h"
#include "riveqsgrhirendernode.h"
#include "riveqsgsoftwarerendernode.h"
#include "riveqtpath.h"
#include "rqqplogging.h"

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#    include "renderer/riveqtrhirenderer.h"
#endif

RiveQSGRenderNode *RiveQtFactory::renderNode(QQuickWindow *window, std::weak_ptr<rive::ArtboardInstance> artboardInstance,
                                             const QRectF &geometry)
{
    switch (window->rendererInterface()->graphicsApi()) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    case QSGRendererInterface::GraphicsApi::OpenGLRhi:
    case QSGRendererInterface::GraphicsApi::MetalRhi:
    case QSGRendererInterface::GraphicsApi::VulkanRhi:
    case QSGRendererInterface::GraphicsApi::Direct3D11Rhi: {
        QSGRendererInterface *renderInterface = window->rendererInterface();
        QRhi *rhi = static_cast<QRhi *>(renderInterface->getResource(window, QSGRendererInterface::RhiResource));
        if (rhi->isFeatureSupported(QRhi::MultisampleTexture) && rhi->isFeatureSupported(QRhi::MultisampleRenderBuffer)) {

            auto node = new RiveQSGRHIRenderNode(window, artboardInstance, geometry);
            node->setFillMode(m_renderSettings.fillMode);
            node->setPostprocessingMode(m_renderSettings.postprocessingMode);
            return node;
        } else {
            qCritical(rqqpFactory)
                << "RiveQtQuickPlign requires multisample support. The hardware does not support multisampling. Switch to "
                   "SoftwareRenderer.";
            return nullptr;
        }
    }
#endif
    case QSGRendererInterface::GraphicsApi::Software:
    default:
        return new RiveQSGSoftwareRenderNode(window, artboardInstance, geometry);
    }
}

rive::rcp<rive::RenderBuffer> RiveQtFactory::makeBufferU16(rive::Span<const uint16_t> data)
{
    return nullptr;
}

rive::rcp<rive::RenderBuffer> RiveQtFactory::makeBufferU32(rive::Span<const uint32_t> data)
{
    return nullptr;
}

rive::rcp<rive::RenderBuffer> RiveQtFactory::makeBufferF32(rive::Span<const float> data)
{
    return nullptr;
}

rive::rcp<rive::RenderBuffer> RiveQtFactory::makeRenderBuffer(rive::RenderBufferType renderBufferType, rive::RenderBufferFlags renderBufferFlags, size_t size)
{
    return rive::make_rcp<rive::DataRenderBuffer>(renderBufferType, renderBufferFlags, size);;
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

std::unique_ptr<rive::RenderPath> RiveQtFactory::makeRenderPath(rive::RawPath &rawPath, rive::FillRule fillRule)
{
    switch (renderType()) {
    case RiveQtRenderType::QPainterRenderer:
        return std::make_unique<RiveQtPainterPath>(rawPath, fillRule);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    case RiveQtRenderType::RHIRenderer: {
        std::unique_ptr<RiveQtPath> riveQtPath = std::make_unique<RiveQtPath>(rawPath, fillRule);
        riveQtPath->setLevelOfDetail(levelOfDetail());
        return riveQtPath;
    }
#endif

    case RiveQtRenderType::None:
    default:
        return std::make_unique<RiveQtPainterPath>(rawPath, fillRule); // TODO Add Empty Path
    }
}

std::unique_ptr<rive::RenderPath> RiveQtFactory::makeEmptyRenderPath()
{
    switch (renderType()) {
    case RiveQtRenderType::QPainterRenderer:
        return std::make_unique<RiveQtPainterPath>();
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    case RiveQtRenderType::RHIRenderer: {
        std::unique_ptr<RiveQtPath> riveQtPath = std::make_unique<RiveQtPath>();
        riveQtPath->setLevelOfDetail(levelOfDetail());
        return riveQtPath;
    }
#endif
    case RiveQtRenderType::None:
    default:
        return std::make_unique<RiveQtPainterPath>(); // TODO Add Empty Path
    }
}

std::unique_ptr<rive::RenderPaint> RiveQtFactory::makeRenderPaint()
{
    return std::make_unique<RiveQtPaint>();
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

rive::rcp<rive::Font> RiveQtFactory::decodeFont(rive::Span<const uint8_t> span)
{
#ifdef WITH_RIVE_TEXT
    return HBFont::Decode(span);
#else
    return nullptr;
#endif
    // Todo: would be nice to use qt build in support for fonts
    // however qt is missing an api to access AXIS data from a font; lets for now use the HBFont maintained by rivecpp and consider
    // switching later using qt directy would maybe allow us to drop the direct dependency on harfbuzz ...

    /*QByteArray fontData(reinterpret_cast<const char *>(span.data()), static_cast<int>(span.size()));
    int fontId = QFontDatabase::addApplicationFontFromData(fontData);

    if (fontId == -1) {
        return nullptr;
    }

    const QStringList fontFamilies = QFontDatabase::applicationFontFamilies(fontId);
    if (fontFamilies.isEmpty()) {
        return nullptr;
    }

    QFont font(fontFamilies.first());
    return rive::rcp<RiveQtFont>(new RiveQtFont(font));*/

}

unsigned int RiveQtFactory::levelOfDetail()
{
    switch (m_renderSettings.renderQuality) {
    case RiveRenderSettings::Low:
        return 1;
    default:
    case RiveRenderSettings::Medium:
        return 5;
    case RiveRenderSettings::High:
        return 10;
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
