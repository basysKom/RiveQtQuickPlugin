
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

#include "qopenglframebufferobject.h"

class SubPath;

class RiveQtOpenGLPath : public rive::RenderPath
{
public:
    RiveQtOpenGLPath() = default;
    RiveQtOpenGLPath(const RiveQtOpenGLPath &rqp);
    RiveQtOpenGLPath(rive::RawPath &rawPath, rive::FillRule fillRule);

    void rewind() override;
    void moveTo(float x, float y) override { m_path.moveTo(x, y); }
    void lineTo(float x, float y) override { m_path.lineTo(x, y); }
    void cubicTo(float ox, float oy, float ix, float iy, float x, float y) override { m_path.cubicTo(ox, oy, ix, iy, x, y); }
    void close() override { m_path.closeSubpath(); }
    void fillRule(rive::FillRule value) override;
    void addRenderPath(RenderPath *path, const rive::Mat2D &transform) override;

    QPainterPath toQPainterPath() const { return m_path; }

    QVector<QVector<QVector2D>> toVertices() const;
    QVector<QVector<QVector2D>> toVerticesLine(const QPen &pen) const;

private:
    QVector<QVector2D> qpainterPathToVector2D(const QPainterPath &path);
    QVector<QVector2D> qpainterPathToOutlineVector2D(const QPainterPath &path);
    QPointF cubicBezier(const QPointF &startPoint, const QPointF &controlPoint1, const QPointF &controlPoint2, const QPointF &endPoint,
                        qreal t);
    void generateVerticies();

    QPainterPath m_path;
    std::vector<SubPath> m_subPaths;
    QVector<QVector<QVector2D>> m_pathSegmentsData;
    QVector<QVector<QVector2D>> m_pathSegmentsOutlineData;

    bool m_isClosed { false };
};

class SubPath
{
private:
    RiveQtOpenGLPath *m_Path;
    QMatrix4x4 m_Transform;

public:
    SubPath(RiveQtOpenGLPath *path, const QMatrix4x4 &transform);

    RiveQtOpenGLPath *path() const;
    QMatrix4x4 transform() const;
};

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
    void drawImageMesh(const rive::RenderImage *image, rive::rcp<rive::RenderBuffer> vertices_f32,
                       rive::rcp<rive::RenderBuffer> uvCoords_f32, rive::rcp<rive::RenderBuffer> indices_u16, rive::BlendMode blendMode,
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

    QMatrix4x4 m_projMatrix;
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
