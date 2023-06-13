
// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <QSGRendererInterface>
#include <QSGRenderNode>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QFile>

#include <rive/node.hpp>
#include <rive/shapes/clipping_shape.hpp>
#include <rive/shapes/rectangle.hpp>
#include <rive/shapes/image.hpp>
#include <rive/shapes/shape.hpp>
#include <rive/assets/image_asset.hpp>
#include <rive/animation/linear_animation_instance.hpp>
#include <rive/generated/shapes/shape_base.hpp>
#include <rive/animation/state_machine_listener.hpp>
#include <rive/file.hpp>

#include "riveqtquickitem.h"
#include "riveqtfactory.h"

RiveQtQuickItem::RiveQtQuickItem(QQuickItem *parent)
    : QQuickItem(parent)
{
    // set global flags and configs of our item
    // TODO: maybe we should also allow Hover Events to be used
    setFlag(QQuickItem::ItemHasContents, true);
    setAcceptedMouseButtons(Qt::AllButtons);

    // we require a window to know the render backend and setup the correct
    // factory
    connect(this, &RiveQtQuickItem::windowChanged, this, [this]() {
        if (m_loadingStatus == LoadingStatus::Loading) {
            loadRiveFile(m_fileSource);
        }
    });

    // TODO: 1) shall we make this Interval match the FPS of the current selected animation
    // TODO: 2) we may want to move this into the render thread to allow the render thread control over the timer,
    //          the timer itself only triggers updates.

    //          All animations are done during the node update, from within the render thread
    //          Note: this is kind of scary as all objects are created in the main thread
    //                which means that we have to be really carefull here to not crash

    // those will be triggered by renderthread once an update was applied to the pipeline
    // this will inform QML about changes
    connect(this, &RiveQtQuickItem::internalArtboardChanged, this, &RiveQtQuickItem::currentArtboardIndexChanged, Qt::QueuedConnection);
    connect(this, &RiveQtQuickItem::internalArtboardChanged, this, &RiveQtQuickItem::updateAnimations, Qt::QueuedConnection);
    connect(this, &RiveQtQuickItem::internalArtboardChanged, this, &RiveQtQuickItem::updateStateMachines, Qt::QueuedConnection);
    connect(
        this, &RiveQtQuickItem::internalStateMachineChanged, this,
        [this]() {
            if (m_stateMachineInputMap) {
                m_stateMachineInputMap->deleteLater();
            }
            // maybe its a bit maniac and insane to push raw instance pointers around.
            // well what could go wrong. aka TODO: dont do this
            m_stateMachineInputMap = new RiveQtStateMachineInputMap(m_currentStateMachineInstance.get(), this);
            emit stateMachineInterfaceChanged();
        },
        Qt::QueuedConnection);

    // do update the index only once we are set up and happy
    connect(this, &RiveQtQuickItem::stateMachineInterfaceChanged, this, &RiveQtQuickItem::currentStateMachineIndexChanged,
            Qt::QueuedConnection);

    m_elapsedTimer.start();
    m_lastUpdateTime = m_elapsedTimer.elapsed();

    update();
}

RiveQtQuickItem::~RiveQtQuickItem() { }

void RiveQtQuickItem::triggerAnimation(int id)
{
    if (m_currentArtboardInstance) {
        if (id >= 0 && id < m_currentArtboardInstance->animationCount()) {
            m_animationInstance = m_currentArtboardInstance->animationAt(id);
        }
    }
}

