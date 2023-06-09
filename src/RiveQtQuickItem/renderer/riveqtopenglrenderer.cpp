
// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#define _USE_MATH_DEFINES
#include <math.h>

#include <QOpenGLFunctions>
#include <QOpenGLTexture>
#include <QOpenGLFramebufferObject>

#include <private/qtriangulator_p.h>

#include "rqqplogging.h"
#include "renderer/riveqtopenglrenderer.h"
#include "renderer/riveqtutils.h"

RiveQtOpenGLRenderer::RiveQtOpenGLRenderer()
    : rive::Renderer()
{
    initializeOpenGLFunctions();
    m_Stack.emplace_back(RenderState());
}

void RiveQtOpenGLRenderer::initGL()
{
    // static fullscreen TRIANGLE_FAN

    m_normalizedQuadVertices = { // Positions  // Texture Coords
                                 -1.0f, -1.0f, 0.0f, 0.0f, //
                                 -1.0f, 1.0f,  0.0f, 1.0f, //
                                 1.0f,  1.0f,  1.0f, 1.0f, //
                                 1.0f,  -1.0f, 1.0f, 0.0f
    };

    // ---- Initialize Path Draw Shader Programm
    //
    m_pathShaderProgram = new QOpenGLShaderProgram();
    m_pathShaderProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/path_vertex_shader.glsl");
    m_pathShaderProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/path_fragement_shader.glsl");
    m_pathShaderProgram->link();

    // ---- All releated to blending shader program
    //
    m_blendShaderProgram = new QOpenGLShaderProgram();
    m_blendShaderProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/blending_vertex_shader.glsl");
    m_blendShaderProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/blending_fragment_shader.glsl");
    m_blendShaderProgram->link();

    m_fullscreenQuadVBO = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);

    m_fullscreenQuadVAO.create();
    m_fullscreenQuadVAO.bind();
    {
        m_fullscreenQuadVBO.create();
        m_fullscreenQuadVBO.bind();
        m_fullscreenQuadVBO.allocate(m_normalizedQuadVertices.constData(), m_normalizedQuadVertices.size() * sizeof(GLfloat));

        int posAttr = m_blendShaderProgram->attributeLocation("a_position");
        m_blendShaderProgram->enableAttributeArray(posAttr);
        m_blendShaderProgram->setAttributeBuffer(posAttr, GL_FLOAT, 0, 2, 4 * sizeof(GLfloat));

        int texCoordAttr = m_blendShaderProgram->attributeLocation("a_texCoord");
        m_blendShaderProgram->enableAttributeArray(texCoordAttr);
        m_blendShaderProgram->setAttributeBuffer(texCoordAttr, GL_FLOAT, 2 * sizeof(GLfloat), 2, 4 * sizeof(GLfloat));
    }
    m_fullscreenQuadVAO.release();

    // ---- All releated to Image shader program
    //
    m_imageShaderProgram = new QOpenGLShaderProgram();
    m_imageShaderProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/image_vertex_shader.glsl");
    m_imageShaderProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/image_fragment_shader.glsl");
    m_imageShaderProgram->link();

    m_imageQuadVAO.create();
    m_imageQuadVAO.bind();
    {
        m_imageQuadVBO.create();
        m_imageQuadVBO.bind();
        m_imageQuadVBO.allocate(m_normalizedQuadVertices.constData(), m_normalizedQuadVertices.size() * sizeof(GLfloat));

        int imagePosAttr = m_imageShaderProgram->attributeLocation("a_position");
        m_imageShaderProgram->enableAttributeArray(imagePosAttr);
        m_imageShaderProgram->setAttributeBuffer(imagePosAttr, GL_FLOAT, 0, 2, 4 * sizeof(GLfloat));

        int imageTexCoordAttr = m_imageShaderProgram->attributeLocation("a_texCoord");
        m_imageShaderProgram->enableAttributeArray(imageTexCoordAttr);
        m_imageShaderProgram->setAttributeBuffer(imageTexCoordAttr, GL_FLOAT, 2 * sizeof(GLfloat), 2, 4 * sizeof(GLfloat));
    }
    m_imageQuadVAO.release();

    // ---- All releated to Image Mesh shader program
    //
    m_imageMeshShaderProgram = new QOpenGLShaderProgram();
    m_imageMeshShaderProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/imageMesh_vertex_shader.glsl");
    m_imageMeshShaderProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/imageMesh_fragment_shader.glsl");
    m_imageMeshShaderProgram->link();

    m_meshVAO.create();
    m_meshVAO.bind();
    {
        // Vertex buffer for positions
        m_meshVertexBuffer = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
        m_meshVertexBuffer.create();
        m_meshVertexBuffer.bind();
        int meshPosAttr = m_imageMeshShaderProgram->attributeLocation("a_position");
        m_imageMeshShaderProgram->enableAttributeArray(meshPosAttr);
        m_imageMeshShaderProgram->setAttributeBuffer(meshPosAttr, GL_FLOAT, 0, 2, 0);

        // Vertex buffer for texture coordinates
        m_meshUvBuffer.create();
        m_meshUvBuffer.bind();
        int meshTexCoordAttr = m_imageMeshShaderProgram->attributeLocation("a_texCoord");
        m_imageMeshShaderProgram->enableAttributeArray(meshTexCoordAttr);
        m_imageMeshShaderProgram->setAttributeBuffer(meshTexCoordAttr, GL_FLOAT, 0, 2, 0);

        // Index buffer
        m_meshIndexBuffer = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
        m_meshIndexBuffer.create();
        m_meshIndexBuffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
        m_meshIndexBuffer.bind();
    }

    m_meshVAO.release();
}

