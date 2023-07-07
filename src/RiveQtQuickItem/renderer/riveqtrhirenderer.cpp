
// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later
#define _USE_MATH_DEFINES

#include <algorithm>
#include <math.h>

#include <QVector4D>
#include <QSGRenderNode>
#include <QQuickWindow>

#include <private/qtriangulator_p.h>

#include "rqqplogging.h"
#include "renderer/riveqtrhirenderer.h"
#include "rhi/texturetargetnode.h"

RiveQtRhiRenderer::RiveQtRhiRenderer(QQuickWindow *window)
    : rive::Renderer()
    , m_window(window)
{
    m_rhiRenderStack.push_back(RhiRenderState());
}

RiveQtRhiRenderer::~RiveQtRhiRenderer()
{
    for (TextureTargetNode *textureTargetNode : m_renderNodes) {
        delete textureTargetNode;
    }
}

void RiveQtRhiRenderer::save()
{
    m_rhiRenderStack.push_back(m_rhiRenderStack.back());
    m_rhiRenderStack.back().stackNodes.clear();
}

void RiveQtRhiRenderer::restore()
{
    assert(m_rhiRenderStack.size() > 1);
    m_rhiRenderStack.pop_back();
    m_rhiRenderStack.back().opacity = 1.0;
}

void RiveQtRhiRenderer::transform(const rive::Mat2D &transform)
{
    QMatrix4x4 &stackMat = m_rhiRenderStack.back().transform;
    QMatrix4x4 matrix(transform[0], transform[2], 0, transform[4], transform[1], transform[3], 0, transform[5], 0, 0, 1, 0, 0, 0, 0, 1);

    stackMat = stackMat * matrix;
}

void RiveQtRhiRenderer::drawPath(rive::RenderPath *path, rive::RenderPaint *paint)
{
    if (!path) {
        qCDebug(rqqpRendering) << "Cannot draw path that is nullptr.";
        return;
    }
    if (!paint) {
        qCDebug(rqqpRendering) << "Cannot draw path, paint is empty.";
        return;
    }

    RiveQtPath *qtPath = static_cast<RiveQtPath *>(path);
    RiveQtPaint *qtPaint = static_cast<RiveQtPaint *>(paint);

    QVector<QVector<QVector2D>> pathData;

    if (qtPaint->paintStyle() == rive::RenderPaintStyle::fill) {
        pathData = qtPath->toVertices();
    }

    if (qtPaint->paintStyle() == rive::RenderPaintStyle::stroke) {
        pathData = qtPath->toVerticesLine(qtPaint->pen());
    }

    QColor color = qtPaint->color();

    TextureTargetNode *node = getRiveDrawTargetNode();

    m_rhiRenderStack.back().opacity = qtPaint->opacity();

    // opacity is used to apply the strength of the blending effect/layer, though
    // for some reason it looks like it failes in case gradients are used
    // look at that later
    node->setOpacity(currentOpacity()); // inherit the opacity from the parent
    node->setBlendMode(qtPaint->blendMode());

    if (color.isValid()) {
        node->setColor(color);
    } else {
        node->setGradient(qtPaint->brush().gradient());
    }

#if 0 // this allows to draw the clipping area which it useful for debugging :)
    TextureTargetNode *drawClipping = getRiveDrawTargetNode();
    drawClipping->setOpacity(currentOpacity()); // inherit the opacity from the parent
    drawClipping->setColor(QColor(255, 0, 0, 5));
    drawClipping->updateGeometry(m_rhiRenderStack.back().clippingGeometry, QMatrix4x4());
#endif

    node->updateClippingGeometry(m_rhiRenderStack.back().clippingGeometry);

    node->updateGeometry(pathData, transformMatrix());

    m_rhiRenderStack.back().stackNodes.append(node);
}

void RiveQtRhiRenderer::clipPath(rive::RenderPath *path)
{
    // alternativly we could save the paths + transform to the node and
    // draw each one by one to the stencil buffer
    // -> I would guess that would be faster
    RiveQtPath *qtPath = static_cast<RiveQtPath *>(path);
    auto pathVertices = qtPath->toVertices();

    for (auto &path : pathVertices) {
        for (auto &point : path) {
            QVector4D vec4(point, 0.0f, 1.0f);
            vec4 = transformMatrix() * vec4;
            point = vec4.toVector2D();
        }
    }

    m_rhiRenderStack.back().clippingGeometry = pathVertices;

    for (TextureTargetNode *textureTargetNode : m_rhiRenderStack.back().stackNodes) {
        textureTargetNode->updateClippingGeometry(m_rhiRenderStack.back().clippingGeometry);
    }
}

