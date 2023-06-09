
// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once
#include <vector>

#include <QFont>
#include <QFontDatabase>

#include <rive/file.hpp>
#include <rive/renderer.hpp>

#include "rive/renderer.hpp"
#include "rive/span.hpp"

#include "../qtquick/datatypes.h"

class RiveQtQuickItem;

class RiveQtFactory : public rive::Factory
{
public:
    enum class RiveQtRenderType : quint8
    {
        None,
        QPainterRenderer,
        QOpenGLRenderer,
        RHIRenderer
    };

    explicit RiveQtFactory(RiveQtRenderType renderType, const RenderSettings renderSettings = RenderSettings())
        : rive::Factory()
        , m_renderType(renderType)
        , m_renderSettings(renderSettings)
    {
    }

    void setRenderType(RiveQtRenderType renderType)
    {
        if (m_renderType != RiveQtRenderType::None) {
            return;
        }
        m_renderType = renderType;
    }

    void setRenderSettings(const RenderSettings renderSettings) { m_renderSettings = renderSettings; }

    rive::rcp<rive::RenderBuffer> makeBufferU16(rive::Span<const uint16_t> data) override;
    rive::rcp<rive::RenderBuffer> makeBufferU32(rive::Span<const uint32_t> data) override;
    rive::rcp<rive::RenderBuffer> makeBufferF32(rive::Span<const float> data) override;
    rive::rcp<rive::RenderShader> makeLinearGradient(float, float, float, float, const rive::ColorInt[], const float[], size_t) override;
    rive::rcp<rive::RenderShader> makeRadialGradient(float, float, float, const rive::ColorInt[], const float[], size_t) override;
    std::unique_ptr<rive::RenderPath> makeRenderPath(rive::RawPath &rawPath, rive::FillRule fillRule) override;
    std::unique_ptr<rive::RenderPath> makeEmptyRenderPath() override;
    std::unique_ptr<rive::RenderPaint> makeRenderPaint() override;
    std::unique_ptr<rive::RenderImage> decodeImage(rive::Span<const uint8_t> span) override;
    rive::rcp<rive::Font> decodeFont(rive::Span<const uint8_t> span) override;

private:
    unsigned segmentCount();

    RiveQtRenderType m_renderType;
    RenderSettings m_renderSettings;
};
