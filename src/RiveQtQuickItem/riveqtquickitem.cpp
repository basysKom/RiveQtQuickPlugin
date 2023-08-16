
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

#include "rive/animation/state_machine_input_instance.hpp"
#include "rqqplogging.h"
#include "riveqtquickitem.h"
#include "renderer/riveqtfactory.h"

RiveQtQuickItem::RiveQtQuickItem(QQuickItem *parent)
    : QQuickItem(parent)
{
    // set global flags and configs of our item
    // TODO: maybe we should also allow Hover Events to be used
    setFlag(QQuickItem::ItemHasContents, true);
    setAcceptedMouseButtons(Qt::AllButtons);
    setAcceptHoverEvents(true);

    // we require a window to know the render backend and setup the correct.
    connect(this, &RiveQtQuickItem::windowChanged, this, [this]() { loadRiveFile(m_fileSource); });

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
    connect(this, &RiveQtQuickItem::loadFileAfterUnloading, this, &RiveQtQuickItem::loadRiveFile, Qt::QueuedConnection);

    // do update the index only once we are set up and happy
    connect(this, &RiveQtQuickItem::stateMachineInterfaceChanged, this, &RiveQtQuickItem::currentStateMachineIndexChanged,
            Qt::QueuedConnection);

    m_elapsedTimer.start();
    m_lastUpdateTime = m_elapsedTimer.elapsed();

    m_stateMachineInterface = new RiveStateMachineInput(this);

    update();
}

RiveQtQuickItem::~RiveQtQuickItem() { }

void RiveQtQuickItem::triggerAnimation(int id)
{
    if (!m_currentArtboardInstance) {
        return;
    }

    if (id < 0 || id >= m_currentArtboardInstance->animationCount()) {
        qCDebug(rqqpItem) << "Requested animation id:" << id << "out of bounds: 0 -" << m_currentArtboardInstance->animationCount();
        m_animationInstance = nullptr;
        m_currentAnimationIndex = -1;
        emit currentAnimationIndexChanged();
        return;
    }

    if (m_currentStateMachineIndex > -1) {
        qCWarning(rqqpItem) << "Requested animation id:" << id << "will not animate since a statemachine with id"
                            << m_currentStateMachineIndex << "is active.";
    }

    m_currentAnimationIndex = id;
    m_animationInstance = m_currentArtboardInstance->animationAt(id);

    qCDebug(rqqpItem) << "Selected Animation" << QString::fromStdString(m_animationInstance->name());
    emit currentAnimationIndexChanged();
}

void RiveQtQuickItem::updateInternalArtboard()
{
    m_hasValidRenderNode = false;

    if (m_currentArtboardIndex == -1 && m_initialArtboardIndex != -1) {
        setCurrentArtboardIndex(m_initialArtboardIndex);
    }

    if (m_loadingStatus == Idle && m_currentArtboardInstance) {
        qCDebug(rqqpItem) << "Unloading artboard instance";
        m_currentArtboardInstance = nullptr;
        emit internalArtboardChanged();
        return;
    }

    if (m_loadingStatus != Loaded) {
        qCWarning(rqqpItem) << "Cannot update internal artboard if no rive file is loaded.";
        return;
    }

    if (m_currentArtboardIndex == -1) {
        m_currentArtboardInstance = m_riveFile->artboardDefault();
    } else {
        m_currentArtboardInstance = m_riveFile->artboardAt(m_currentArtboardIndex);
    }

    if (!m_currentArtboardInstance) {
        qCDebug(rqqpItem) << "Artboard changed, but instance is null";
        m_currentStateMachineInstance = nullptr;
        if (m_stateMachineInterface) {
            m_stateMachineInterface->setStateMachineInstance(m_currentStateMachineInstance.get());
        }
        m_animationInstance = nullptr;
        emit internalArtboardChanged();
        return;
    }

    m_currentArtboardInstance->updateComponents();

    m_scheduleStateMachineChange = true;
    m_geometryChanged = true;

    m_animationInstance = nullptr;

    emit internalArtboardChanged();
}

