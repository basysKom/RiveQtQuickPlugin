
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

RiveQtRhiRenderer::RiveQtRhiRenderer(QQuickWindow *window, RiveQSGRHIRenderNode *node)
    : rive::Renderer()
    , m_window(window)
    , m_node(node)
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

    RiveQtPath clipResult;
    if (!m_rhiRenderStack.back().m_allClipPainterPathesApplied.empty()) {
        QPair<QPainterPath, QMatrix4x4> firstLevel = m_rhiRenderStack.back().m_allClipPainterPathesApplied.first();

        clipResult.setQPainterPath(firstLevel.first);
        clipResult.applyMatrix(firstLevel.second);

        for (int i = 1; i < m_rhiRenderStack.back().m_allClipPainterPathesApplied.count(); ++i) {
            RiveQtPath a;
            const auto &entry = m_rhiRenderStack.back().m_allClipPainterPathesApplied[i];
            a.setQPainterPath(entry.first);
            a.applyMatrix(entry.second);
            clipResult.intersectWith(a.toQPainterPath());
        }

        // #if 0 // this allows to draw the clipping area which it useful for debugging :)
        //    TextureTargetNode *drawClipping = getRiveDrawTargetNode();
        //    drawClipping->setOpacity(currentOpacity()); // inherit the opacity from the parent
        //    drawClipping->setColor(QColor(255, 0, 0, 29));
        //    drawClipping->updateGeometry(clipResult.toVertices(), QMatrix4x4());
        //  #endif

        node->updateClippingGeometry(clipResult.toVertices());
    }

    node->updateGeometry(pathData, transformMatrix());
}

void RiveQtRhiRenderer::clipPath(rive::RenderPath *path)
{
    // alternativly we could save the paths + transform to the node and
    // draw each one by one to the stencil buffer
    // -> I would guess that would be faster
    RiveQtPath *qtPath = static_cast<RiveQtPath *>(path);
    assert(qtPath != nullptr);

    m_rhiRenderStack.back().m_allClipPainterPathesApplied.push_back(
        QPair<QPainterPath, QMatrix4x4>(qtPath->toQPainterPath(), transformMatrix()));
}

void RiveQtRhiRenderer::drawImage(const rive::RenderImage *image, rive::BlendMode blendMode, float opacity)
{
    TextureTargetNode *node = getRiveDrawTargetNode();

    m_rhiRenderStack.back().opacity = opacity;
    node->setOpacity(currentOpacity()); // inherit the opacity from the parent
    node->setBlendMode(blendMode);

    node->setTexture(static_cast<const RiveQtImage *>(image)->image(), //
                     nullptr, nullptr, nullptr, 0, 0,
                     /* recreate= */ true,
                     transformMatrix()); //

    RiveQtPath clipResult;
    QPair<QPainterPath, QMatrix4x4> firstLevel = m_rhiRenderStack.back().m_allClipPainterPathesApplied.first();

    clipResult.setQPainterPath(firstLevel.first);
    clipResult.applyMatrix(firstLevel.second);

    for (int i = 1; i < m_rhiRenderStack.back().m_allClipPainterPathesApplied.count(); ++i) {
        RiveQtPath a;
        const auto &entry = m_rhiRenderStack.back().m_allClipPainterPathesApplied[i];
        a.setQPainterPath(entry.first);
        a.applyMatrix(entry.second);
        clipResult.intersectWith(a.toQPainterPath());
    }

    // #if 0 // this allows to draw the clipping area which it useful for debugging :)
    //    TextureTargetNode *drawClipping = getRiveDrawTargetNode();
    //    drawClipping->setOpacity(currentOpacity()); // inherit the opacity from the parent
    //    drawClipping->setColor(QColor(255, 0, 0, 29));
    //    drawClipping->updateGeometry(clipResult.toVertices(), QMatrix4x4());
    //  #endif

    node->updateClippingGeometry(clipResult.toVertices());
}

void RiveQtRhiRenderer::drawImageMesh(const rive::RenderImage *image, rive::rcp<rive::RenderBuffer> vertices_f32,
                                      rive::rcp<rive::RenderBuffer> uvCoords_f32, rive::rcp<rive::RenderBuffer> indices_u16,
                                      uint32_t vertexCount, uint32_t indexCount, rive::BlendMode blendMode, float opacity)
{

    TextureTargetNode *node = getRiveDrawTargetNode();

    m_rhiRenderStack.back().opacity = opacity;

    node->setOpacity(currentOpacity()); // inherit the opacity from the parent
    node->setBlendMode(blendMode);

    node->setTexture(static_cast<const RiveQtImage *>(image)->image(), //
                     vertices_f32, uvCoords_f32, indices_u16, vertexCount, indexCount,
                     /* recreate= */ false,
                     transformMatrix()); //

    RiveQtPath clipResult;
    QPair<QPainterPath, QMatrix4x4> firstLevel = m_rhiRenderStack.back().m_allClipPainterPathesApplied.first();

    clipResult.setQPainterPath(firstLevel.first);
    clipResult.applyMatrix(firstLevel.second);

    for (int i = 1; i < m_rhiRenderStack.back().m_allClipPainterPathesApplied.count(); ++i) {
        RiveQtPath a;
        const auto &entry = m_rhiRenderStack.back().m_allClipPainterPathesApplied[i];
        a.setQPainterPath(entry.first);
        a.applyMatrix(entry.second);
        clipResult.intersectWith(a.toQPainterPath());
    }

    // #if 0 // this allows to draw the clipping area which it useful for debugging :)
    //    TextureTargetNode *drawClipping = getRiveDrawTargetNode();
    //    drawClipping->setOpacity(currentOpacity()); // inherit the opacity from the parent
    //    drawClipping->setColor(QColor(255, 0, 0, 29));
    //    drawClipping->updateGeometry(clipResult.toVertices(), QMatrix4x4());
    //  #endif

    node->updateClippingGeometry(clipResult.toVertices());
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
        pathNode = new TextureTargetNode(m_window, m_node, m_viewportRect, &m_combinedMatrix, &m_projectionMatrix);
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

void RiveQtRhiRenderer::updateViewPort(const QRectF &viewportRect)
{
    while (!m_renderNodes.empty()) {
        auto *textureTargetNode = m_renderNodes.last();
        m_renderNodes.removeAll(textureTargetNode);
        delete textureTargetNode;
    }

    m_viewportRect = viewportRect;
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
