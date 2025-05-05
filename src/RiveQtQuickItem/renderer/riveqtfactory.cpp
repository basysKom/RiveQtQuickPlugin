// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "renderer/riveqtfactory.h"
#include "riveqtpath.h"
#include "riveqtutils.h"

#include <utils/factory_utils.hpp>

RiveQtFactory::RiveQtFactory(RiveRenderSettings &renderSettings)
    : rive::Factory()
    , m_renderSettings(renderSettings)
{
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
    return rive::make_rcp<RiveQtPath>(rawPath, fillRule, m_renderSettings.renderQuality);
}

rive::rcp<rive::RenderPath> RiveQtFactory::makeEmptyRenderPath()
{
    return rive::make_rcp<RiveQtPath>();
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
