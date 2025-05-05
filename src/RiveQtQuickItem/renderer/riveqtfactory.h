// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <rive/factory.hpp>
#include <rive/artboard.hpp>
#include <rive/renderer.hpp>
#include <rive/span.hpp>

#include "datatypes.h"

class RiveQSGRenderNode;

class RiveQtFactory : public rive::Factory
{
public:
    enum class RiveQtRenderType : quint8
    {
        None,
        QPainterRenderer,
        RHIRenderer
    };

    explicit RiveQtFactory(const RiveRenderSettings &renderSettings = RiveRenderSettings());

    void setRenderSettings(const RiveRenderSettings &renderSettings);

    RiveQSGRenderNode *renderNode(QQuickWindow *window, std::weak_ptr<rive::ArtboardInstance> artboardInstance, const QRectF &geometry);
    rive::rcp<rive::RenderBuffer> makeRenderBuffer(rive::RenderBufferType, rive::RenderBufferFlags, size_t) override;

    rive::rcp<rive::RenderShader> makeLinearGradient(float, float, float, float, const rive::ColorInt[], const float[], size_t) override;
    rive::rcp<rive::RenderShader> makeRadialGradient(float, float, float, const rive::ColorInt[], const float[], size_t) override;
    rive::rcp<rive::RenderPath> makeRenderPath(rive::RawPath &rawPath, rive::FillRule fillRule) override;
    rive::rcp<rive::RenderPath> makeEmptyRenderPath() override;
    rive::rcp<rive::RenderPaint> makeRenderPaint() override;
    rive::rcp<rive::RenderImage> decodeImage(rive::Span<const uint8_t> span) override;

private:
    unsigned levelOfDetail();
    RiveQtRenderType renderType();

    RiveRenderSettings m_renderSettings;
};
