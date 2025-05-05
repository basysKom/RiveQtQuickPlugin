// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "renderer/riveqtpainterrenderer.h"
#include "rqqplogging.h"
#include "riveqtutils.h"

RiveQtPainterPath::RiveQtPainterPath(const RiveQtPainterPath &rqp)
    : rive::RenderPath()
    , m_path(rqp.m_path)
{
}

RiveQtPainterPath::RiveQtPainterPath(rive::RawPath &rawPath, rive::FillRule fillRule)
    : rive::RenderPath()
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

void RiveQtPainterPath::rewind()
{
    m_path.clear();
}

void RiveQtPainterPath::moveTo(float x, float y)
{
    m_path.moveTo(x, y);
}

void RiveQtPainterPath::lineTo(float x, float y)
{
    m_path.lineTo(x, y);
}

void RiveQtPainterPath::cubicTo(float ox, float oy, float ix, float iy, float x, float y)
{
    m_path.cubicTo(ox, oy, ix, iy, x, y);
}

void RiveQtPainterPath::close()
{
    m_path.closeSubpath();
}

void RiveQtPainterPath::fillRule(rive::FillRule value)
{
    switch (value) {
    case rive::FillRule::evenOdd:
        m_path.setFillRule(Qt::FillRule::OddEvenFill);
        break;
    case rive::FillRule::nonZero:
    default:
        m_path.setFillRule(Qt::FillRule::WindingFill);
        break;
    }
}

void RiveQtPainterPath::addRenderPath(RenderPath *path, const rive::Mat2D &transform)
{
    if (!path) {
        return;
    }

    RiveQtPainterPath *qtPath = static_cast<RiveQtPainterPath *>(path);
    QTransform qTransform(transform[0], transform[1], transform[2], transform[3], transform[4], transform[5]);

    QPainterPath qPath = qtPath->toQPainterPath() * qTransform;
    m_path.addPath(qPath);
}

void RiveQtPainterPath::addRawPath(const rive::RawPath &path)
{

}

QPainterPath RiveQtPainterPath::toQPainterPath() const
{
    return m_path;
}

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
    QTransform transform(m[0], m[1], m[2], m[3], m[4], m[5]);
    m_painter->setTransform(transform, true);
}

void RiveQtPainterRenderer::drawPath(rive::RenderPath *path, rive::RenderPaint *paint)
{
    if (!path || !paint) {
        return;
    }

    RiveQtPainterPath *qtPath = static_cast<RiveQtPainterPath *>(path);
    RiveQtPaint *qtPaint = static_cast<RiveQtPaint *>(paint);

    QPainter::CompositionMode compositionMode = convertRiveBlendModeToQCompositionMode(qtPaint->blendMode());

    QPainter::RenderHints oldRenderHint = m_painter->renderHints();
    m_painter->setCompositionMode(compositionMode);
    m_painter->setRenderHint(QPainter::Antialiasing, true);
    m_painter->setBrush(qtPaint->brush());
    m_painter->setPen(qtPaint->pen());

    const auto qpath = qtPath->toQPainterPath();

    if (!qpath.isEmpty()) {
        switch (qtPaint->paintStyle()) {
        case rive::RenderPaintStyle::fill:
            m_painter->fillPath(qpath, qtPaint->brush());
            break;
        case rive::RenderPaintStyle::stroke:
            m_painter->strokePath(qpath, qtPaint->pen());
            break;
        default:
            break;
        }
    }

    m_painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
    m_painter->setRenderHints(oldRenderHint);
}

void RiveQtPainterRenderer::clipPath(rive::RenderPath *path)
{
    if (!path) {
        return;
    }

    RiveQtPainterPath *qtPath = static_cast<RiveQtPainterPath *>(path);
    m_painter->setClipPath(qtPath->toQPainterPath(), Qt::ClipOperation::IntersectClip);
}

void RiveQtPainterRenderer::drawImage(const rive::RenderImage *image, rive::BlendMode blendMode, float opacity)
{
    if (!image) {
        return;
    }

    // Assuming you have a method to convert rive::RenderImage* to QImage
    const QImage qtImage = convertRiveImageToQImage(image);
    if (qtImage.isNull()) {
        qCDebug(rqqpRendering) << "Converting rive image to QImage failed. Image is null.";
        return;
    }

    QPainter::CompositionMode compositionMode = convertRiveBlendModeToQCompositionMode(blendMode);
    QPainter::RenderHints oldRenderHint = m_painter->renderHints();
    m_painter->setCompositionMode(compositionMode);
    m_painter->setRenderHint(QPainter::Antialiasing, true);

    // Set the blend mode
    m_painter->setCompositionMode(compositionMode);

    // Set the opacity
    m_painter->setOpacity(opacity);

    // Draw the image
    m_painter->drawImage(0, 0, qtImage);

    // Reset the composition mode and opacity to their default values
    m_painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
    m_painter->setOpacity(1.0);
    m_painter->setRenderHints(oldRenderHint);
}

void RiveQtPainterRenderer::drawImageMesh(const rive::RenderImage *image, rive::rcp<rive::RenderBuffer> vertices_f32,
                                          rive::rcp<rive::RenderBuffer> uvCoords_f32, rive::rcp<rive::RenderBuffer> indices_u16,
                                          uint32_t vertexCount, uint32_t indexCount, rive::BlendMode blendMode, float opacity)
{
    qCWarning(rqqpRendering) << "Draw image mesh is not implemented for qpainter approach";
}

QImage RiveQtPainterRenderer::convertRiveImageToQImage(const rive::RenderImage *image)
{
    if (!image) {
        return QImage();
    }

    // Cast the rive::RenderImage to RiveQtImage
    const RiveQtImage *qtImage = static_cast<const RiveQtImage *>(image);

    // Return the QImage contained in the RiveQtImage
    return qtImage->image();
}

QPainter::CompositionMode RiveQtPainterRenderer::convertRiveBlendModeToQCompositionMode(rive::BlendMode blendMode)
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