QSGNode *RiveQtQuickItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    QQuickWindow *currentWindow = window();

    m_renderNode = static_cast<RiveQSGRenderNode *>(oldNode);

    if (m_stateMachineInputMap && m_stateMachineInputMap->hasDirtyStateMachine()) {
        m_scheduleStateMachineChange = true;
        m_scheduleArtboardChange = true;
    }

    if (m_scheduleArtboardChange) {
        m_hasValidRenderNode = false;
        m_currentArtboardInstance = m_riveFile->artboardAt(m_currentArtboardIndex);
        if (m_currentArtboardInstance) {
            m_animationInstance = m_currentArtboardInstance->animationNamed(m_currentArtboardInstance->firstAnimation()->name());
            m_currentStateMachineInstance = nullptr;
            m_currentArtboardInstance->updateComponents();
            if (m_currentStateMachineIndex == -1) {
                setCurrentStateMachineIndex(
                    m_currentArtboardInstance->defaultStateMachineIndex()); // this will set m_scheduleStateMachineChange
            }
        }
        emit internalArtboardChanged();

        if (m_renderNode) {
            m_renderNode->updateArtboardInstance(m_currentArtboardInstance.get());
        }
        m_scheduleArtboardChange = false;
    }

    if (m_scheduleStateMachineChange && m_currentArtboardInstance) {
        m_currentStateMachineInstance = m_currentArtboardInstance->stateMachineAt(m_currentStateMachineIndex);
        emit internalStateMachineChanged();
        m_scheduleStateMachineChange = false;
    }

    if (!m_renderNode && m_currentArtboardInstance) {
        m_renderNode = m_riveQtFactory.renderNode(currentWindow, m_currentArtboardInstance.get(), this);
    }

    qint64 currentTime = m_elapsedTimer.elapsed();
    float deltaTime = (currentTime - m_lastUpdateTime) / 1000.0f;
    m_lastUpdateTime = currentTime;

    if (m_currentArtboardInstance) {
        if (m_animationInstance) {
            bool shouldContinue = m_animationInstance->advance(deltaTime);
            if (shouldContinue) {
                m_animationInstance->apply();
            }
        }
        if (m_currentStateMachineInstance) {
            m_currentStateMachineInstance->advance(deltaTime);
        }
        m_currentArtboardInstance->updateComponents();
        m_currentArtboardInstance->advance(deltaTime);
        m_currentArtboardInstance->update(rive::ComponentDirt::Filthy);
    }

    if (m_renderNode) {
        m_renderNode->markDirty(QSGNode::DirtyForceUpdate);
    }
    if (m_renderNode && m_geometryChanged) {
        m_renderNode->setRect(QRectF(x(), y(), width(), height()));
        m_geometryChanged = false;
    }

    this->update();

    m_hasValidRenderNode = true;
    return m_renderNode;
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
void RiveQtQuickItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    m_geometryChanged = true;

    update();
    QQuickItem::geometryChange(newGeometry, oldGeometry);
}
#endif

void RiveQtQuickItem::mousePressEvent(QMouseEvent *event)
{
    hitTest(event->pos(), rive::ListenerType::down);
    // todo: shall we tell qml about a hit and details about that?
}

void RiveQtQuickItem::mouseMoveEvent(QMouseEvent *event)
{
    hitTest(event->pos(), rive::ListenerType::move);
    // todo: shall we tell qml about a hit and details about that?
}

void RiveQtQuickItem::hoverMoveEvent(QHoverEvent *event)
{
    hitTest(event->pos(), rive::ListenerType::move);
    // todo: shall we tell qml about a hit and details about that?
}

void RiveQtQuickItem::mouseReleaseEvent(QMouseEvent *event)
{
    hitTest(event->pos(), rive::ListenerType::up);
    // todo: shall we tell qml about a hit and details about that?
}

void RiveQtQuickItem::setFileSource(const QString &source)
{
    if (m_fileSource == source) {
        return;
    }

    m_fileSource = source;
    emit fileSourceChanged();
    m_loadingStatus = Loading;
    emit loadingStatusChanged();

    // Load the Rive file when the fileSource is set
    loadRiveFile(source);
}