void RiveQtOpenGLRenderer::reset()
{
    if (m_displayFramebuffer) {
        m_displayFramebuffer->bind();
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Clear color with alpha set to zero
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        m_displayFramebuffer->release();
    }
    if (m_alphaMaskFramebuffer) {
        m_alphaMaskFramebuffer->bind();
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Clear color with alpha set to zero
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        m_alphaMaskFramebuffer->release();
    }
    if (m_blendFramebuffer) {
        m_blendFramebuffer->bind();
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Clear color with alpha set to zero
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        m_blendFramebuffer->release();
    }
}

void RiveQtOpenGLRenderer::save()
{
    m_Stack.push_back(m_Stack.back());
}

void RiveQtOpenGLRenderer::restore()
{
    assert(m_Stack.size() > 1);
    m_Stack.pop_back();
}

void RiveQtOpenGLRenderer::transform(const rive::Mat2D &transform)
{
    QMatrix4x4 &stackMat = m_Stack.back().transform;
    QMatrix4x4 matrix(transform[0], transform[2], 0, transform[4], transform[1], transform[3], 0, transform[5], 0, 0, 1, 0, 0, 0, 0, 1);

    stackMat = stackMat * matrix;
}

void RiveQtOpenGLRenderer::setupBlendMode(rive::BlendMode blendMode)
{
    switch (blendMode) {
    case rive::BlendMode::colorDodge:
    case rive::BlendMode::overlay:
    case rive::BlendMode::colorBurn:
    case rive::BlendMode::softLight:
    case rive::BlendMode::hardLight:
    case rive::BlendMode::exclusion:
    case rive::BlendMode::hue:
    case rive::BlendMode::saturation:
    case rive::BlendMode::color:
    case rive::BlendMode::luminosity:
    case rive::BlendMode::screen:
        m_useBlendingTexture = true; // intentional fallthrough
    case rive::BlendMode::srcOver:
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        break;
    case rive::BlendMode::darken:
        glBlendFunc(GL_ONE, GL_ONE);
        glBlendEquation(GL_MIN);
        break;
    case rive::BlendMode::lighten:
        glBlendFunc(GL_ONE, GL_ONE);
        glBlendEquation(GL_MAX);
        break;
    case rive::BlendMode::difference:
        glBlendFunc(GL_ONE, GL_ONE);
        glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
        break;
    case rive::BlendMode::multiply:
        glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
        glBlendFuncSeparate(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        break;
    default:
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        break;
    }
}

const QMatrix4x4 &RiveQtOpenGLRenderer::modelMatrix()
{
    return m_Stack.front().transform;
}

void RiveQtOpenGLRenderer::composeFinalImage(rive::BlendMode blendMode)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (m_useBlendingTexture) {
        m_blendFramebuffer->release();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_blendFramebuffer->texture());

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_displayFramebuffer->texture());

        m_displayFramebuffer->bind();

        // Draw the fullscreen quad
        m_fullscreenQuadVAO.bind();
        m_blendShaderProgram->bind();

        m_fullscreenQuadVBO.bind();
        m_fullscreenQuadVBO.allocate(m_normalizedQuadVertices.constData(), m_normalizedQuadVertices.size() * sizeof(GLfloat));
        m_blendShaderProgram->setUniformValue("u_blendTexture", 0);
        m_blendShaderProgram->setUniformValue("u_displayTexture", 1);
        m_blendShaderProgram->setUniformValue("u_blendMode", static_cast<int>(blendMode));
        m_blendShaderProgram->setUniformValue("u_projection", m_projectionMatrix);
        m_blendShaderProgram->setUniformValue("u_applyTranform", false);

        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        // Clean up

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);
        m_blendShaderProgram->release();
        m_fullscreenQuadVAO.release();

        m_useBlendingTexture = false;
    }

    m_displayFramebuffer->release();

    // Bind the default framebuffer
    QOpenGLFramebufferObject::bindDefault();
    // Set the blend texture

    // Draw the fullscreen quad
    m_fullscreenQuadVAO.bind();
    m_blendShaderProgram->bind();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_displayFramebuffer->texture());

    m_fullscreenQuadVBO.bind();
    m_fullscreenQuadVBO.allocate(m_viewPortQuadVertices.constData(), sizeof(GLfloat) * m_viewPortQuadVertices.size());

    m_blendShaderProgram->setUniformValue("u_displayTexture", 0);
    m_blendShaderProgram->setUniformValue("u_blendMode", 0);
    m_blendShaderProgram->setUniformValue("u_projection", m_projectionMatrix);
    m_blendShaderProgram->setUniformValue("u_applyTranform", true);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    // Clean up
    m_blendShaderProgram->release();
    m_fullscreenQuadVAO.release();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void RiveQtOpenGLRenderer::configureGradientShader(const QGradient *gradient)
{
    QGradientStops gradientStops = gradient->stops();

    QVector<QVector4D> gradientColors;
    QVector<QVector2D> gradientPositions;

    for (const auto &stop : gradientStops) {
        QColor color = stop.second;
        gradientColors.append(QVector4D(color.redF(), color.greenF(), color.blueF(), color.alphaF()));
        gradientPositions.append(QVector2D(stop.first, 0.0f));
    }

    m_pathShaderProgram->setUniformValueArray("u_stopColors", gradientColors.constData(), gradientColors.size());
    m_pathShaderProgram->setUniformValueArray("u_stopPositions", gradientPositions.constData(), gradientPositions.size());
    m_pathShaderProgram->setUniformValue("u_numStops", static_cast<GLint>(gradientStops.size()));

    // Check the gradient type and set the appropriate shader uniforms
    if (gradient->type() == QGradient::LinearGradient) {
        const QLinearGradient *linearGradient = static_cast<const QLinearGradient *>(gradient);
        QPointF startPoint = linearGradient->start();
        QPointF endPoint = linearGradient->finalStop();

        m_pathShaderProgram->setUniformValue("u_gradientStart", startPoint);
        m_pathShaderProgram->setUniformValue("u_gradientEnd", endPoint);

        m_pathShaderProgram->setUniformValue("u_useGradient", true);
        m_pathShaderProgram->setUniformValue("u_gradientType", 0); // 0 for linear gradient

    } else if (gradient->type() == QGradient::RadialGradient) {
        const QRadialGradient *radialGradient = static_cast<const QRadialGradient *>(gradient);
        QPointF center = radialGradient->center();
        QPointF focalPoint = radialGradient->focalPoint();
        qreal radius = radialGradient->radius();

        m_pathShaderProgram->setUniformValue("u_gradientCenter", QVector2D(center));
        m_pathShaderProgram->setUniformValue("u_gradientFocalPoint", QVector2D(focalPoint));
        m_pathShaderProgram->setUniformValue("u_gradientRadius", static_cast<float>(radius));

        m_pathShaderProgram->setUniformValue("u_useGradient", true);
        m_pathShaderProgram->setUniformValue("u_gradientType", 1); // 1 for radial gradient
    }
}

