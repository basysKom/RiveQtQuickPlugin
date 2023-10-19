// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <rive/artboard.hpp>
#include <rive/renderer.hpp>
#include <rive/math/raw_path.hpp>

#include <QPainterPath>
#include <QSGRenderNode>

#include "riveqtutils.h"

class QRhiCommandBuffer;
class QRhiResourceUpdateBatch;
#define INITIAL_VERTICES 1000

class QRhiRenderBuffer;
class QRhiSampler;
class QRhi;
class QRhiBuffer;
class QRhiTexture;
class QRhiShaderResourceBindings;
class QRhiGraphicsPipeline;
class QRhiTextureRenderTarget;
class QRhiRenderPassDescriptor;
class QRhiResource;
class QRhiShaderStage;
class QQuickWindow;
class QQuickItem;

class RiveQSGRHIRenderNode;

class TextureTargetNode
{
public:
    TextureTargetNode(QQuickWindow *window, RiveQSGRHIRenderNode *node, const QRectF &viewPortRect, const QMatrix4x4 *combinedMatrix,
                      const QMatrix4x4 *projectMatrix);
    virtual ~TextureTargetNode();

    // this is true in case the node is currently unused
    bool isRecycled() const { return m_recycled; }
    void recycle();
    void take() { m_recycled = false; }

    void render(QRhiCommandBuffer *cb);
    void renderBlend(QRhiCommandBuffer *cb);
    void releaseResources();
    void updateViewport(const QRectF &rect);

    void setOpacity(const float opacity);
    void setClipping(const bool clip);

    void setColor(const QColor &color);
    void setGradient(const QGradient *gradient);
    void setTexture(const QImage &image,
                    rive::rcp<rive::RenderBuffer> qtVertices,
                    rive::rcp<rive::RenderBuffer> qtUvCoords,
                    rive::rcp<rive::RenderBuffer> indices,
                    uint32_t vertexCount, uint32_t indexCount,
                    bool recreate,
                    const QMatrix4x4 &transform);

    int blendMode() const { return (int)m_blendMode; }
    void setBlendMode(rive::BlendMode blendMode);

    void updateGeometry(const QVector<QVector<QVector2D>> &geometry, const QMatrix4x4 &transform);
    void updateClippingGeometry(const QVector<QVector<QVector2D>> &clippingGeometry);

private:
    void prepareRender();

    bool m_recycled { true };
    bool m_clip { false };

    bool m_blendVerticesDirty = true;
    bool m_shaderBlending = false;

    int m_maximumVerticies { INITIAL_VERTICES };
    int m_maximumClippingVerticies { INITIAL_VERTICES };
    int m_oldBufferSize = 0;

    QSize m_textureSize;

    QVector<QRhiResource *> m_cleanupList;

    QRhiBuffer *m_vertexBuffer { nullptr };
    QRhiBuffer *m_texCoordBuffer { nullptr };
    QRhiBuffer *m_indicesBuffer { nullptr };

    QRhiBuffer *m_blendVertexBuffer { nullptr };
    QRhiBuffer *m_blendTexCoordBuffer { nullptr };
    QRhiBuffer *m_blendUniformBuffer { nullptr };

    QRhiBuffer *m_clippingUniformBuffer { nullptr };
    QRhiBuffer *m_drawUniformBuffer { nullptr };

    QRhiBuffer *m_clippingVertexBuffer { nullptr };

    QRhiShaderResourceBindings *m_blendResourceBindingsA { nullptr };
    QRhiShaderResourceBindings *m_blendResourceBindingsB { nullptr };
    QRhiShaderResourceBindings *m_drawPipelineResourceBindings { nullptr };
    QRhiShaderResourceBindings *m_clippingResourceBindings { nullptr };

    QRhiTextureRenderTarget *m_blendTextureRenderTargetA { nullptr };
    QRhiTextureRenderTarget *m_blendTextureRenderTargetB { nullptr };

    QRhiRenderPassDescriptor *m_blendRenderDescriptorA { nullptr };
    QRhiRenderPassDescriptor *m_blendRenderDescriptorB { nullptr };

    QRhiRenderBuffer *m_stencilClippingBuffer { nullptr };

    QRhiSampler *m_sampler { nullptr };
    QRhiSampler *m_blendSampler { nullptr };

    QList<QRhiShaderStage> m_pathShader;
    QList<QRhiShaderStage> m_textureShader;
    QList<QRhiShaderStage> m_blendShaders;

    QRhiTexture *m_qImageTexture { nullptr };

    RiveQSGRHIRenderNode *m_node { nullptr };

    QRhiResourceUpdateBatch *m_resourceUpdates { nullptr };
    QRhiResourceUpdateBatch *m_blendResourceUpdates { nullptr };

    QQuickWindow *m_window { nullptr };

    // Material Related // Shader Related data
    QColor m_color;
    const QGradient *m_gradient { nullptr };
    QImage m_texture;

    float m_opacity { 1.0 };

    rive::BlendMode m_blendMode = rive::BlendMode::srcOver;

    int useGradient; // 0 -> false , 1 -> true
    int gradientType; // 0 -> Linear, 1 -> Radial
    QPointF gradientFocalPoint;
    QPointF gradientCenter;
    QPointF startPoint;
    QPointF endPoint;
    float gradientRadius;
    QVector<QColor> gradientColors;
    QVector<QVector2D> gradientPositions;
    QList<QVector2D> m_blendVertices;
    QList<QVector2D> m_blendTexCoords;

    QByteArray m_geometryData;
    QByteArray m_clippingData;
    QByteArray m_texCoordData;
    QByteArray m_indicesData;
    QByteArray m_clearData; // this is as large as it must and used in case we reduce the size of a geometry but not reducing the buffer

    struct GradientData
    {
        float gradientRadius; // 68
        float gradientFocalPointX; // 80
        float gradientFocalPointY; // 84
        float gradientCenterX; // 88
        float gradientCenterY; // 92
        float startPointX; // 96
        float startPointY; // 100
        float endPointX; // 104
        float endPointY; // 108
        int numberOfStops; // 112
        int gradientType; // 116

        QVector<QColor> gradientColors; // 144
        QVector<QVector2D> gradientPositions; // 464
    };

    bool m_useTexture { false };
    GradientData m_gradientData;

    // drawing matrix and transformations
    const QMatrix4x4 *m_combinedMatrix;
    const QMatrix4x4 *m_projectionMatrix;
    QMatrix4x4 m_transform;

    QRectF m_rect;
    QRhiGraphicsPipeline *createBlendPipeline(QRhi *rhi, QRhiRenderPassDescriptor *renderPass,
                                              QRhiShaderResourceBindings *blendResourceBindings);

    QRhiGraphicsPipeline *createClipPipeline(QRhi *rhi, QRhiRenderPassDescriptor *renderPassDescriptor);
    void createDrawPipelines(QRhi *rhi, QMap<rive::BlendMode, QRhiGraphicsPipeline *> &map, QRhiRenderPassDescriptor *renderPassDescriptor);
};