QSGNode *RiveQtQuickItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    if (m_loadingGuard) {
        return oldNode;
    }
    static QElapsedTimer et;
    QQuickWindow *currentWindow = window();

    if (!currentWindow) {
        return oldNode;
    }

    m_renderNode = static_cast<RiveQSGRenderNode *>(oldNode);

    // unload the file from the render thread to make sure its not accessed at time of unloading
    if (m_loadingStatus == Unloading && m_renderNode) {
        qCDebug(rqqpItem) << "Unloading";
        m_loadingGuard = true;
        m_stateMachineInterface->disconnect();
        m_stateMachineInterface->deleteLater();
        m_stateMachineInterface = nullptr;
        // we do not send emit stateMachineInterfaceChanged(); here because that would cause nullptr errors in qml
        // as long as the object is referenced in qml it wont be deleted
        // of course the calls to the stateMachineInterface wont do anything

        m_renderNode->updateArtboardInstance(std::weak_ptr<rive::ArtboardInstance>());

        // reset all
        m_riveFile = nullptr;
        m_scheduleArtboardChange = true;
        m_scheduleStateMachineChange = true;

        // now load the file from the main thread -> connected as Queued Connection to make sure its called in its owning thread
        emit loadFileAfterUnloading(m_fileSource);
        m_frameRate = 0;
        emit frameRateChanged();
        return m_renderNode;
    }

    if (!isVisible()) {
        m_frameRate = 0;
        emit frameRateChanged();
        return m_renderNode;
    }

    if (m_scheduleArtboardChange) {
        updateInternalArtboard();

        if (m_renderNode) {
            m_renderNode->updateArtboardInstance(m_currentArtboardInstance);
        }

        if (!m_currentArtboardInstance) {
            qCDebug(rqqpItem) << "No artboard loaded.";
            m_currentStateMachineInstance = nullptr;
            if (m_stateMachineInterface) {
                m_stateMachineInterface->setStateMachineInstance(m_currentStateMachineInstance.get());
            }
            emit internalStateMachineChanged();
            m_frameRate = 0;
            emit frameRateChanged();
            return m_renderNode;
        }

        m_scheduleArtboardChange = false;
        updateCurrentStateMachineIndex();
        m_scheduleStateMachineChange = true;

        // Update the default animation when the artboard has change.
        // We need the animation instance, that's why this looks a little weird.
        m_animationInstance = m_currentArtboardInstance->animationNamed(m_currentArtboardInstance->firstAnimation()->name());
    }

    if (m_scheduleStateMachineChange && m_currentArtboardInstance) {
        m_currentStateMachineInstance = m_currentArtboardInstance->stateMachineAt(m_currentStateMachineIndex);
        if (m_stateMachineInterface) {
            m_stateMachineInterface->setStateMachineInstance(m_currentStateMachineInstance.get());
        }
        emit internalStateMachineChanged();
        m_scheduleStateMachineChange = false;
    }

    if (m_stateMachineInterface && m_stateMachineInterface->hasDirtyStateMachine()) {
        m_scheduleArtboardChange = true;
        m_scheduleStateMachineChange = true;
    }

    if (!m_renderNode && m_currentArtboardInstance) {
        m_renderNode = m_riveQtFactory.renderNode(currentWindow, m_currentArtboardInstance, this->boundingRect());
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

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // in qt6 this is done in the offscreen render call
    if (m_renderNode && m_geometryChanged) {
        m_renderNode->setRect(QRectF(x(), y(), width(), height()));
        m_renderNode->setArtboardRect(artboardRect());
        m_geometryChanged = false;
    }
#endif

    this->update();

    if (et.isValid()) {
        m_frameRate = int(1000000000 / et.nsecsElapsed());
        emit frameRateChanged();
    }
    et.start();

    m_hasValidRenderNode = true;
    return m_renderNode;
}

