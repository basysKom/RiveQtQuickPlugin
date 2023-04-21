/*
 * MIT License
 *
 * Copyright (C) 2023 by Jeremias Bosch
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <QFont>
#include <QFontDatabase>

#include "riveqtopenglrenderer.h"
#include "riveqtpainterrenderer.h"

#include "rive/renderer.hpp"
#include "rive/span.hpp"
#include <rive/file.hpp>

#include <rive/renderer.hpp>
#include <vector>

class RiveQtFactory : public rive::Factory
{
public:
  enum class RiveQtRenderType : quint8
  {
    None,
    QPainterRenderer,
    QOpenGLRenderer
  };

  explicit RiveQtFactory(RiveQtRenderType renderType)
    : rive::Factory()
    , m_renderType(renderType)
  {
  }

  void setRenderType(RiveQtRenderType renderType)
  {
    if (m_renderType != RiveQtRenderType::None) {
      return;
    }
    m_renderType = renderType;
  }

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
  RiveQtRenderType m_renderType;
};
