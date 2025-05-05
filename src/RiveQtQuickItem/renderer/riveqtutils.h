// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <rive/renderer.hpp>
#include <rive/command_path.hpp>
#include <rive/shapes/paint/blend_mode.hpp>
#include <rive/shapes/paint/color.hpp>
#include <rive/shapes/paint/stroke_cap.hpp>
#include <rive/shapes/paint/stroke_join.hpp>
#include <rive/math/mat2d.hpp>

#include <QMatrix4x4>
#include <QColor>
#include <QPainterPath>
#include <QPainter>

namespace RiveQtUtils {
    QColor convert(rive::ColorInt value);
    Qt::PenJoinStyle convert(rive::StrokeJoin join);
    Qt::PenCapStyle convert(rive::StrokeCap cap);
    Qt::FillRule convert(rive::FillRule fillRule);
    QPainter::CompositionMode convert(rive::BlendMode blendMode);
}

class RiveQtImage : public rive::RenderImage
{
public:
    RiveQtImage(const QImage &image)
        : m_image(image)
    {
        m_Width = m_image.width();
        m_Height = m_image.height();
    }

    QImage image() const { return m_image; }

private:
    QImage m_image;
};

class RiveQtShader : public rive::RenderShader
{
public:
    RiveQtShader() = default;

    virtual QSharedPointer<QGradient> gradient() const = 0;

protected:
    float m_opacity { 1.0 };
};

class RiveQtRadialGradient : public RiveQtShader
{
public:
    RiveQtRadialGradient(float centerX, float centerY, float radius, const rive::ColorInt colors[], const float positions[], size_t count);

    QSharedPointer<QGradient> gradient() const override { return QSharedPointer<QGradient>::create(m_gradient); }

    QBrush brush() const { return m_brush; }

private:
    QRadialGradient m_gradient;
    QBrush m_brush;
};

class RiveQtLinearGradient : public RiveQtShader
{
public:
    RiveQtLinearGradient(float x1, float y1, float x2, float y2, const rive::ColorInt *colors, const float *stops, size_t count);

    QSharedPointer<QGradient> gradient() const override { return QSharedPointer<QGradient>::create(m_gradient); }

private:
    QLinearGradient m_gradient;
};

class RiveQtPaint : public rive::RenderPaint
{
public:
    RiveQtPaint();

    void color(rive::ColorInt value) override;
    void thickness(float value) override;
    void join(rive::StrokeJoin value) override;
    void cap(rive::StrokeCap value) override;
    void blendMode(rive::BlendMode value) override;
    void style(rive::RenderPaintStyle value) override;
    void shader(rive::rcp<rive::RenderShader> shader) override;
    void invalidateStroke() override {}; // maybe we need to reset something here
    void feather(float value) override {};

    rive::RenderPaintStyle paintStyle() const { return m_paintStyle; }
    rive::BlendMode blendMode() const { return m_blendMode; }

    QColor color() const { return m_color; }
    QBrush brush() const { return m_brush; }
    QPen pen() const { return m_pen; }
    float opacity() const { return m_opacity; }

private:
    rive::RenderPaintStyle m_paintStyle;
    rive::BlendMode m_blendMode;

    QColor m_color;
    QBrush m_brush { Qt::NoBrush };
    QPen m_pen { Qt::NoPen };
    float m_opacity { 1.0 };

    rive::rcp<rive::RenderShader> m_shader;
    QSharedPointer<QGradient> m_gradient;
};