void RiveQtOpenGLRenderer::drawPath(rive::RenderPath *path, rive::RenderPaint *paint)
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

    const auto &blendMode = qtPaint->blendMode();
    // Set the appropriate OpenGL blend functions
    setupBlendMode(qtPaint->blendMode());

    glEnable(GL_BLEND);
    {
        QVector<QVector<QVector2D>> pathData;

        if (qtPaint->paintStyle() == rive::RenderPaintStyle::fill) {
            pathData = qtPath->toVertices();
        }

        if (qtPaint->paintStyle() == rive::RenderPaintStyle::stroke) {
            pathData = qtPath->toVerticesLine(qtPaint->pen());
        }

        // Check if the alphaMaskFramebuffer is valid
        if (m_alphaMaskFramebuffer && m_alphaMaskFramebuffer->isValid()) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_alphaMaskFramebuffer->texture());
        } else {
            qWarning() << "AlphaMaskFramebuffer is not valid.";
        }

        if (m_useBlendingTexture) {
            if (m_blendFramebuffer && m_blendFramebuffer->isValid()) {
                m_blendFramebuffer->bind();

                glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Clear color with alpha set to zero
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

            } else {
                qWarning() << "BlendFramebuffer is not valid. disable blending texture use.";
            }

        } else {
            if (m_displayFramebuffer && m_displayFramebuffer->isValid()) {
                m_displayFramebuffer->bind();

            } else {
                qWarning() << "m_displayFramebuffer is not valid. disable its use.";
            }
        }

        QOpenGLVertexArrayObject vao;
        vao.create();
        vao.bind();

        QOpenGLBuffer vbo(QOpenGLBuffer::VertexBuffer);
        vbo.create();
        vbo.bind();
        vbo.setUsagePattern(QOpenGLBuffer::DynamicDraw);

        int maxVerticesSize = 0;
        for (const auto &segment : qAsConst(pathData)) {
            maxVerticesSize = qMax(maxVerticesSize, segment.size());
        }
        vbo.allocate(maxVerticesSize * sizeof(QVector2D));

        m_pathShaderProgram->bind();

        int posAttr = m_pathShaderProgram->attributeLocation("a_position");
        m_pathShaderProgram->enableAttributeArray(posAttr);

        for (const auto &segment : qAsConst(pathData)) {
            QVector2D *vboData = static_cast<QVector2D *>(vbo.map(QOpenGLBuffer::WriteOnly));
            std::copy(segment.constBegin(), segment.constEnd(), vboData);
            vbo.unmap();

            m_pathShaderProgram->setUniformValue("u_useAlphaMask", m_IsClippingDirty);
            m_pathShaderProgram->setUniformValue("u_alphaMaskTexture", 0);

            QColor color = qtPaint->color();

            // Gradient code starts here
            if (!color.isValid()) {

                const auto *gradient = qtPaint->brush().gradient();
                if (gradient) {
                    configureGradientShader(gradient);
                } else {
                    m_pathShaderProgram->setUniformValue("u_useGradient", false);
                }
                m_pathShaderProgram->setUniformValue("u_color", QColor(255, 0, 0, 255));
            } else {
                m_pathShaderProgram->setUniformValue("u_useGradient", false);
                m_pathShaderProgram->setUniformValue("u_color", color);
            }
            // Gradient code ends here

            m_pathShaderProgram->setUniformValue("u_viewportDimensions", QVector2D(m_viewportWidth, m_viewportHeight));
            m_pathShaderProgram->setUniformValue("u_projection", m_projectionMatrix);
            m_pathShaderProgram->setUniformValue("u_transform", transform());
            m_pathShaderProgram->setAttributeBuffer(posAttr, GL_FLOAT, 0, 2, 0);

            if (qtPaint->paintStyle() == rive::RenderPaintStyle::stroke) {
                glDrawArrays(GL_TRIANGLE_STRIP, 0, segment.count());
            } else {
                glDrawArrays(GL_TRIANGLES, 0, segment.count());
            }
        }

        m_pathShaderProgram->disableAttributeArray(posAttr);
        m_pathShaderProgram->release();
        vao.release();
        vbo.release();

        // this function exists to make the code easier to read
        // it's aware and relays on the states set here, like the active buffer, blending ect.
        composeFinalImage(blendMode);
    }
    glDisable(GL_BLEND);

    if (blendMode == rive::BlendMode::darken || blendMode == rive::BlendMode::lighten) {
        glBlendEquation(GL_FUNC_ADD);
    }

    m_IsClippingDirty = false;
}

