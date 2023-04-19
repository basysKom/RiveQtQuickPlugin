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

#include <QPainter>
#include <QPainterPath>
#include <QBrush>
#include <QPen>
#include <QLinearGradient>

#include <rive/renderer.hpp>
#include <rive/math/raw_path.hpp>

#include "riveqtutils.h"

class QPainterSubPath;

class RiveQtPainterPath : public rive::RenderPath
{
private:
  QPainterPath m_path;

public:
  RiveQtPainterPath() = default;
  RiveQtPainterPath(const RiveQtPainterPath &rqp) { m_path = rqp.m_path; }

  RiveQtPainterPath(rive::RawPath &rawPath, rive::FillRule fillRule);

  void rewind() override { m_path.clear(); }

  void moveTo(float x, float y) override { m_path.moveTo(x, y); }

  void lineTo(float x, float y) override { m_path.lineTo(x, y); }

  void cubicTo(float ox, float oy, float ix, float iy, float x, float y) override { m_path.cubicTo(ox, oy, ix, iy, x, y); }

  void close() override { m_path.closeSubpath(); }

  void fillRule(rive::FillRule value) override
  {
    switch (value) {
    case rive::FillRule::evenOdd:
      m_path.setFillRule(Qt::FillRule::OddEvenFill);
      break;
    case rive::FillRule::nonZero:
      m_path.setFillRule(Qt::FillRule::WindingFill);
      break;
    }
  }
  void addRenderPath(RenderPath *path, const rive::Mat2D &transform) override;

  void setQPainterPath(QPainterPath path) { m_path = path; }

  QPainterPath toQPainterPath() const { return m_path; }
};

class RiveQtPainterRenderer : public rive::Renderer
{
public:
  RiveQtPainterRenderer();

  void setPainter(QPainter *painter);
  void save() override;
  void restore() override;
  void transform(const rive::Mat2D &transform) override;
  void drawPath(rive::RenderPath *path, rive::RenderPaint *paint) override;
  void clipPath(rive::RenderPath *path) override;
  void drawImage(const rive::RenderImage *image, rive::BlendMode blendMode, float opacity) override;
  void drawImageMesh(const rive::RenderImage *image, rive::rcp<rive::RenderBuffer> vertices_f32, rive::rcp<rive::RenderBuffer> uvCoords_f32,
                     rive::rcp<rive::RenderBuffer> indices_u16, rive::BlendMode blendMode, float opacity) override;

private:
  QImage convertRiveImageToQImage(const rive::RenderImage *image)
  {
    if (!image) {
      return QImage();
    }

    // Cast the rive::RenderImage to RiveQtImage
    const RiveQtImage *qtImage = static_cast<const RiveQtImage *>(image);

    // Return the QImage contained in the RiveQtImage
    return qtImage->image();
  }

  QPainter::CompositionMode convertRiveBlendModeToQCompositionMode(rive::BlendMode blendMode)
  {
    switch (blendMode) {
    case rive::BlendMode::srcOver:
      return QPainter::CompositionMode_SourceOver;
    case rive::BlendMode::screen:
      return QPainter::CompositionMode_Screen;
    case rive::BlendMode::overlay:
      return QPainter::CompositionMode_Overlay;
    case rive::BlendMode::darken:
      return QPainter::CompositionMode_Darken;
    case rive::BlendMode::lighten:
      return QPainter::CompositionMode_Lighten;
    case rive::BlendMode::colorDodge:
      return QPainter::CompositionMode_ColorDodge;
    case rive::BlendMode::colorBurn:
      return QPainter::CompositionMode_ColorBurn;
    case rive::BlendMode::hardLight:
      return QPainter::CompositionMode_HardLight;
    case rive::BlendMode::softLight:
      return QPainter::CompositionMode_SoftLight;
    case rive::BlendMode::difference:
      return QPainter::CompositionMode_Difference;
    case rive::BlendMode::exclusion:
      return QPainter::CompositionMode_Exclusion;
    case rive::BlendMode::multiply:
      return QPainter::CompositionMode_Multiply;
    case rive::BlendMode::hue:
    case rive::BlendMode::saturation:
    case rive::BlendMode::color:
    case rive::BlendMode::luminosity:
      // QPainter doesn't have corresponding composition modes for these blend modes
      return QPainter::CompositionMode_SourceOver;
    default:
      return QPainter::CompositionMode_SourceOver;
    }
  }

  QPainter *m_painter;
};
