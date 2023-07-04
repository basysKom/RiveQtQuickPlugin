
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

    connect(this, &RiveQtQuickItem::internalStateMachineChanged, this, &RiveQtQuickItem::updateStateMachineInputMap, Qt::QueuedConnection);

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
    if (!m_currentArtboardInstance) {
        return;
    }

    if (id < 0 || id >= m_currentArtboardInstance->animationCount()) {
        qCDebug(rqqpItem) << "Requested animation id:" << id << "out of bounds: 0 -" << m_currentArtboardInstance->animationCount();
        m_animationInstance = nullptr;
        return;
    }

    m_animationInstance = m_currentArtboardInstance->animationAt(id);
}

void RiveQtQuickItem::updateStateMachineInputMap()
{
    // maybe its a bit maniac and insane to push raw instance pointers around.
    // well what could go wrong. aka TODO: dont do this
    m_stateMachineInputMap->deleteLater();
    m_stateMachineInputMap = new RiveQtStateMachineInputMap(m_currentStateMachineInstance.get(), this);
    emit stateMachineInterfaceChanged();
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
        m_animationInstance = nullptr;
        emit internalArtboardChanged();
        return;
    }

    m_currentArtboardInstance->updateComponents();

    m_scheduleStateMachineChange = true;

    m_animationInstance = nullptr;

    emit internalArtboardChanged();
}

QSGNode *RiveQtQuickItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    QQuickWindow *currentWindow = window();

    m_renderNode = static_cast<RiveQSGRenderNode *>(oldNode);

    if (!isVisible()) {
        return m_renderNode;
    }

    if (m_scheduleArtboardChange) {
        updateInternalArtboard();

        if (m_renderNode) {
            m_renderNode->updateArtboardInstance(m_currentArtboardInstance.get());
        }

        if (!m_currentArtboardInstance) {
            qCDebug(rqqpItem) << "No artboard loaded.";
            m_currentStateMachineInstance = nullptr;
            emit internalStateMachineChanged();
            return m_renderNode;
        }

        m_scheduleArtboardChange = false;
        updateCurrentStateMachineIndex();
        m_scheduleStateMachineChange = true;

        // Update the default animation when the artboard has changed.
        // We need the animation instance, that's why this looks a little weird.
        m_animationInstance = m_currentArtboardInstance->animationNamed(m_currentArtboardInstance->firstAnimation()->name());
    }

    if (m_scheduleStateMachineChange && m_currentArtboardInstance) {
        m_currentStateMachineInstance = m_currentArtboardInstance->stateMachineAt(m_currentStateMachineIndex);
        emit internalStateMachineChanged();
        m_scheduleStateMachineChange = false;
    }

    if (m_stateMachineInputMap && m_stateMachineInputMap->hasDirtyStateMachine()) {
        m_scheduleArtboardChange = true;
        m_scheduleStateMachineChange = true;
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
        m_renderNode->setArtboardRect(artboardRect());
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

void RiveQtQuickItem::mouseMoveEvent(QMouseEvent *event)
{

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    hitTest(event->position(), rive::ListenerType::move);
#else
    hitTest(event->pos(), rive::ListenerType::move);
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

void RiveQtQuickItem::loadRiveFile(const QString &source)
{
    m_loadingStatus = Loading;
    emit loadingStatusChanged();

    if (source.isEmpty()) {
        m_riveFile = nullptr;
        m_scheduleArtboardChange = true;
        m_scheduleStateMachineChange = true;
        m_artboardInfoList.clear();
        emit artboardsChanged();
        m_animationList.clear();
        emit animationsChanged();
        m_stateMachineList.clear();
        emit stateMachinesChanged();
        m_loadingStatus = Idle;
        emit loadingStatusChanged();
        return;
    }

    QQuickWindow *currentWindow = window();

    if (!currentWindow) {
        qCWarning(rqqpItem) << "No window found";
        m_loadingStatus = Error;
        emit loadingStatusChanged();
        return;
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    connect(currentWindow, &QQuickWindow::beforeFrameBegin, this, &RiveQtQuickItem::renderOffscreen, Qt::ConnectionType::DirectConnection);
#endif

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
            m_artboardInfoList.append(info);
        }
    }
    emit artboardsChanged();

    updateCurrentArtboardIndex();

    m_scheduleArtboardChange = true;
    m_scheduleStateMachineChange = true;

    qCDebug(rqqpItem) << "Successfully imported Rive file.";
    m_loadingStatus = Loaded;
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
    if (!m_riveFile || !m_currentArtboardInstance || !m_currentStateMachineInstance) {
        qCDebug(rqqpItem) << Q_FUNC_INFO << "File, Artboard, StateMachine is null:" << (m_riveFile == nullptr)
                          << (m_currentArtboardInstance == nullptr) << (m_currentStateMachineInstance == nullptr);
        return false;
    }

    if (!isVisible()) {
        return false;
    }

    m_lastMouseX = (pos.x() - m_renderNode->topLeft().rx()) / m_renderNode->scaleFactorX();
    m_lastMouseY = (pos.y() - m_renderNode->topLeft().ry()) / m_renderNode->scaleFactorY();

    for (int i = 0; i < m_currentStateMachineInstance->stateMachine()->listenerCount(); ++i) {
        auto *listener = m_currentStateMachineInstance->stateMachine()->listener(i);

        if (!listener) {
            qCDebug(rqqpInteraction) << "Listener is nullptr";
            continue;
        }

        if (listener->listenerType() == rive::ListenerType::enter || listener->listenerType() == rive::ListenerType::exit) {
            qCWarning(rqqpItem) << "Enter and Exit Actions are not yet supported";
            continue;
        }

        if (listener->listenerType() != type) {
            qCDebug(rqqpInteraction) << "Skipping listener of type" << (int)listener->listenerType() << "expected:" << (int)type;
            continue;
        }

        for (const auto &id : listener->hitShapeIds()) {
            auto *coreElement = m_currentStateMachineInstance->artboard()->resolve(id);

            if (!coreElement->is<rive::Shape>()) {
                qCDebug(rqqpInteraction) << "Skipping non shape";
                continue;
            }

            rive::Shape *shape = dynamic_cast<rive::Shape *>(coreElement);
            const rive::IAABB area = { static_cast<int32_t>(m_lastMouseX), static_cast<int32_t>(m_lastMouseY),
                                       static_cast<int32_t>(m_lastMouseX + 1), static_cast<int32_t>(m_lastMouseY + 1) };
            const bool hit = shape->hitTest(area);

            if (hit) {
                qCDebug(rqqpInteraction) << "ID:" << listener->targetId() << "- Hit";
                listener->performChanges(m_currentStateMachineInstance.get(), rive::Vec2D(m_lastMouseX, m_lastMouseY));
            } else {
                qCDebug(rqqpInteraction) << "ID:" << listener->targetId() << "- No hit";
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
    qCDebug(rqqpItem) << Q_FUNC_INFO;
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

    if (newCurrentStateMachineIndex >= m_riveFile->artboard(m_currentArtboardIndex)->stateMachineCount()) {
        qCDebug(rqqpItem) << "Requested State Machine Index" << newCurrentStateMachineIndex << "out of bounds. Upper bound <"
                          << m_riveFile->artboard()->stateMachineCount();
        return;
    }

    // -1 is a valid value!
    m_currentStateMachineIndex = newCurrentStateMachineIndex;
    emit currentStateMachineIndexChanged();

    m_scheduleStateMachineChange = true; // we have to do this in the render thread.
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
