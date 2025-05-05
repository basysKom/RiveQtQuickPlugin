// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QQuickItem>
#include <QSGRenderNode>
#include <QSGTextureProvider>
#include <QElapsedTimer>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QQuickPaintedItem>
#define RiveQtQuickItemBase QQuickPaintedItem
#else
#define RiveQtQuickItemBase QQuickItem
#endif

#include "rivestatemachineinput.h"
#include "datatypes.h"
#include "renderer/riveqtfactory.h"

#include <rive/listener_type.hpp>
#include <rive/animation/state_machine_instance.hpp>
#include <rive/animation/linear_animation_instance.hpp>
#include <rive/animation/animation_state_instance.hpp>

#if defined(RIVEQTQUICKITEM_LIBRARY)
#    define RIVEQTQUICKITEM_EXPORT Q_DECL_EXPORT
#else
#    define RIVEQTQUICKITEM_EXPORT Q_DECL_IMPORT
#endif

class RiveQSGRenderNode;
/**
 * \class RiveQtQuickItem
 * \brief A quick item for Rive-based animations.
 * \details This class represents a QQuickItem that provides functionalities to work with Rive animations.
 *
 * \author Jeremias Bosch, Jonas Kalinka, basysKom GmbH
 * \date 2023/08/01
 */
class RiveQtQuickItem : public RiveQtQuickItemBase
{
    Q_OBJECT

    /**
     * \property RiveQtQuickItem::fileSource
     *
     * \brief Represents the source file for the Rive animation.
     *
     * This property holds the path to the Rive animation file that will be used
     * by the RiveQtQuickItem. Setting this property will initiate the loading
     * process for the given animation. Once the animation is successfully loaded,
     * users can manipulate and control playback via the other properties and methods
     * provided by this class.
     *
     * \note Ensure that the provided file path is valid and the file is accessible.
     *
     * \par Example:
     * \code
     * RiveQtQuickItem {
     *  fileSource: "/path/to/rive/file.riv"
     * }
     * \endcode
     */
    Q_PROPERTY(QString fileSource READ fileSource WRITE setFileSource NOTIFY fileSourceChanged)

    /**
     * \property RiveQtQuickItem::loadingStatus
     *
     * \brief Indicates the loading status of the Rive file.
     *
     * This property represents the current state of the Rive file loading process.
     *
     * The possible statuses are:
     * - \em Idle: No file is being loaded.
     * - \em Loading: A file is currently being loaded.
     * - \em Loaded: The file has been successfully loaded.
     * - \em Error: An error occurred during the loading process.
     * - \em Unloading: The file is currently being unloaded.
     *
     * \note Monitoring this property is essential to determine the state of the animation and to handle any potential loading errors.
     */
    Q_PROPERTY(LoadingStatus loadingStatus READ loadingStatus NOTIFY loadingStatusChanged)

    /**
     * \property RiveQtQuickItem::artboards
     *
     * \brief Contains a list of available artboards.
     *
     * This property holds a list of artboards present in the loaded Rive file.
     * Each artboard contains meta information, as defined by the \em ArtBoardInfo class,
     * providing details about the artboard's specifications and properties.
     *
     * \par Structure Details:
     * The \em ArtBoardInfo class contains the following properties:
     * - \em id: A unique identifier for the artboard.
     * - \em name: The name of the artboard.
     *
     * \par Example:
     * \code
     * RiveQtQuickItem {
     *     Component.onCompleted: {
     *         console.log(artboards[0].name); // Outputs the name of the first artboard
     *     }
     * }
     * \endcode
     */
    Q_PROPERTY(QVector<ArtBoardInfo> artboards READ artboards NOTIFY artboardsChanged)

    /**
     * \property RiveQtQuickItem::animations
     *
     * \brief Contains a list of available animations.
     *
     * This property holds a list of animations present in the loaded Rive file.
     * Each animation entry, as defined by the \em AnimationInfo struct, provides
     * meta information like the animation's name, duration, frame rate, and ID.
     *
     * \par Structure Details:
     * The \em AnimationInfo struct contains the following properties:
     * - \em id: A unique identifier for the animation.
     * - \em name: The name of the animation.
     * - \em duration: The total duration of the animation in seconds.
     * - \em fps: The frame rate at which the animation should be played, expressed in frames per second.
     *
     * \par Example:
     * \code
     * RiveQtQuickItem {
     *     Component.onCompleted: {
     *         console.log(animations[0].name); // Outputs the name of the first animation
     *     }
     * }
     * \endcode
     */
    Q_PROPERTY(QVector<AnimationInfo> animations READ animations NOTIFY animationsChanged)