void RiveQtQuickItem::componentComplete()
{
    QQuickItem::componentComplete();
    // if we did not get an external interface, create one
    // this is good for introspection of animations
    // and usefull when building more generic views
    //

    connect(m_stateMachineInterface, &RiveStateMachineInput::riveInputsChanged, this,
            &RiveQtQuickItem::stateMachineStringInterfaceChanged);
    m_stateMachineInterface->initializeInternal();
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
void RiveQtQuickItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    m_geometryChanged = true;

    update();
    QQuickItem::geometryChange(newGeometry, oldGeometry);
}
#else
void RiveQtQuickItem::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    m_geometryChanged = true;

    update();
    QQuickItem::geometryChanged(newGeometry, oldGeometry);
}
#endif

void RiveQtQuickItem::mousePressEvent(QMouseEvent *event)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    hitTest(event->position(), rive::ListenerType::down);
#else
    hitTest(event->pos(), rive::ListenerType::down);
#endif
    // todo: shall we tell qml about a hit and details about that?
}

void RiveQtQuickItem::hoverMoveEvent(QHoverEvent *event)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    hitTest(event->position(), rive::ListenerType::move);
#else
    hitTest(event->pos(), rive::ListenerType::move);
#endif
    // todo: shall we tell qml about a hit and details about that?
}

void RiveQtQuickItem::hoverEnterEvent(QHoverEvent *event)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    hitTest(event->position(), rive::ListenerType::enter);
#else
    hitTest(event->pos(), rive::ListenerType::enter);
#endif
}

void RiveQtQuickItem::hoverLeaveEvent(QHoverEvent *event)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    hitTest(event->position(), rive::ListenerType::exit);
#else
    hitTest(event->pos(), rive::ListenerType::exit);
#endif
}

void RiveQtQuickItem::mouseReleaseEvent(QMouseEvent *event)
{

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    hitTest(event->position(), rive::ListenerType::up);
#else
    hitTest(event->pos(), rive::ListenerType::up);
#endif
    // todo: shall we tell qml about a hit and details about that?
}

void RiveQtQuickItem::setFileSource(const QString &source)
{
    if (m_fileSource == source) {
        return;
    }

    m_fileSource = source;
    emit fileSourceChanged();

    // Load the Rive file when the fileSource is set
    loadRiveFile(source);
}

int RiveQtQuickItem::currentAnimationIndex() const
{
    return m_currentAnimationIndex;
}

void RiveQtQuickItem::updateStateMachineValues()
{
    // this sync from rive to qml
    // polling from there and update on diff to qml
    // values may change during an animation
    if (m_stateMachineInterface) {
        m_stateMachineInterface->updateValues();
    }
    update();
}

