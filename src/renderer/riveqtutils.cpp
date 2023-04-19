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

#include "riveqtutils.h"
#include <QVector4D>
#include <QMatrix4x4>

#include <rive/renderer.hpp>
#include <rive/command_path.hpp>

#include <QDebug>

QColor RiveQtUtils::riveColorToQt(rive::ColorInt value)
{
  return QColor::fromRgb(rive::colorRed(value), rive::colorGreen(value), rive::colorBlue(value), rive::colorAlpha(value));
}

Qt::PenJoinStyle RiveQtUtils::riveStrokeJoinToQt(rive::StrokeJoin join)
{
  switch (join) {
  case rive::StrokeJoin::miter:
    return Qt::PenJoinStyle::MiterJoin;
  case rive::StrokeJoin::round:
    return Qt::PenJoinStyle::RoundJoin;
  case rive::StrokeJoin::bevel:
    return Qt::PenJoinStyle::BevelJoin;
  }
  return Qt::PenJoinStyle::MiterJoin;
}

Qt::PenCapStyle RiveQtUtils::riveStrokeCapToQt(rive::StrokeCap cap)
{
  switch (cap) {
  case rive::StrokeCap::butt:
    return Qt::PenCapStyle::FlatCap;
  case rive::StrokeCap::round:
    return Qt::PenCapStyle::RoundCap;
  case rive::StrokeCap::square:
    return Qt::PenCapStyle::SquareCap;
  }
  return Qt::PenCapStyle::FlatCap;
}

QMatrix RiveQtUtils::riveMat2DToQt(const rive::Mat2D &riveMatrix)
{
  return QMatrix(riveMatrix[0], riveMatrix[1], riveMatrix[2], riveMatrix[3], riveMatrix[4], riveMatrix[5]);
}

Qt::FillRule RiveQtUtils::riveFillRuleToQt(rive::FillRule fillRule)
{
  switch (fillRule) {
  case rive::FillRule::evenOdd:
    return Qt::FillRule::OddEvenFill;
  case rive::FillRule::nonZero:
  default:
    return Qt::FillRule::WindingFill;
  }
}

QPainterPath RiveQtUtils::transformPathWithMatrix4x4(const QPainterPath &path, const QMatrix4x4 &matrix)
{
  QPainterPath transformedPath;

  int count = path.elementCount();
  for (int i = 0; i < count; ++i) {
    QPainterPath::Element element = path.elementAt(i);
    QVector4D point(element.x, element.y, 0, 1);
    QVector4D transformedPoint = matrix * point;

    switch (element.type) {
    case QPainterPath::MoveToElement:
      transformedPath.moveTo(transformedPoint.x(), transformedPoint.y());
      break;
    case QPainterPath::LineToElement:
      transformedPath.lineTo(transformedPoint.x(), transformedPoint.y());
      break;
    case QPainterPath::CurveToElement: {
      QPainterPath::Element controlPoint1 = element;
      QPainterPath::Element controlPoint2 = path.elementAt(++i);
      QPainterPath::Element endPoint = path.elementAt(++i);

      QVector4D ctrlPt1(controlPoint1.x, controlPoint1.y, 0, 1);
      QVector4D ctrlPt2(controlPoint2.x, controlPoint2.y, 0, 1);
      QVector4D endPt(endPoint.x, endPoint.y, 0, 1);

      QVector4D transformedCtrlPt1 = matrix * ctrlPt1;
      QVector4D transformedCtrlPt2 = matrix * ctrlPt2;
      QVector4D transformedEndPt = matrix * endPt;

      transformedPath.cubicTo(transformedCtrlPt1.x(), transformedCtrlPt1.y(), transformedCtrlPt2.x(), transformedCtrlPt2.y(),
                              transformedEndPt.x(), transformedEndPt.y());
    } break;
    default:
      break;
    }
  }

  return transformedPath;
}

RiveQtPaint::RiveQtPaint() { }

void RiveQtPaint::color(rive::ColorInt value)
{
  m_color = RiveQtUtils::riveColorToQt(value);

  if (!m_color.isValid()) {

    qDebug() << "INVALID COLOR";
  }

  m_Brush.setColor(m_color);
  m_Pen.setColor(m_color);
}

void RiveQtPaint::thickness(float value)
{
  m_Pen.setWidthF(value);
}

void RiveQtPaint::join(rive::StrokeJoin value)
{
  m_Pen.setJoinStyle(RiveQtUtils::riveStrokeJoinToQt(value));
}

void RiveQtPaint::cap(rive::StrokeCap value)
{
  m_Pen.setCapStyle(RiveQtUtils::riveStrokeCapToQt(value));
}

void RiveQtPaint::blendMode(rive::BlendMode value)
{
  m_BlendMode = value;
}

void RiveQtPaint::style(rive::RenderPaintStyle value)
{
  m_paintStyle = value;

  switch (value) {
  case rive::RenderPaintStyle::fill:
    m_Pen.setStyle(Qt::NoPen);
    m_Brush.setStyle(Qt::SolidPattern);
    break;
  case rive::RenderPaintStyle::stroke:
    m_Pen.setStyle(Qt::SolidLine);
    m_Brush.setStyle(Qt::NoBrush);
    break;
  default:
    qDebug() << "DEFAULT STYLE!";
    m_Pen.setStyle(Qt::NoPen);
    m_Brush.setStyle(Qt::NoBrush);
    break;
  }
}

void RiveQtPaint::shader(rive::rcp<rive::RenderShader> shader)
{
  m_shader = shader;
  if (shader) {
    RiveQtShader *qtShader = static_cast<RiveQtShader *>(shader.get());
    m_qtGradient = qtShader->gradient();

    if (!m_qtGradient.isNull()) {
      m_Brush = QBrush(*m_qtGradient);
    } else {
      m_Brush = QBrush(m_color);
    }
  } else {
    m_qtGradient.reset();
    m_Brush = QBrush(m_color);
  }
}

RiveQtLinearGradient::RiveQtLinearGradient(float x1, float y1, float x2, float y2, const rive::ColorInt *colors, const float *stops,
                                           size_t count)
  : m_gradient(x1, y1, x2, y2)
{
  for (size_t i = 0; i < count; ++i) {
    QColor color = RiveQtUtils::riveColorToQt(colors[i]);
    qreal stop = stops[i];
    m_gradient.setColorAt(stop, color);
  }
}
