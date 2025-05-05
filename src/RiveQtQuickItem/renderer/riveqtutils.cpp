// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "renderer/riveqtutils.h"
#include "rqqplogging.h"

#include <QMatrix4x4>
#include <QVector4D>

#include <rive/shapes/paint/color.hpp>
#include <rive/renderer.hpp>
#include <rive/command_path.hpp>

QColor RiveQtUtils::convert(rive::ColorInt value)
{
    return QColor::fromRgb(rive::colorRed(value), rive::colorGreen(value), rive::colorBlue(value), rive::colorAlpha(value));
}

Qt::PenJoinStyle RiveQtUtils::convert(rive::StrokeJoin join)
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

Qt::PenCapStyle RiveQtUtils::convert(rive::StrokeCap cap)
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

Qt::FillRule RiveQtUtils::convert(rive::FillRule fillRule)
{
    switch (fillRule) {
    case rive::FillRule::evenOdd:
        return Qt::FillRule::OddEvenFill;
    case rive::FillRule::nonZero:
    default:
        return Qt::FillRule::WindingFill;
    }
}

QPainter::CompositionMode RiveQtUtils::convert(rive::BlendMode blendMode)
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

RiveQtPaint::RiveQtPaint()
    : rive::RenderPaint()
{
}

void RiveQtPaint::color(rive::ColorInt value)
{
    m_color = RiveQtUtils::convert(value);
    m_opacity = rive::colorOpacity(value);

    if (!m_color.isValid()) {
        qCDebug(rqqpRendering) << "INVALID COLOR";
    }

    m_brush.setColor(m_color);
    m_pen.setColor(m_color);
}

void RiveQtPaint::thickness(float value)
{
    m_pen.setWidthF(value);
}

void RiveQtPaint::join(rive::StrokeJoin value)
{
    m_pen.setJoinStyle(RiveQtUtils::convert(value));
}

void RiveQtPaint::cap(rive::StrokeCap value)
{
    m_pen.setCapStyle(RiveQtUtils::convert(value));
}

void RiveQtPaint::blendMode(rive::BlendMode value)
{
    m_blendMode = value;
}

void RiveQtPaint::style(rive::RenderPaintStyle value)
{
    m_paintStyle = value;

    switch (value) {
    case rive::RenderPaintStyle::fill:
        m_pen.setStyle(Qt::NoPen);
        m_brush.setStyle(Qt::SolidPattern);
        break;
    case rive::RenderPaintStyle::stroke:
        m_pen.setStyle(Qt::SolidLine);
        m_brush.setStyle(Qt::NoBrush);
        break;
    default:
        qCDebug(rqqpRendering) << "DEFAULT STYLE!";
        m_pen.setStyle(Qt::NoPen);
        m_brush.setStyle(Qt::NoBrush);
        break;
    }
}

void RiveQtPaint::shader(rive::rcp<rive::RenderShader> shader)
{
    m_shader = shader;
    if (shader) {
        RiveQtShader *qtShader = static_cast<RiveQtShader *>(shader.get());
        m_gradient = qtShader->gradient();

        if (!m_gradient.isNull()) {
            m_brush = QBrush(*m_gradient);
            if (m_paintStyle == rive::RenderPaintStyle::stroke) {
                m_pen.setBrush(m_brush);
            }
        } else {
            m_brush = QBrush(m_color);
        }
    } else {
        m_gradient.reset();
        m_brush = QBrush(m_color);
    }
}

RiveQtLinearGradient::RiveQtLinearGradient(float x1, float y1, float x2, float y2, const rive::ColorInt *colors, const float *stops,
                                           size_t count)
    : m_gradient(x1, y1, x2, y2)
{
    m_opacity = 0;

    for (size_t i = 0; i < count; ++i) {
        QColor color = RiveQtUtils::convert(colors[i]);
        m_opacity = qMax(m_opacity, rive::colorOpacity(colors[i]));
        qreal stop = stops[i];
        m_gradient.setColorAt(stop, color);
    }
}

RiveQtRadialGradient::RiveQtRadialGradient(float centerX, float centerY, float radius, const rive::ColorInt colors[],
                                           const float positions[], size_t count)
{
    m_gradient = QRadialGradient(centerX, centerY, radius);

    for (size_t i = 0; i < count; i++) {
        QColor color(RiveQtUtils::convert(colors[i]));
        m_opacity = rive::colorOpacity(colors[i]);
        m_gradient.setColorAt(positions[i], color);
    }

    m_brush = QBrush(m_gradient);
}