    /**
     * \property RiveQtQuickItem::stateMachines
     *
     * \brief Contains a list of available state machines.
     *
     * This property provides a list of state machines that are defined in the loaded Rive file.
     * Each state machine entry, as defined by the \em StateMachineInfo struct, provides
     * meta information, helping users to understand the structure and possible states of the machine.
     *
     * \par Structure Details:
     * The \em StateMachineInfo struct contains the following properties:
     * - \em id: A unique identifier for the state machine.
     * - \em name: The name of the state machine.
     *
     * \par Example:
     * \code
     * RiveQtQuickItem {
     *     Component.onCompleted: {
     *         console.log(stateMachines[0].name); // Outputs the name of the first state machine
     *     }
     * }
     * \endcode
     */
    Q_PROPERTY(QVector<StateMachineInfo> stateMachines READ stateMachines NOTIFY stateMachinesChanged)

    /**
     * \property RiveQtQuickItem::currentArtboardIndex
     *
     * \brief Represents the currently active artboard index.
     *
     * Set this property to change the active artboard being displayed. The index corresponds
     * to the position of the artboard in the `artboards` list. Changing the index will
     * immediately reflect the change in the displayed content.
     *
     * \par Example:
     * \code
     * RiveQtQuickItem {
     *     currentArtboardIndex: 1 // Sets the second artboard as active
     * }
     * \endcode
     */
    Q_PROPERTY(int currentArtboardIndex READ currentArtboardIndex WRITE setCurrentArtboardIndex NOTIFY currentArtboardIndexChanged)

    /**
     * \property RiveQtQuickItem::currentAnimationIndex
     *
     * \brief Represents the currently active animation index.
     *
     * Change this property's value to alter the animation being played back. The index
     * corresponds to the position of the animation in the `animations` list. Modifying
     * the index will start the specified animation.
     *
     * \par Structure Details:
     * The \em AnimationInfo struct contains the following properties:
     * - \em id: A unique integer that identifies the animation.
     * - \em name: The name of the animation.
     * - \em duration: The total duration of the animation in seconds.
     * - \em fps: The frame rate at which the animation should be played, expressed in frames per second.
     *
     * \note Setting the property has no effect if a state machine is currently active.
     * A value of -1 for `currentStateMachineIndex` signifies that no state machine is selected.
     *
     * \par Example:
     * \code
     * RiveQtQuickItem {
     *     currentAnimationIndex: 2 // Starts the third animation
     * }
     * \endcode
     */
    Q_PROPERTY(int currentAnimationIndex READ currentAnimationIndex WRITE triggerAnimation NOTIFY currentAnimationIndexChanged)

    /**
     * \property RiveQtQuickItem::currentStateMachineIndex
     *
     * \brief Represents the currently active state machine index.
     *
     * Altering this property's value changes the active state machine. The index corresponds
     * to the position of the state machine in the `stateMachines` list. Set the property to `-1`
     * to deselect any active state machine.
     *
     * \par Example:
     * \code
     * RiveQtQuickItem {
     *     currentStateMachineIndex: 0 // Activates the first state machine
     * }
     * \endcode
     */
    Q_PROPERTY(
        int currentStateMachineIndex READ currentStateMachineIndex WRITE setCurrentStateMachineIndex NOTIFY currentStateMachineIndexChanged)

    /**
     * \property RiveQtQuickItem::interactive
     *
     * \brief Indicates if the item is interactive.
     *
     * When set to true, the item responds to mouse interactions. If set to false,
     * it will not register any mouse interactions, except hover effects.
     *
     * \par Example:
     * \code
     * RiveQtQuickItem {
     *     interactive: false // Disables mouse interactions
     * }
     * \endcode
     */
    Q_PROPERTY(bool interactive READ interactive WRITE setInteractive NOTIFY interactiveChanged)

