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

#include "riveqtpainterrenderer.h"
#include "qdebug.h"

#define _USE_MATH_DEFINES
#include <math.h>

RiveQtPainterRenderer::RiveQtPainterRenderer()
  : rive::Renderer()
{
}

void RiveQtPainterRenderer::setPainter(QPainter *painter)
{
  m_painter = painter;
}

void RiveQtPainterRenderer::save()
{
  m_painter->save();
}

void RiveQtPainterRenderer::restore()
{
  m_painter->restore();
}

void RiveQtPainterRenderer::transform(const rive::Mat2D &m)
{
  QTransform qTransform(m[0], m[1], m[2], m[3], m[4], m[5]);

  m_painter->setTransform(qTransform, true);
}

void RiveQtPainterRenderer::drawPath(rive::RenderPath *path, rive::RenderPaint *paint)
{
  if (!path || !paint) {
    return;
  }

  RiveQtPainterPath *qtPath = static_cast<RiveQtPainterPath *>(path);
  RiveQtPaint *qtPaint = static_cast<RiveQtPaint *>(paint);

  QPainter::RenderHints oldRenderHint = m_painter->renderHints();
  m_painter->setRenderHint(QPainter::Antialiasing, true);
  m_painter->setBrush(qtPaint->brush());
  m_painter->setPen(qtPaint->pen());

  QPainterPath painterPath = qtPath->toQPainterPath();
  if (painterPath.isEmpty()) {
    return;
  }

  switch (qtPaint->paintStyle()) {
  case rive::RenderPaintStyle::fill:
    m_painter->fillPath(painterPath, qtPaint->brush());
    break;
  case rive::RenderPaintStyle::stroke:
    m_painter->strokePath(painterPath, qtPaint->pen());
    break;
  default:
    break;
  }

  m_painter->setRenderHints(oldRenderHint);
}

void RiveQtPainterRenderer::clipPath(rive::RenderPath *path)
{
  if (!path) {
    return;
  }

  RiveQtPainterPath *qtPath = static_cast<RiveQtPainterPath *>(path);
  QPainterPath painterPath = qtPath->toQPainterPath();

  if (!painterPath.isEmpty()) {
    m_painter->setClipPath(painterPath, Qt::ClipOperation::ReplaceClip);
  }
}

void RiveQtPainterRenderer::drawImage(const rive::RenderImage *image, rive::BlendMode blendMode, float opacity)
{
  if (!image) {
    return;
  }

  // Assuming you have a method to convert rive::RenderImage* to QImage
  const QImage qtImage = convertRiveImageToQImage(image);
  if (qtImage.isNull()) {
    return;
  }

  // Set the blend mode
  QPainter::CompositionMode compositionMode = convertRiveBlendModeToQCompositionMode(blendMode);
  m_painter->setCompositionMode(compositionMode);

  // Set the opacity
  m_painter->setOpacity(opacity);

  // Draw the image
  m_painter->drawImage(0, 0, qtImage);

  // Reset the composition mode and opacity to their default values
  m_painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
  m_painter->setOpacity(1.0);
}

void RiveQtPainterRenderer::drawImageMesh(const rive::RenderImage *image, rive::rcp<rive::RenderBuffer> vertices_f32,
                                          rive::rcp<rive::RenderBuffer> uvCoords_f32, rive::rcp<rive::RenderBuffer> indices_u16,
                                          rive::BlendMode blendMode, float opacity)
{
  qWarning() << "Draw image mesh is not implemented for qpainter approach";
}

RiveQtPainterPath::RiveQtPainterPath(rive::RawPath &rawPath, rive::FillRule fillRule)
{
  m_path.clear();
  m_path.setFillRule(RiveQtUtils::riveFillRuleToQt(fillRule));

  for (const auto &[verb, pts] : rawPath) {
    switch (verb) {
    case rive::PathVerb::move:
      m_path.moveTo(pts->x, pts->y);
      break;
    case rive::PathVerb::line:
      m_path.lineTo(pts->x, pts->y);
      break;
    case rive::PathVerb::cubic:
      m_path.cubicTo(pts[0].x, pts[0].y, pts[1].x, pts[1].y, pts[2].x, pts[2].y);
      break;
    case rive::PathVerb::close:
      m_path.lineTo(pts->x, pts->y);
      m_path.closeSubpath();
      break;
    default:
      break;
    }
  }
}

void RiveQtPainterPath::addRenderPath(RenderPath *path, const rive::Mat2D &transform)
{
  if (!path) {
    return;
  }

  RiveQtPainterPath *qtPath = static_cast<RiveQtPainterPath *>(path);
  QTransform qTransform(transform[0], transform[1], transform[2], transform[3], transform[4], transform[5]);
  QPainterPath transformedPath = qtPath->m_path * qTransform;
  m_path.addPath(transformedPath);
}