void RiveQtQuickItem::loadRiveFile(const QString &source)
{
    if (source.isEmpty()) {
        return;
    }

    QQuickWindow *currentWindow = window();

    if (!currentWindow) {
        return;
    }
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    connect(currentWindow, &QQuickWindow::beforeFrameBegin, this, &RiveQtQuickItem::renderOffscreen, Qt::ConnectionType::DirectConnection);
#endif

    QFile file(source);

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning("Failed to open the file.");
        m_loadingStatus = Error;
        emit loadingStatusChanged();
        return;
    }

    m_renderSettings.graphicsApi = currentWindow->rendererInterface()->graphicsApi();

    m_riveQtFactory.setRenderSettings(m_renderSettings);

    QByteArray fileData = file.readAll();

    rive::Span<const uint8_t> dataSpan(reinterpret_cast<const uint8_t *>(fileData.constData()), fileData.size());

    rive::ImportResult importResult;
    m_riveFile = rive::File::import(dataSpan, &m_riveQtFactory, &importResult);

    if (importResult == rive::ImportResult::success) {
        m_artboardInfoList.clear();

        // Get info about the artboards
        for (size_t i = 0; i < m_riveFile->artboardCount(); i++) {
            const auto artBoard = m_riveFile->artboard(i);

            if (artBoard) {
                ArtBoardInfo info;
                info.id = i;
                info.name = QString::fromStdString(m_riveFile->artboardNameAt(i));
                m_artboardInfoList.append(info);
            }
        }
        emit artboardsChanged();

        // TODO allow to preselect the artboard and statemachine and animation at load
        setCurrentArtboardIndex(0);
        if (m_initialStateMachineIndex != -1) {
            setCurrentStateMachineIndex(m_initialStateMachineIndex);
        }

        m_scheduleArtboardChange = true;
        m_scheduleStateMachineChange = true;

        qDebug("Successfully imported Rive file.");
        m_loadingStatus = Loaded;

    } else {
        qDebug("Failed to import Rive file.");
        m_loadingStatus = Error;
    }
    emit loadingStatusChanged();
}

void RiveQtQuickItem::updateAnimations()
{
    m_animationList.clear();

    if (!m_currentArtboardInstance) {
        return;
    }

    for (size_t i = 0; i < m_currentArtboardInstance->animationCount(); i++) {
        const auto animation = m_currentArtboardInstance->animation(i);
        if (animation) {

            qDebug() << "Animation" << i << "found.";
            qDebug() << "  Duration:" << animation->duration();
            qDebug() << "  FPS:" << animation->fps();

            AnimationInfo info;
            info.id = i;
            info.name = QString::fromStdString(animation->name());
            info.duration = animation->duration();
            info.fps = animation->fps();

            qDebug() << "  Name:" << info.name;
            m_animationList.append(info);
        }
    }

    emit animationsChanged();
}

void RiveQtQuickItem::updateStateMachines()
{
    m_stateMachineList.clear();

    if (!m_currentArtboardInstance) {
        return;
    }

    for (size_t i = 0; i < m_currentArtboardInstance->stateMachineCount(); i++) {
        const auto stateMachine = m_currentArtboardInstance->stateMachine(i);
        if (stateMachine) {
            StateMachineInfo info;
            info.id = i;
            info.name = QString::fromStdString(stateMachine->name());

            m_stateMachineList.append(info);
        }
    }
    emit stateMachinesChanged();
}
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
void RiveQtQuickItem::renderOffscreen()
{
    if (m_renderNode && isVisible() && m_hasValidRenderNode) {
        m_renderNode->renderOffscreen();
    }
}
#endif

