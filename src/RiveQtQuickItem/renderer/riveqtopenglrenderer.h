
// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QPainterPath>
#include <QBrush>
#include <QPen>
#include <QLinearGradient>
#include <QOpenGLPaintDevice>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>

#include <rive/renderer.hpp>
#include <rive/math/raw_path.hpp>
#include "riveqtpath.h"

#include "qopenglframebufferobject.h"

struct RenderState
{
    QMatrix4x4 transform;
};

class RiveQtOpenGLRenderer : public rive::Renderer, protected QOpenGLFunctions
{
public:
    RiveQtOpenGLRenderer();

    void initGL();
    void reset();

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

    void updateViewportSize();
    void updateProjectionMatrix(const QMatrix4x4 &projMatrix);
    void updateModelMatrix(const QMatrix4x4 &modelMatrix);

private:
    void setupBlendMode(rive::BlendMode blendMode);
    const QMatrix4x4 &modelMatrix();
    const QMatrix4x4 &transform();

    QOpenGLFramebufferObject *m_displayFramebuffer { nullptr };
    QOpenGLFramebufferObject *m_alphaMaskFramebuffer { nullptr };
    QOpenGLFramebufferObject *m_blendFramebuffer { nullptr };

    QOpenGLShaderProgram *m_imageShaderProgram { nullptr };
    QOpenGLShaderProgram *m_pathShaderProgram { nullptr };
    QOpenGLShaderProgram *m_blendShaderProgram { nullptr };
    QOpenGLShaderProgram *m_imageMeshShaderProgram { nullptr };

    QOpenGLVertexArrayObject m_fullscreenQuadVAO;
    QOpenGLVertexArrayObject m_imageQuadVAO;
    QOpenGLVertexArrayObject m_meshVAO;

    QOpenGLBuffer m_fullscreenQuadVBO;
    QOpenGLBuffer m_imageQuadVBO;
    QOpenGLBuffer m_meshVertexBuffer;
    QOpenGLBuffer m_meshUvBuffer;
    QOpenGLBuffer m_meshIndexBuffer;

    QMatrix4x4 m_projectionMatrix;
    // QMatrix4x4 m_modelMatrix the modelMatrix is stored as base of our renderState!

    qreal m_width { 0 };
    qreal m_height { 0 };
    qreal m_viewportWidth { 0 };
    qreal m_viewportHeight { 0 };

    QVector<GLfloat> m_normalizedQuadVertices;
    QVector<GLfloat> m_viewPortQuadVertices;

    std::list<RenderState> m_Stack;

    bool m_IsClippingDirty = false;
    bool m_useBlendingTexture = false;
    void composeFinalImage(rive::BlendMode blendMode);
    void configureGradientShader(const QGradient *gradient);
};
