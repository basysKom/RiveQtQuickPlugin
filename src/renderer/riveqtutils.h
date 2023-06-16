
// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QColor>
#include <QMatrix4x4>
#include <QPainter>
#include <QPainterPath>

#include <rive/command_path.hpp>
#include <rive/math/mat2d.hpp>
#include <rive/renderer.hpp>
#include <rive/shapes/paint/blend_mode.hpp>
#include <rive/shapes/paint/color.hpp>
#include <rive/shapes/paint/stroke_cap.hpp>
#include <rive/shapes/paint/stroke_join.hpp>

namespace RiveQtUtils {
QColor riveColorToQt(rive::ColorInt value);
Qt::PenJoinStyle riveStrokeJoinToQt(rive::StrokeJoin join);
Qt::PenCapStyle riveStrokeCapToQt(rive::StrokeCap cap);
QMatrix4x4 riveMat2DToQt(const rive::Mat2D &riveMatrix);
Qt::FillRule riveFillRuleToQt(rive::FillRule fillRule);
QPainterPath transformPathWithMatrix4x4(const QPainterPath &path, const QMatrix4x4 &matrix);
} // namespace RiveQtUtils

class RiveQtBufferU16 : public rive::RenderBuffer
{
public:
    RiveQtBufferU16(size_t count)
        : rive::RenderBuffer(count)
        , m_data(count)
    {}
    void setData(const std::vector<uint16_t> &data) { m_data = data; }
    const uint16_t *data() const { return m_data.data(); }
    const size_t size() const { return m_data.size() * sizeof(uint16_t); }
    const uint count() const { return m_data.size(); }

private:
    std::vector<uint16_t> m_data;
};

class RiveQtBufferU32 : public rive::RenderBuffer
{
public:
    RiveQtBufferU32(size_t count)
        : rive::RenderBuffer(count)
        , m_data(count)
    {}
    void setData(const std::vector<uint32_t> &data) { m_data = data; }
    const uint32_t *data() const { return m_data.data(); }
    const size_t size() const { return m_data.size() * sizeof(uint32_t); }
    const uint count() const { return m_data.size(); }

private:
    std::vector<uint32_t> m_data;
};

class RiveQtBufferF32 : public rive::RenderBuffer
{
public:
    RiveQtBufferF32(size_t count)
        : rive::RenderBuffer(count)
        , m_data(count)
    {}
    void setData(const std::vector<float> &data) { m_data = data; }
    const float *data() const { return m_data.data(); }
    const int size() const { return m_data.size() * sizeof(float); }
    const uint count() const { return m_data.size(); }

private:
    std::vector<float> m_data;
};

class RiveQtImage : public rive::RenderImage
{
public:
    RiveQtImage(const QImage &image)
        : m_image(image)
    {
        m_Width = m_image.width();
        m_Height = m_image.height();
    }

    const QImage &image() const { return m_image; }

private:
    QImage m_image;
};

class RiveQtShader : public rive::RenderShader
{
public:
    RiveQtShader() = default;

    virtual QSharedPointer<QGradient> gradient() const = 0;

    float m_opacity{1.0};
};

class RiveQtRadialGradient : public RiveQtShader
{
public:
    RiveQtRadialGradient(float centerX, float centerY, float radius, const rive::ColorInt colors[], const float positions[], size_t count);

    QSharedPointer<QGradient> gradient() const override { return QSharedPointer<QGradient>::create(m_gradient); }

    const QBrush &brush() const { return m_brush; }

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

    rive::RenderPaintStyle paintStyle() const { return m_paintStyle; }
    rive::BlendMode blendMode() const { return m_BlendMode; }

    const QColor &color() const { return m_color; }
    const QBrush &brush() const { return m_Brush; }
    const QPen &pen() const { return m_Pen; }
    const float &opacity() const { return m_opacity; }

    virtual void shader(rive::rcp<rive::RenderShader> shader) override;
    virtual void invalidateStroke() override{}; // maybe we need to reset something here

private:
    rive::RenderPaintStyle m_paintStyle;
    rive::BlendMode m_BlendMode;

    QColor m_color;
    QBrush m_Brush;
    QPen m_Pen;
    float m_opacity{1.0};

    rive::rcp<rive::RenderShader> m_shader;
    QSharedPointer<QGradient> m_qtGradient;
};