bool RiveQtQuickItem::hitTest(const QPointF &pos, const rive::ListenerType &type)
{
    if (!m_riveFile || !m_currentArtboardInstance || !m_currentStateMachineInstance)
        return false;

    // Scale mouse position based on current item size and artboard size
    float scaleX = width() / m_currentArtboardInstance->width();
    float scaleY = height() / m_currentArtboardInstance->height();

    // Calculate the uniform scale factor to preserve the aspect ratio
    auto scaleFactor = qMin(scaleX, scaleY);

    // Calculate the new width and height of the item while preserving the aspect ratio
    auto newWidth = m_currentArtboardInstance->width() * scaleFactor;
    auto newHeight = m_currentArtboardInstance->height() * scaleFactor;

    // Calculate the offsets needed to center the item within its bounding rectangle
    auto offsetX = (width() - newWidth) / 2.0;
    auto offsetY = (height() - newHeight) / 2.0;

    rive::HitInfo hitInfo;
    rive::Mat2D transform;

    m_lastMouseX = (pos.x() - offsetX) / scaleFactor;
    m_lastMouseY = (pos.y() - offsetY) / scaleFactor;
    m_listenerType = type;

    for (int i = 0; i < m_currentStateMachineInstance->stateMachine()->listenerCount(); ++i) {
        auto *listener = m_currentStateMachineInstance->stateMachine()->listener(i);
        if (listener) {
            if (listener->listenerType() == rive::ListenerType::enter || listener->listenerType() == rive::ListenerType::exit) {
                qDebug() << "Enter and Exit Actions are not yet supported";
            }

            if (listener->listenerType() == m_listenerType) {
                for (const auto &id : listener->hitShapeIds()) {
                    auto *coreElement = m_currentStateMachineInstance->artboard()->resolve(id);

                    if (coreElement->is<rive::Shape>()) {
                        rive::Shape *shape = dynamic_cast<rive::Shape *>(coreElement);

                        const rive::IAABB area = { static_cast<int32_t>(m_lastMouseX), static_cast<int32_t>(m_lastMouseY),
                                                   static_cast<int32_t>(m_lastMouseX + 1), static_cast<int32_t>(m_lastMouseY + 1) };
                        bool hit = shape->hitTest(area);

                        if (hit) {
                            listener->performChanges(m_currentStateMachineInstance.get(), rive::Vec2D(m_lastMouseX, m_lastMouseY));
                            m_scheduleStateMachineChange = true;
                            m_scheduleArtboardChange = true;
                        }
                    }
                }
            }
        }
    }
    return true;
}

const QVector<AnimationInfo> &RiveQtQuickItem::animations() const
{
    return m_animationList;
}

const QVector<ArtBoardInfo> &RiveQtQuickItem::artboards() const
{
    return m_artboardInfoList;
}

int RiveQtQuickItem::currentArtboardIndex() const
{
    return m_currentArtboardIndex;
}

void RiveQtQuickItem::setCurrentArtboardIndex(int newCurrentArtboardIndex)
{
    if (m_currentArtboardIndex == newCurrentArtboardIndex) {
        return;
    }

    if (!m_riveFile) {
        return;
    }

    if (newCurrentArtboardIndex < 0 || newCurrentArtboardIndex >= m_riveFile->artboardCount()) {
        return;
    }

    m_currentArtboardIndex = newCurrentArtboardIndex;

    m_scheduleArtboardChange = true; // we have to do this in the render thread.
    m_renderNode = nullptr;
    setCurrentStateMachineIndex(-1);

    update();
}

int RiveQtQuickItem::currentStateMachineIndex() const
{
    return m_currentStateMachineIndex;
}

void RiveQtQuickItem::setCurrentStateMachineIndex(int newCurrentStateMachineIndex)
{
    if (m_currentStateMachineIndex == newCurrentStateMachineIndex) {
        return;
    }

    if (!m_riveFile) {
        m_initialStateMachineIndex = newCurrentStateMachineIndex; // file not yet loaded, save the info from qml
        return;
    }

    if (!m_riveFile->artboard())
        return;

    // -1 is a valid value!
    if (newCurrentStateMachineIndex != -1) {
        if (newCurrentStateMachineIndex >= m_riveFile->artboard()->stateMachineCount()) {
            return;
        }
    }

    m_currentStateMachineIndex = newCurrentStateMachineIndex;

    m_scheduleStateMachineChange = true; // we have to do this in the render thread.
    m_stateMachineInputMap->deleteLater();
    m_stateMachineInputMap = nullptr;
    emit stateMachineInterfaceChanged();

    update();
}

RiveQtStateMachineInputMap *RiveQtQuickItem::stateMachineInterface() const
{
    return m_stateMachineInputMap;
}

bool RiveQtQuickItem::interactive() const
{
    return acceptedMouseButtons() == Qt::AllButtons;
}

void RiveQtQuickItem::setInteractive(bool newInteractive)
{
    if ((acceptedMouseButtons() == Qt::AllButtons && newInteractive) || (acceptedMouseButtons() != Qt::AllButtons && !newInteractive)) {
        return;
    }

    if (newInteractive) {
        setAcceptedMouseButtons(Qt::AllButtons);
    }

    if (!newInteractive) {
        setAcceptedMouseButtons(Qt::NoButton);
    }

    emit interactiveChanged();
}