void RiveQtOpenGLRenderer::clipPath(rive::RenderPath *path)
{
    m_IsClippingDirty = true;
    m_alphaMaskFramebuffer->bind();

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Clear color with alpha set to zero
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    RiveQtPath *qtPath = static_cast<RiveQtPath *>(path);

    QColor outlineColor(255, 0, 0, 255); // Red color for the outlines

    QVector<QVector<QVector2D>> pathData;

    pathData = qtPath->toVertices();

    for (const auto &segment : qAsConst(pathData)) {
        QOpenGLVertexArrayObject vao;
        vao.create();
        vao.bind();

        QOpenGLBuffer vbo(QOpenGLBuffer::VertexBuffer);
        vbo.create();
        vbo.bind();
        vbo.allocate(segment.constData(), segment.count() * sizeof(QVector2D));

        // Enable the shader program
        m_pathShaderProgram->bind();

        m_pathShaderProgram->setUniformValue("u_useGradient", false);
        m_pathShaderProgram->setUniformValue("u_useAlphaMask", false);
        m_pathShaderProgram->setUniformValue("u_color", outlineColor);

        // Set the viewport dimensions
        m_pathShaderProgram->setUniformValue("u_viewportDimensions", QVector2D(m_viewportWidth, m_viewportHeight));
        m_pathShaderProgram->setUniformValue("u_projection", m_projectionMatrix);
        m_pathShaderProgram->setUniformValue("u_transform", transform());

        // Set up the vertex attribute pointer for the position
        int posAttr = m_pathShaderProgram->attributeLocation("a_position");
        m_pathShaderProgram->enableAttributeArray(posAttr);
        m_pathShaderProgram->setAttributeBuffer(posAttr, GL_FLOAT, 0, 2, 0);

        glDrawArrays(GL_TRIANGLES, 0, vbo.size() / sizeof(QVector2D));

        // Clean up
        m_pathShaderProgram->disableAttributeArray(posAttr);
        m_pathShaderProgram->release();
        vao.release();
        vbo.release();
    }

    m_alphaMaskFramebuffer->release();
}