    /**
     * \property RiveQtQuickItem::stateMachineInterface
     *
     * \brief Represents the state machine property interface.
     *
     * This object allows introspection and manipulation of the properties and states
     * provided by Rive's state machine. For more details and advanced usage, refer
     * to the detailed documentation specific to RiveStateMachineInput.
     *
     * \par Example:
     * \code
     * RiveQtQuickItem {
     *     stateMachineInterface: RiveStateMachineInput {
     *         property real level: slider.value // level must match a property name in the animation for this to work
     *     }
     * }
     *
     * Slider {
     *      id: slider
     *      from: 1
     *      value: 25
     *      to: 100
     *      stepSize: 0.1
     *  }
     * \endcode
     */
    Q_PROPERTY(RiveStateMachineInput *stateMachineInterface READ stateMachineInterface WRITE setStateMachineInterface NOTIFY
                   stateMachineInterfaceChanged)

    /**
     * \property RiveQtQuickItem::renderQuality
     *
     * \brief Represents the render quality setting.
     *
     * This property defines the visual quality of the rendered Rive animation.
     *
     * The possible qualities are:
     * - \em Low: Provides a lower-quality, optimized render. Suitable for less powerful devices.
     * - \em Medium: Provides a balanced render quality suitable for most applications.
     * - \em High: Delivers the highest quality render. May be resource-intensive.
     *
     * \par Example:
     * \code
     * RiveQtQuickItem {
     *     renderQuality: RiveRenderSettings.High // Sets the render quality to high
     * }
     * \endcode
     */
    Q_PROPERTY(RiveRenderSettings::RenderQuality renderQuality READ renderQuality WRITE setRenderQuality NOTIFY renderQualityChanged)

    /**
     * \property RiveQtQuickItem::postprocessingMode
     *
     * \brief Represents the postprocessing
     *
     * The property defines whether the RiveQtQuickItem should be postprocessed
     *
     * The possible modes are:
     * - \em None: No postprocessing is done
     * - \em SMAA: An antialiasing step is executed using the SMAA (Subpixel Morphological Antialiasing) algorithm
     *
     * \par Example:
     * \code
     * RiveQtQuickItem {
     *     postprocessingMode: RiveRenderSettings.SMAA // do antialiasing using SMAA
     * }
     * \endcode
     */
    Q_PROPERTY(RiveRenderSettings::PostprocessingMode postprocessingMode READ postprocessingMode WRITE setPostprocessingMode NOTIFY
                   postprocessingModeChanged)

    /**
     * \property RiveQtQuickItem::fillMode
     *
     * \brief Represents the fill mode setting.
     *
     * This property determines how the Rive animation fits within the bounding box of the item.
     *
     * The possible modes are:
     * - \em Stretch: Stretches the animation to fill the entire bounding box.
     * - \em PreserveAspectFit: Maintains the original aspect ratio while fitting the animation within the bounds.
     * - \em PreserveAspectCrop: Maintains the original aspect ratio, potentially cropping parts of the animation if they exceed the bounds.
     *
     * \par Example:
     * \code
     * RiveQtQuickItem {
     *     fillMode: RiveRenderSettings.PreserveAspectFit // Sets the fill mode to preserve aspect while fitting
     * }
     * \endcode
     */
    Q_PROPERTY(RiveRenderSettings::FillMode fillMode READ fillMode WRITE setFillMode NOTIFY fillModeChanged)

    /**
     * \property RiveQtQuickItem::frameRate
     *
     * \brief Represents the frame rate of the animation.
     *
     * This property provides the frame rate (in frames per second) at which the loaded Rive animation is intended to be played.
     * It gives an insight into the expected playback speed of the animation.
     *
     * \par Example:
     * \code
     * RiveQtQuickItem {
     *     Component.onCompleted: {
     *         console.log(frameRate); // Outputs the animation's frame rate
     *     }
     * }
     * \endcode
     */
    Q_PROPERTY(int frameRate READ frameRate NOTIFY frameRateChanged)

    QML_ELEMENT

public:
    //! Enum representing the possible loading states.
    enum LoadingStatus
    {
        Idle,
        Loading,
        Loaded,
        Error,
        Unloading,
    };
    Q_ENUM(LoadingStatus)

    RiveQtQuickItem(QQuickItem *parent = nullptr);
    ~RiveQtQuickItem();

    void triggerAnimation(int id);

    bool isTextureProvider() const override;

    QSGTextureProvider *textureProvider() const override;

    QString fileSource() const;
    void setFileSource(const QString &source);

