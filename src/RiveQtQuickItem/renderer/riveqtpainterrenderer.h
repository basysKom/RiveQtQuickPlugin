// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <rive/renderer.hpp>
#include <rive/math/raw_path.hpp>

#include <QPainter>
#include <QPainterPath>

class RiveQtPainterPath : public rive::RenderPath
{
public:
    RiveQtPainterPath() = default;
    RiveQtPainterPath(const RiveQtPainterPath &rqp);
    RiveQtPainterPath(rive::RawPath &rawPath, rive::FillRule fillRule);

    void rewind() override;
    void moveTo(float x, float y) override;
    void lineTo(float x, float y) override;
    void cubicTo(float ox, float oy, float ix, float iy, float x, float y) override;
    void close() override;
    void fillRule(rive::FillRule value) override;
    void addRenderPath(RenderPath *path, const rive::Mat2D &transform) override;
    void addRawPath(const rive::RawPath &path) override;

    QPainterPath toQPainterPath() const;

private:
    QPainterPath m_path;
};

class RiveQtPainterRenderer : public rive::Renderer
{
public:
    RiveQtPainterRenderer();

    void save() override;
    void restore() override;
    void transform(const rive::Mat2D &transform) override;
    void drawPath(rive::RenderPath *path, rive::RenderPaint *paint) override;
    void clipPath(rive::RenderPath *path) override;
    void drawImage(const rive::RenderImage *image, rive::BlendMode blendMode, float opacity) override;
    void drawImageMesh(const rive::RenderImage *image,
                       rive::rcp<rive::RenderBuffer> vertices_f32,
                       rive::rcp<rive::RenderBuffer> uvCoords_f32,
                       rive::rcp<rive::RenderBuffer> indices_u16,
                       uint32_t vertexCount,
                       uint32_t indexCount,
                       rive::BlendMode blendMode,
                       float opacity) override;

    void setPainter(QPainter *painter);

private:
    QImage convertRiveImageToQImage(const rive::RenderImage *image);
    QPainter::CompositionMode convertRiveBlendModeToQCompositionMode(rive::BlendMode blendMode);

    QTransform m_transform;
    QPainter *m_painter;
};