void RiveQtRhiRenderer::drawImage(const rive::RenderImage *image, rive::BlendMode blendMode, float opacity)
{
    TextureTargetNode *node = getRiveDrawTargetNode();

    m_rhiRenderStack.back().opacity = opacity;
    node->setOpacity(currentOpacity()); // inherit the opacity from the parent
    node->setBlendMode(blendMode);

    node->setTexture(static_cast<const RiveQtImage *>(image)->image(), //
                     nullptr, nullptr, nullptr,
                     transformMatrix()); //

#if 0 // this allows to draw the clipping area which it usefull for debugging :)
    TextureTargetNode *drawClipping = getRiveDrawTargetNode();
    drawClipping->setOpacity(currentOpacity()); // inherit the opacity from the parent
    drawClipping->setColor(QColor(255, 0, 0, 50));
    drawClipping->updateGeometry(m_rhiRenderStack.back().clippingGeometry, QMatrix4x4());
#endif

    node->updateClippingGeometry(m_rhiRenderStack.back().clippingGeometry);
    m_rhiRenderStack.back().stackNodes.append(node);
}

void RiveQtRhiRenderer::drawImageMesh(const rive::RenderImage *image, rive::rcp<rive::RenderBuffer> vertices_f32,
                                      rive::rcp<rive::RenderBuffer> uvCoords_f32, rive::rcp<rive::RenderBuffer> indices_u16,
                                      rive::BlendMode blendMode, float opacity)
{
    TextureTargetNode *node = getRiveDrawTargetNode();

    m_rhiRenderStack.back().opacity = opacity;

    node->setOpacity(currentOpacity()); // inherit the opacity from the parent
    node->setBlendMode(blendMode);

    node->setTexture(static_cast<const RiveQtImage *>(image)->image(), //
                     static_cast<RiveQtBufferF32 *>(vertices_f32.get()), //
                     static_cast<RiveQtBufferF32 *>(uvCoords_f32.get()), //
                     static_cast<RiveQtBufferU16 *>(indices_u16.get()),
                     transformMatrix()); //

#if 0 // this allows to draw the clipping area which it usefull for debugging :)
  TextureTargetNode *drawClipping = getRiveDrawTargetNode();
  drawClipping->setOpacity(currentOpacity()); // inherit the opacity from the parent
  drawClipping->setColor(QColor(255, 0, 0, 50));
  drawClipping->updateGeometry(m_rhiRenderStack.back().clippingGeometry, QMatrix4x4());
#endif

    node->updateClippingGeometry(m_rhiRenderStack.back().clippingGeometry);
    m_rhiRenderStack.back().stackNodes.append(node);
}

void RiveQtRhiRenderer::render(QRhiCommandBuffer *cb)
{
    for (TextureTargetNode *textureTargetNode : m_renderNodes) {
        textureTargetNode->render(cb);
    }
}

TextureTargetNode *RiveQtRhiRenderer::getRiveDrawTargetNode()
{
    TextureTargetNode *pathNode = nullptr;

    for (TextureTargetNode *textureTargetNode : m_renderNodes) {
        if (textureTargetNode->isRecycled()) {
            pathNode = textureTargetNode;
            pathNode->take();
            break;
        }
    }

    if (!pathNode) {
        pathNode = new TextureTargetNode(m_window, m_displayBuffer, m_viewportRect, &m_combinedMatrix, &m_projectionMatrix);
        pathNode->take();
        m_renderNodes.append(pathNode);
    }

    return pathNode;
}

void RiveQtRhiRenderer::setProjectionMatrix(const QMatrix4x4 *projectionMatrix, const QMatrix4x4 *combinedMatrix)
{
    m_projectionMatrix = *projectionMatrix;
    m_combinedMatrix = *combinedMatrix;
}

void RiveQtRhiRenderer::updateViewPort(const QRectF &viewportRect, QRhiTexture *displayBuffer)
{
    while (!m_renderNodes.empty()) {
        auto *textureTargetNode = m_renderNodes.last();
        m_renderNodes.removeAll(textureTargetNode);
        delete textureTargetNode;
    }

    m_viewportRect = viewportRect;
    m_displayBuffer = displayBuffer;
}

void RiveQtRhiRenderer::recycleRiveNodes()
{
    for (TextureTargetNode *textureTargetNode : m_renderNodes) {
        textureTargetNode->recycle();
    }
}

const QMatrix4x4 &RiveQtRhiRenderer::transformMatrix() const
{
    return m_rhiRenderStack.back().transform;
}

float RiveQtRhiRenderer::currentOpacity()
{
    float opacity = 1.0;
    for (const auto &renderState : m_rhiRenderStack) {
        opacity *= renderState.opacity;
    }
    return opacity;
}
