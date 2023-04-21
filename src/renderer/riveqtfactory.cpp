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

#include "riveqtfactory.h"
#include "riveqtfont.h"

rive::rcp<rive::RenderBuffer> RiveQtFactory::makeBufferU16(rive::Span<const uint16_t> data)
{
  auto buffer = new RiveQtBufferU16(data.size());
  std::vector<uint16_t> vecData(data.begin(), data.end());
  buffer->setData(vecData);
  return rive::rcp<rive::RenderBuffer>(buffer);
}

rive::rcp<rive::RenderBuffer> RiveQtFactory::makeBufferU32(rive::Span<const uint32_t> data)
{
  auto buffer = new RiveQtBufferU32(data.size());
  std::vector<uint32_t> vecData(data.begin(), data.end());
  buffer->setData(vecData);
  return rive::rcp<rive::RenderBuffer>(buffer);
}

rive::rcp<rive::RenderBuffer> RiveQtFactory::makeBufferF32(rive::Span<const float> data)
{
  auto buffer = new RiveQtBufferF32(data.size());
  std::vector<float> vecData(data.begin(), data.end());
  buffer->setData(vecData);
  return rive::rcp<rive::RenderBuffer>(buffer);
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
  if (m_renderType == RiveQtRenderType::QOpenGLRenderer) {
    return std::make_unique<RiveQtOpenGLPath>(rawPath, fillRule);
  } else {
    return std::make_unique<RiveQtPainterPath>(rawPath, fillRule);
  }
}

std::unique_ptr<rive::RenderPath> RiveQtFactory::makeEmptyRenderPath()
{
  switch (m_renderType) {
  case RiveQtRenderType::QOpenGLRenderer:
    return std::make_unique<RiveQtOpenGLPath>();
  case RiveQtRenderType::QPainterRenderer:
    return std::make_unique<RiveQtPainterPath>();
  case RiveQtRenderType::None:
    return std::make_unique<RiveQtPainterPath>(); // TODO Add Empty Path
  }
}

std::unique_ptr<rive::RenderPaint> RiveQtFactory::makeRenderPaint()
{
  return std::make_unique<RiveQtPaint>();
}

std::unique_ptr<rive::RenderImage> RiveQtFactory::decodeImage(rive::Span<const uint8_t> span)
{
  QByteArray imageData(reinterpret_cast<const char *>(span.data()), static_cast<int>(span.size()));
  QImage image = QImage::fromData(imageData);

  if (image.isNull()) {
    return nullptr;
  }

  return std::make_unique<RiveQtImage>(image);
}

rive::rcp<rive::Font> RiveQtFactory::decodeFont(rive::Span<const uint8_t> span)
{
  QByteArray fontData(reinterpret_cast<const char *>(span.data()), static_cast<int>(span.size()));
  int fontId = QFontDatabase::addApplicationFontFromData(fontData);

  if (fontId == -1) {
    return nullptr;
  }

  QStringList fontFamilies = QFontDatabase::applicationFontFamilies(fontId);
  if (fontFamilies.isEmpty()) {
    return nullptr;
  }

  QFont font(fontFamilies.first());
  return rive::rcp<RiveQtFont>(new RiveQtFont(font, std::vector<rive::Font::Coord>()));
}