void RiveQtOpenGLRenderer::drawImage(const rive::RenderImage *image, rive::BlendMode blendMode, float opacity)
{
    return;
    if (!image) {
        return;
    }

    // Set the appropriate OpenGL blend functions
    setupBlendMode(blendMode);

    if (m_useBlendingTexture) {
        qWarning() << "Blending Modes that require shaders are not yet supported for Qt OpenGL Rive Image Rendering.";
        m_useBlendingTexture = false;
    }

    glEnable(GL_BLEND);

    const auto *riveImage = static_cast<const RiveQtImage *>(image);
    const QImage &qImage = riveImage->image();
    QOpenGLTexture texture(qImage);

    // Bind the texture
    texture.bind();

    m_imageQuadVAO.bind();

    float halfWidth = qImage.width() / 2.0f;
    float halfHeight = qImage.height() / 2.0f;

    // todo -> move to RiveQtImage to do this not every frame...
    GLfloat imageData[] = {
        // Positions      // Texture coordinates
        -halfWidth, -halfHeight, 0.0f, 1.0f, // Bottom-left
        halfWidth,  -halfHeight, 1.0f, 1.0f, // Bottom-right
        halfWidth,  halfHeight,  1.0f, 0.0f, // Top-right
        -halfWidth, halfHeight,  0.0f, 0.0f // Top-left
    };

    m_imageQuadVBO.bind();
    m_imageQuadVBO.allocate(imageData, sizeof(imageData));

    m_imageShaderProgram->bind();

    m_imageShaderProgram->setUniformValue("u_texture", 0);
    m_imageShaderProgram->setUniformValue("u_projection", m_projectionMatrix);
    m_imageShaderProgram->setUniformValue("u_transform", transform());
    m_imageShaderProgram->setUniformValue("u_model", modelMatrix());
    m_imageShaderProgram->setUniformValue("u_opacity", opacity);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    m_imageShaderProgram->release();

    m_imageQuadVAO.release();

    glDisable(GL_BLEND);
}

