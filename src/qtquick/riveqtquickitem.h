/*
 * MIT License
 *
 * Copyright (C) 2023 by Jeremias Bosch
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <QElapsedTimer>
#include <QQuickItem>
#include <QQuickPaintedItem>
#include <QSGRenderNode>
#include <QSGTextureProvider>

#include <QTimer>
#include "rive/artboard.hpp"
#include "riveqtfactory.h"
#include "riveqtopenglrenderer.h"
#include "riveqtstatemachineinputmap.h"

// TODO: Move Structs in extra file
struct AnimationInfo
{
  Q_GADGET

  Q_PROPERTY(int id MEMBER id CONSTANT)
  Q_PROPERTY(QString name MEMBER name CONSTANT)
  Q_PROPERTY(float duration MEMBER duration CONSTANT)
  Q_PROPERTY(uint32_t fps MEMBER fps CONSTANT)

public:
  int id;
  QString name;
  float duration;
  uint32_t fps;
};

class ArtBoardInfo
{
  Q_GADGET

  Q_PROPERTY(int id MEMBER id CONSTANT)
  Q_PROPERTY(QString name MEMBER name CONSTANT)

public:
  int id;
  QString name;
};

struct StateMachineInfo
{
  Q_GADGET

  Q_PROPERTY(int id MEMBER id CONSTANT)
  Q_PROPERTY(QString name MEMBER name CONSTANT)

public:
  int id;
  QString name;
};

class RiveQtQuickItem;

class RiveQSGRenderNode : public QSGRenderNode, public QOpenGLFunctions
{

public:
  RiveQSGRenderNode(rive::ArtboardInstance *artboardInstance, RiveQtQuickItem *item);

  void render(const RenderState *state) override;

  StateFlags changedStates() const override
  {
    // Return the necessary state flags
    return QSGRenderNode::BlendState;
  }

  QRectF rect() const override;

  RenderingFlags flags() const override
  {
    // Return the necessary rendering flags
    return QSGRenderNode::BoundedRectRendering | QSGRenderNode::DepthAwareRendering;
  }

  void updateArtboardInstance(rive::ArtboardInstance *artboardInstance)
  {
    m_renderer.reset();
    m_artboardInstance = artboardInstance;
  }

private:
  rive::ArtboardInstance *m_artboardInstance;
  RiveQtOpenGLRenderer m_renderer;
  RiveQtQuickItem *m_item;
  void printOpenGLStates(const char *context);
  QPointF globalPosition(QQuickItem *item);
};

class RiveQtQuickItem : public QQuickItem
{
  Q_OBJECT

  Q_PROPERTY(QString fileSource READ fileSource WRITE setFileSource NOTIFY fileSourceChanged)
  Q_PROPERTY(LoadingStatus loadingStatus READ loadingStatus NOTIFY loadingStatusChanged)
  Q_PROPERTY(QList<ArtBoardInfo> artboards READ artboards NOTIFY artboardsChanged)
  Q_PROPERTY(QList<AnimationInfo> animations READ animations NOTIFY animationsChanged)
  Q_PROPERTY(QList<StateMachineInfo> stateMachines READ stateMachines NOTIFY stateMachinesChanged)

  Q_PROPERTY(int currentArtboardIndex READ currentArtboardIndex WRITE setCurrentArtboardIndex NOTIFY currentArtboardIndexChanged)
  Q_PROPERTY(
    int currentStateMachineIndex READ currentStateMachineIndex WRITE setCurrentStateMachineIndex NOTIFY currentStateMachineIndexChanged)

  Q_PROPERTY(bool interactive READ interactive WRITE setInteractive NOTIFY interactiveChanged)

  Q_PROPERTY(RiveQtStateMachineInputMap *stateMachineInterface READ stateMachineInterface NOTIFY stateMachineInterfaceChanged)
public:
  enum LoadingStatus
  {
    Idle,
    Loading,
    Loaded,
    Error
  };
  Q_ENUM(LoadingStatus)

  RiveQtQuickItem(QQuickItem *parent = nullptr);

  Q_INVOKABLE void triggerAnimation(int id);

  std::unique_ptr<rive::ArtboardInstance> &artboardInstance() { return m_currentArtboardInstance; };

  bool isTextureProvider() const override { return true; }
  QSGTextureProvider *textureProvider() const override { return m_textureProvider.data(); }

  QString fileSource() const { return m_fileSource; }
  void setFileSource(const QString &source);

  LoadingStatus loadingStatus() const { return m_loadingStatus; }

  int currentArtboardIndex() const;
  void setCurrentArtboardIndex(int newCurrentArtboardIndex);

  const QList<ArtBoardInfo> &artboards() const;
  const QList<StateMachineInfo> &stateMachines() const { return m_stateMachineList; }
  const QList<AnimationInfo> &animations() const;

  int currentStateMachineIndex() const;
  void setCurrentStateMachineIndex(int newCurrentStateMachineIndex);

  RiveQtStateMachineInputMap *stateMachineInterface() const;

  bool interactive() const;
  void setInteractive(bool newInteractive);

signals:
  void animationsChanged();
  void artboardsChanged();
  void stateMachinesChanged();

  void fileSourceChanged();
  void loadingStatusChanged();

  void currentArtboardIndexChanged();
  void currentStateMachineIndexChanged();

  void interactiveChanged();

  void internalArtboardChanged();
  void internalStateMachineChanged();
  void stateMachineInterfaceChanged();

protected:
  QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override;

  void mousePressEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;

private:
  void loadRiveFile(const QString &source);
  void updateAnimations();
  void updateStateMachines();

  bool hitTest(const QPointF &pos, const rive::ListenerType &type);

  QList<ArtBoardInfo> m_artboards;
  QList<AnimationInfo> m_animationList;
  QList<StateMachineInfo> m_stateMachineList;

  std::unique_ptr<rive::File> m_riveFile;

  mutable QScopedPointer<QSGTextureProvider> m_textureProvider;

  QString m_fileSource;
  LoadingStatus m_loadingStatus { Idle };

  std::unique_ptr<rive::ArtboardInstance> m_currentArtboardInstance { nullptr };
  std::unique_ptr<rive::LinearAnimationInstance> m_animationInstance { nullptr };
  std::unique_ptr<rive::StateMachineInstance> m_currentStateMachineInstance { nullptr };

  bool m_scheduleArtboardChange { false };
  bool m_scheduleStateMachineChange { false };

  int m_currentArtboardIndex { -1 };
  int m_currentStateMachineIndex { -1 };
  int m_initialStateMachineIndex { -1 };

  RiveQtStateMachineInputMap *m_stateMachineInputMap { nullptr };

  RiveQtFactory customFactory { RiveQtFactory::RiveQtRenderType::QOpenGLRenderer };

  QElapsedTimer m_elapsedTimer;
  qint64 m_lastUpdateTime;
};
