// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "renderer/riveqtpainterrenderer.h"
#include "riveqtpath.h"
#include "rqqplogging.h"
#include "riveqtutils.h"

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

    RiveQtPath *qtPath = static_cast<RiveQtPath *>(path);
    RiveQtPaint *qtPaint = static_cast<RiveQtPaint *>(paint);

    QPainter::CompositionMode compositionMode = RiveQtUtils::convert(qtPaint->blendMode());

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

    RiveQtPath *qtPath = static_cast<RiveQtPath *>(path);
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

    QPainter::CompositionMode compositionMode = RiveQtUtils::convert(blendMode);
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