void RiveQtOpenGLRenderer::drawImageMesh(const rive::RenderImage *image, rive::rcp<rive::RenderBuffer> vertices_f32,
                                         rive::rcp<rive::RenderBuffer> uvCoords_f32, rive::rcp<rive::RenderBuffer> indices_u16,
                                         rive::BlendMode blendMode, float opacity)
{
    const QImage &qImage = static_cast<const RiveQtImage *>(image)->image();
    QOpenGLTexture texture(qImage);

    // Bind the texture
    texture.bind();

    RiveQtBufferF32 *qtVertices = static_cast<RiveQtBufferF32 *>(vertices_f32.get());
    RiveQtBufferF32 *qtUvCoords = static_cast<RiveQtBufferF32 *>(uvCoords_f32.get());
    RiveQtBufferU16 *qtIndices = static_cast<RiveQtBufferU16 *>(indices_u16.get());

    // Set the appropriate OpenGL blend functions
    setupBlendMode(blendMode);

    if (m_useBlendingTexture) {
        qWarning() << "Blending Modes that require shaders are not yet supported for Qt OpenGL Rive Image Mesh Rendering.";
        m_useBlendingTexture = false;
    }

    glEnable(GL_BLEND);

    m_meshVAO.bind();
    m_meshVertexBuffer.bind();
    m_meshVertexBuffer.allocate(qtVertices->data(), qtVertices->size());
    m_meshUvBuffer.bind();
    m_meshUvBuffer.allocate(qtUvCoords->data(), qtUvCoords->size());

    m_imageMeshShaderProgram->bind();

    m_imageMeshShaderProgram->setUniformValue("u_texture", 0);
    m_imageMeshShaderProgram->setUniformValue("u_projection", m_projectionMatrix);
    m_imageMeshShaderProgram->setUniformValue("u_transform", transform());
    m_imageMeshShaderProgram->setUniformValue("u_model", modelMatrix());
    m_imageMeshShaderProgram->setUniformValue("u_opacity", opacity);

    m_meshIndexBuffer.bind();
    m_meshIndexBuffer.allocate(qtIndices->data(), qtIndices->size());
    // Issue the draw call with the indexed geometry
    glDrawElements(GL_TRIANGLES, qtIndices->count(), GL_UNSIGNED_SHORT, nullptr);

    // Release the shader program and the VAO
    m_imageMeshShaderProgram->release();
    m_meshVAO.release();

    glDisable(GL_BLEND);
}

void RiveQtOpenGLRenderer::updateModelMatrix(const QMatrix4x4 &modelMatrix)
{
    m_Stack.front().transform = modelMatrix;
}

void RiveQtOpenGLRenderer::updateProjectionMatrix(const QMatrix4x4 &projMatrix)
{
    m_projectionMatrix = projMatrix;
}

void RiveQtOpenGLRenderer::updateViewportSize()
{
    // for clipping we have to get the viewport dimensions
    // the clip texture is scaled on the entire viewport
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    int viewportWidth = viewport[2];
    int viewportHeight = viewport[3];

    if (viewportWidth == m_viewportWidth && viewportHeight == m_viewportHeight) {
        return;
    }

    m_viewportWidth = viewportWidth;
    m_viewportHeight = viewportHeight;

    QOpenGLFramebufferObject *newDisplay = new QOpenGLFramebufferObject(m_viewportWidth, m_viewportHeight);
    delete m_displayFramebuffer;
    m_displayFramebuffer = newDisplay;

    QOpenGLFramebufferObject *newBlend = new QOpenGLFramebufferObject(m_viewportWidth, m_viewportHeight);
    delete m_blendFramebuffer;
    m_blendFramebuffer = newBlend;

    // this is a fullscreen texture! so we push the entire screen size in.
    QOpenGLFramebufferObject *newAlpha = new QOpenGLFramebufferObject(m_viewportWidth, m_viewportHeight);
    delete m_alphaMaskFramebuffer;
    m_alphaMaskFramebuffer = newAlpha;

    m_viewPortQuadVertices = {
        // Positions          // Texture coordinates
        0.0f,
        0.0f,
        0.0f,
        1.0f, // Bottom-left
        static_cast<float>(m_viewportWidth),
        0.0f,
        1.0f,
        1.0f, // Bottom-right
        static_cast<float>(m_viewportWidth),
        static_cast<float>(m_viewportHeight),
        1.0f,
        0.0f, // Top-right
        0.0f,
        static_cast<float>(m_viewportHeight),
        0.0f,
        0.0f // Top-left
    };
}

const QMatrix4x4 &RiveQtOpenGLRenderer::transform()
{
    return m_Stack.back().transform;
}