void RiveQtQuickItem::loadRiveFile(const QString &source)
{
    if (m_loadingStatus != Idle && m_loadingStatus != Unloading && m_loadingStatus != Loading) {
        // clear the data
        m_artboardInfoList.clear();
        emit artboardsChanged();

        m_animationList.clear();
        emit animationsChanged();

        m_stateMachineList.clear();
        emit stateMachinesChanged();

        m_loadingStatus = Unloading;
        emit loadingStatusChanged();

        m_fileSource = source;
        return;
    }

    m_loadingStatus = Loading;
    emit loadingStatusChanged();

    if (source.isEmpty()) {
        m_loadingStatus = Idle;
        emit loadingStatusChanged();
        return;
    }

    QQuickWindow *currentWindow = window();

    if (!currentWindow) {
        qCWarning(rqqpItem) << "No window found";
        m_loadingStatus = Loading;
        emit loadingStatusChanged();
        return;
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    connect(currentWindow, &QQuickWindow::beforeFrameBegin, this, &RiveQtQuickItem::renderOffscreen,
            static_cast<Qt::ConnectionType>(Qt::DirectConnection | Qt::UniqueConnection));
#endif
    connect(currentWindow, &QQuickWindow::beforeSynchronizing, this, &RiveQtQuickItem::updateStateMachineValues,
            static_cast<Qt::ConnectionType>(Qt::DirectConnection | Qt::UniqueConnection));

    QFile file(source);

    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(rqqpItem) << "Failed to open the file " << source;
        m_loadingStatus = Error;
        emit loadingStatusChanged();
        return;
    }

    QByteArray fileData = file.readAll();
    file.close();

    rive::Span<const uint8_t> dataSpan(reinterpret_cast<const uint8_t *>(fileData.constData()), fileData.size());

    rive::ImportResult importResult;
    m_riveFile = rive::File::import(dataSpan, &m_riveQtFactory, &importResult);

    if (importResult != rive::ImportResult::success) {
        qCDebug(rqqpItem) << "Failed to import Rive file.";
        m_loadingStatus = Error;
        emit loadingStatusChanged();
        return;
    }

    m_renderSettings.graphicsApi = currentWindow->rendererInterface()->graphicsApi();
    m_riveQtFactory.setRenderSettings(m_renderSettings);

    // Update artboard info
    m_artboardInfoList.clear();
    for (size_t i = 0; i < m_riveFile->artboardCount(); ++i) {
        const auto artBoard = m_riveFile->artboard(i);

        if (artBoard) {
            ArtBoardInfo info;
            info.id = i;
            info.name = QString::fromStdString(m_riveFile->artboardNameAt(i));
            qCDebug(rqqpInspection) << "ArtBoardInfo" << i << "found.\tName:" << info.name;

            m_artboardInfoList.append(info);
        }
    }
    emit artboardsChanged();

    updateCurrentArtboardIndex();

    m_scheduleArtboardChange = true;
    m_scheduleStateMachineChange = true;

    if (!m_stateMachineInterface) {
        m_stateMachineInterface = new RiveStateMachineInput(this);
        m_stateMachineInterface->initializeInternal();
        emit stateMachineInterfaceChanged();
    }

    qCDebug(rqqpItem) << "Successfully imported Rive file.";
    m_loadingStatus = Loaded;
    m_loadingGuard = false;
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

        if (!animation) {
            continue;
        }

        AnimationInfo info;
        info.id = i;
        info.name = QString::fromStdString(animation->name());
        info.duration = animation->duration();
        info.fps = animation->fps();

        qCDebug(rqqpInspection) << "Animation" << i << "found.\tDuration:" << animation->duration() << "\tFPS:" << animation->fps()
                                << "\tName:" << info.name;

        m_animationList.append(info);
    }

    emit animationsChanged();
}