    LoadingStatus loadingStatus() const;

    int currentAnimationIndex() const;
    int currentArtboardIndex() const;
    void setCurrentArtboardIndex(int index);

    const QVector<ArtBoardInfo> &artboards() const;
    const QVector<StateMachineInfo> &stateMachines() const;
    const QVector<AnimationInfo> &animations() const;

    int currentStateMachineIndex() const;
    void setCurrentStateMachineIndex(int index);

    RiveStateMachineInput *stateMachineInterface() const;
    void setStateMachineInterface(RiveStateMachineInput *stateMachineInterface);

    bool interactive() const;
    void setInteractive(bool interactive);

    RiveRenderSettings::PostprocessingMode postprocessingMode() const;
    void setPostprocessingMode(RiveRenderSettings::PostprocessingMode mode);

    RiveRenderSettings::RenderQuality renderQuality() const;
    void setRenderQuality(RiveRenderSettings::RenderQuality quality);

    RiveRenderSettings::FillMode fillMode() const;
    void setFillMode(RiveRenderSettings::FillMode fillMode);

    int frameRate();

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    void paint(QPainter *painter) override;
#endif

signals:
    void animationsChanged();
    void artboardsChanged();
    void stateMachinesChanged();

    void fileSourceChanged();
    void loadingStatusChanged();

    void currentArtboardIndexChanged();
    void currentAnimationIndexChanged();
    void currentStateMachineIndexChanged();

    void interactiveChanged();

    void loadFileAfterUnloading(QString fileName);
    void internalArtboardChanged();
    void internalStateMachineChanged();
    void stateMachineInterfaceChanged();
    void stateMachineStringInterfaceChanged();

    void renderQualityChanged();
    void postprocessingModeChanged();
    void fillModeChanged();

    void frameRateChanged();

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data) override;
    void componentComplete() override;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
#else
    void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) override;
#endif

    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void hoverMoveEvent(QHoverEvent *event) override;
    void hoverEnterEvent(QHoverEvent *event) override;
    void hoverLeaveEvent(QHoverEvent *event) override;

private:
    void loadRiveFile(const QString &source);

    void updateInternalArtboard();
    void updateAnimations();
    void updateStateMachines();
    void updateCurrentArtboardIndex();
    void updateCurrentStateMachineIndex();
    void updateStateMachineValues();

    QRectF artboardRect();

    void renderOffscreen();

    bool hitTest(const QPointF &pos, const rive::ListenerType &type);

    RiveQSGRenderNode *createRenderNode(const RiveRenderSettings &renderSettings,
                                        QQuickWindow *window,
                                        std::weak_ptr<rive::ArtboardInstance> artboardInstance,
                                        const QRectF &geometry);

    QVector<ArtBoardInfo> m_artboardInfoList;
    QVector<AnimationInfo> m_animationList;
    QVector<StateMachineInfo> m_stateMachineList;

    std::unique_ptr<rive::File> m_riveFile;

    mutable QScopedPointer<QSGTextureProvider> m_textureProvider;

    QString m_fileSource;
    LoadingStatus m_loadingStatus { Idle };

    std::shared_ptr<rive::ArtboardInstance> m_currentArtboardInstance { nullptr };
    std::unique_ptr<rive::LinearAnimationInstance> m_animationInstance { nullptr };
    std::shared_ptr<rive::StateMachineInstance> m_currentStateMachineInstance { nullptr };

    bool m_scheduleArtboardChange { false };
    bool m_scheduleStateMachineChange { false };

    int m_currentArtboardIndex { -1 };
    int m_currentAnimationIndex { -1 };
    int m_initialArtboardIndex { -1 };
    int m_currentStateMachineIndex { -1 };
    int m_initialStateMachineIndex { -1 };

    RiveStateMachineInput *m_stateMachineInterface { nullptr };

    RiveRenderSettings m_renderSettings;

    RiveQtFactory m_riveQtFactory { m_renderSettings };

    QElapsedTimer m_elapsedTimer;
    qint64 m_lastUpdateTime;
    bool m_geometryChanged { true };

    bool m_hasValidRenderNode { false };
    float m_lastMouseX { 0.f };
    float m_lastMouseY { 0.f };

    int m_frameRate { 0 };

    RiveQSGRenderNode *m_renderNode { nullptr };

    bool m_loadingGuard { false };
};
