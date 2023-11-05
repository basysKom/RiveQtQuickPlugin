
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
#include <rive/span.hpp>
#include <utils/factory_utils.hpp>

#ifdef WITH_RIVE_TEXT
#    include <rive/text/font_hb.hpp>
#endif

#include "datatypes.h"
#include "riveqsgrendernode.h"

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

    explicit RiveQtFactory(const RiveRenderSettings &renderSettings = RiveRenderSettings())
        : rive::Factory()
        , m_renderSettings(renderSettings)
    {
    }

    void setRenderSettings(const RiveRenderSettings &renderSettings) { m_renderSettings = renderSettings; }

    RiveQSGRenderNode *renderNode(QQuickWindow *window, std::weak_ptr<rive::ArtboardInstance> artboardInstance, const QRectF &geometry);

    rive::rcp<rive::RenderBuffer> makeRenderBuffer(rive::RenderBufferType, rive::RenderBufferFlags, size_t) override;

    rive::rcp<rive::RenderShader> makeLinearGradient(float, float, float, float, const rive::ColorInt[], const float[], size_t) override;
    rive::rcp<rive::RenderShader> makeRadialGradient(float, float, float, const rive::ColorInt[], const float[], size_t) override;
    std::unique_ptr<rive::RenderPath> makeRenderPath(rive::RawPath &rawPath, rive::FillRule fillRule) override;
    std::unique_ptr<rive::RenderPath> makeEmptyRenderPath() override;
    std::unique_ptr<rive::RenderPaint> makeRenderPaint() override;
    std::unique_ptr<rive::RenderImage> decodeImage(rive::Span<const uint8_t> span) override;
    rive::rcp<rive::Font> decodeFont(rive::Span<const uint8_t> span) override;

private:
    unsigned levelOfDetail();

    RiveQtRenderType renderType();

    RiveRenderSettings m_renderSettings;
};