void RiveQtQuickItem::updateStateMachines()
{
    m_stateMachineList.clear();

    if (!m_currentArtboardInstance) {
        return;
    }

    for (size_t i = 0; i < m_currentArtboardInstance->stateMachineCount(); ++i) {
        const auto stateMachine = m_currentArtboardInstance->stateMachine(i);
        if (stateMachine) {
            StateMachineInfo info;
            info.id = i;
            qDebug() << "SDTATE MACHINE NAME" << stateMachine->name();
            info.name = QString::fromStdString(stateMachine->name());

            qCDebug(rqqpInspection) << "StateMachineInfo" << i << "found.\tName:" << info.name;

            m_stateMachineList.append(info);
        }
    }
    emit stateMachinesChanged();
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
void RiveQtQuickItem::renderOffscreen()
{
    // its okay io call this since we are sure that the renderthread is not active when we get called
    if (m_renderNode && isVisible() && m_hasValidRenderNode) {
        if (m_geometryChanged) {
            m_renderNode->setRect(QRectF(x(), y(), width(), height()));
            m_renderNode->setArtboardRect(artboardRect());
            m_geometryChanged = false;
        }

        m_renderNode->renderOffscreen();
    }
}
#endif

bool RiveQtQuickItem::hitTest(const QPointF &pos, const rive::ListenerType &type)
{
    if (!m_riveFile || !m_currentArtboardInstance || !m_currentStateMachineInstance) {
        //        qCDebug(rqqpItem) << Q_FUNC_INFO << "File, Artboard, StateMachine is null:" << (m_riveFile == nullptr)
        //                          << (m_currentArtboardInstance == nullptr) << (m_currentStateMachineInstance == nullptr);
        return false;
    }

    if (!isVisible()) {
        return false;
    }

    // m_renderNode is managed and owend by the renderThread the calls should be ok (as they only read)
    // but still some potential to cause trouble
    m_lastMouseX = (pos.x() - m_renderNode->topLeft().rx()) / m_renderNode->scaleFactorX();
    m_lastMouseY = (pos.y() - m_renderNode->topLeft().ry()) / m_renderNode->scaleFactorY();
    switch (type) {
    case rive::ListenerType::move:
        m_currentStateMachineInstance->pointerMove(rive::Vec2D(m_lastMouseX, m_lastMouseY));
        return true;
    case rive::ListenerType::down:
        m_currentStateMachineInstance->pointerDown(rive::Vec2D(m_lastMouseX, m_lastMouseY));
        return true;
    case rive::ListenerType::up:
        m_currentStateMachineInstance->pointerUp(rive::Vec2D(m_lastMouseX, m_lastMouseY));
        return true;
    case rive::ListenerType::enter:
    case rive::ListenerType::exit:
        // not handled in rivecpp as well
        return false;
    }

    return false;
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

void RiveQtQuickItem::updateCurrentStateMachineIndex()
{
    qCDebug(rqqpItem) << Q_FUNC_INFO;
    if (!m_currentArtboardInstance) {
        qCDebug(rqqpItem) << "Cannot update state machine index";
        return;
    }

    if (m_initialStateMachineIndex == -1) {
        qCDebug(rqqpItem) << "Setting default state machine:" << m_currentArtboardInstance->defaultStateMachineIndex();
        setCurrentStateMachineIndex(m_currentArtboardInstance->defaultStateMachineIndex());
    } else {
        qCDebug(rqqpItem) << "Setting initial state machine:" << m_currentArtboardInstance->defaultStateMachineIndex();
        setCurrentStateMachineIndex(m_initialStateMachineIndex);
    }
}

QRectF RiveQtQuickItem::artboardRect()
{
    if (!m_currentArtboardInstance) {
        return QRectF();
    }

    float aspectA = m_currentArtboardInstance->width() / m_currentArtboardInstance->height();
    float aspectI = width() / height();

    float scaleX = width() / m_currentArtboardInstance->width();
    float scaleY = height() / m_currentArtboardInstance->height();

    switch (m_renderSettings.fillMode) {
    default:
    case RiveRenderSettings::PreserveAspectFit: {
        float scale = qMin(scaleX, scaleY);
        float artWidth = m_currentArtboardInstance->width() * scale;
        float artHeight = m_currentArtboardInstance->height() * scale;
        float offsetX = aspectA > aspectI ? 0 : (width() - artWidth) / 2;
        float offsetY = aspectI > aspectA ? 0 : (height() - artHeight) / 2;

        return QRectF(offsetX, offsetY, artWidth, artHeight);
    }
    case RiveRenderSettings::PreserveAspectCrop: {
        float scale = qMax(scaleX, scaleY);
        float artHeight = m_currentArtboardInstance->height() * scale;
        float artWidth = m_currentArtboardInstance->width() * scale;
        return QRectF(0, 0, artWidth, artHeight);
    }
    case RiveRenderSettings::Stretch:
        return QRectF(0, 0, width(), height());
    }
}

void RiveQtQuickItem::updateCurrentArtboardIndex()
{
    qCDebug(rqqpItem) << Q_FUNC_INFO;

    if (!m_riveFile) {
        return;
    }

    if (m_initialArtboardIndex != -1) {
        qCDebug(rqqpItem) << "Current artboard index will be set to initial index:" << m_initialArtboardIndex;
        setCurrentArtboardIndex(m_initialArtboardIndex);
        return;
    }

    qCDebug(rqqpItem) << "Trying to set default artboard index";
    const auto defaultArtboard = m_riveFile->artboardDefault();
    if (!defaultArtboard) {
        return;
    }

    const auto info = std::find_if(m_artboardInfoList.constBegin(), m_artboardInfoList.constEnd(),
                                   [&defaultArtboard](const auto &info) { return info.name.toStdString() == defaultArtboard->name(); });

    if (info == m_artboardInfoList.constEnd()) {
        qCDebug(rqqpItem) << "Default artboard not set. Using 0 as index";
        setCurrentArtboardIndex(0);
        return;
    }
    qCDebug(rqqpInspection()) << "Default artboard:" << info->id;

    setCurrentArtboardIndex(info->id);
}

void RiveQtQuickItem::setCurrentArtboardIndex(const int newIndex)
{
    if (!m_riveFile) {
        qCDebug(rqqpItem) << "Setting initial artboard index to" << newIndex;
        m_initialArtboardIndex = newIndex;
        m_scheduleArtboardChange = true;
        return;
    }

    if (newIndex < 0 || newIndex >= m_riveFile->artboardCount()) {
        qCWarning(rqqpItem) << "Cannot select artboard. Index" << newIndex << "out of bounds [0, " << m_riveFile->artboardCount() << "[";
        qCDebug(rqqpItem) << "Trying to set default artboard index";
        const auto defaultArtboard = m_riveFile->artboardDefault();
        if (!defaultArtboard) {
            return;
        }

        const auto info = std::find_if(m_artboardInfoList.constBegin(), m_artboardInfoList.constEnd(),
                                       [&defaultArtboard](const auto &info) { return info.name.toStdString() == defaultArtboard->name(); });

        if (info == m_artboardInfoList.constEnd()) {
            qCDebug(rqqpItem) << "Default artboard not set. Using 0 as index";
            m_currentArtboardIndex = 0;
            return;
        } else {
            qCDebug(rqqpItem) << "Default artboard index:" << info->id;
            m_currentArtboardIndex = info->id;
        }
        emit currentArtboardIndexChanged();

        m_scheduleArtboardChange = true; // we have to do this in the render thread.
        m_renderNode = nullptr;
        return;
    }

    if (m_currentArtboardIndex == newIndex) {
        return;
    }

    qCDebug(rqqpItem) << "Setting current artboard index to" << newIndex;
    m_currentArtboardIndex = newIndex;
    emit currentArtboardIndexChanged();

    m_scheduleArtboardChange = true; // we have to do this in the render thread.
    m_renderNode = nullptr;
    update();
}

int RiveQtQuickItem::currentStateMachineIndex() const
{
    return m_currentStateMachineIndex;
}

void RiveQtQuickItem::setCurrentStateMachineIndex(const int newCurrentStateMachineIndex)
{
    qCDebug(rqqpItem) << Q_FUNC_INFO << newCurrentStateMachineIndex;
    if (m_currentStateMachineIndex == newCurrentStateMachineIndex) {
        return;
    }

    if (!m_riveFile) {
        m_initialStateMachineIndex = newCurrentStateMachineIndex; // file not yet loaded, save the info from qml
        return;
    }

    if (!m_riveFile->artboard(m_currentArtboardIndex)) {
        return;
    }

    // -1 is a valid value!
    m_currentStateMachineIndex = newCurrentStateMachineIndex;
    emit currentStateMachineIndexChanged();

    m_scheduleStateMachineChange = true; // we have to do this in the render thread.

    update();
}

RiveStateMachineInput *RiveQtQuickItem::stateMachineInterface() const
{
    return m_stateMachineInterface;
}

void RiveQtQuickItem::setStateMachineInterface(RiveStateMachineInput *stateMachineInterface)
{
    if (m_stateMachineInterface != nullptr) {
        m_stateMachineInterface->deleteLater();
    }

    m_stateMachineInterface = stateMachineInterface;
    if (m_stateMachineInterface) {
        QQmlEngine::setObjectOwnership(m_stateMachineInterface, QQmlEngine::CppOwnership);
        m_stateMachineInterface->setStateMachineInstance(m_currentStateMachineInstance.get());
        connect(m_stateMachineInterface, &RiveStateMachineInput::riveInputsChanged, this,
                &RiveQtQuickItem::stateMachineStringInterfaceChanged);
    }
    emit stateMachineInterfaceChanged();
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
