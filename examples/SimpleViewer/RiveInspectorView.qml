// SPDX-FileCopyrightText: 2023 Jonas Kalinka <jonas.kalinka@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH

// SPDX-License-Identifier: LGPL-3.0-or-later
import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

import RiveQtQuickPlugin 1.0

Item {
    id: root

    property alias fileSource: riveItem.fileSource

    RowLayout {
        anchors.fill: parent

        ControlPanel {
            title: "Artboards"

            model: riveItem.artboards
            highlightedIndex: riveItem.currentArtboardIndex
            visible: model !== undefined ? model.length > 0 : false
            onClicked: modelData => {
                           if (riveItem.currentArtboardIndex === modelData.id) {
                               riveItem.currentArtboardIndex = -1
                           } else {
                               riveItem.currentArtboardIndex = modelData.id
                           }
                       }

            Layout.fillHeight: true
            Layout.minimumWidth: 200
            Layout.minimumHeight: 200
        }

        ControlPanel {
            title: "Animations"

            model: riveItem.animations
            highlightedIndex: riveItem.currentAnimationIndex
            visible: model !== undefined ? model.length > 0 : false
            onClicked: modelData => {
                           if (riveItem.currentAnimationIndex === modelData.id) {
                               riveItem.currentAnimationIndex = -1
                           } else {
                               riveItem.currentAnimationIndex = modelData.id
                           }
                       }

            Layout.fillHeight: true
            Layout.minimumWidth: 200
            Layout.minimumHeight: 200
        }

        ControlPanel {
            title: "State Machines"

            model: riveItem.stateMachines
            highlightedIndex: riveItem.currentStateMachineIndex
            visible: model !== undefined ? model.length > 0 : false
            onClicked: modelData => {
                           if (riveItem.currentStateMachineIndex === modelData.id) {
                               riveItem.currentStateMachineIndex = -1
                           } else {
                               riveItem.currentStateMachineIndex = modelData.id
                           }
                       }

            Layout.fillHeight: true
            Layout.minimumWidth: 200
            Layout.minimumHeight: 200
        }

        ControlPanel {
            id: dynamicProperties
            title: "Properties"

            model: riveItem.stateMachineInterface.riveInputs
            visible: riveItem.stateMachineInterface.riveInputs.length > 0

            delegate: Button {
                id: buttonDelegate

                width: dynamicProperties.width

                flat: true
                padding: 2

                checkable: false

                text: {
                    if (modelData.type == RiveStateMachineInput.RiveTrigger) {
                        return modelData.text
                    }

                    if (modelData.type == RiveStateMachineInput.RiveBoolean
                            || modelData.type == RiveStateMachineInput.RiveNumber) {
                        return modelData.text + ": "+riveItem.stateMachineInterface.listenTo(modelData.text).value
                    }
                    return modelData.text + " " + modelData.type
                }

                onClicked: {
                    if (modelData.type == RiveStateMachineInput.RiveTrigger) {
                        riveItem.stateMachineInterface.callTrigger(modelData.text)
                    }
                    if (modelData.type == RiveStateMachineInput.RiveBoolean) {
                        riveItem.stateMachineInterface.setRiveProperty(modelData.text,
                                                                       !riveItem.stateMachineInterface.listenTo(modelData.text).value)
                    }
                }

                Slider {
                    id: slider

                    visible: modelData.type == RiveStateMachineInput.RiveNumber

                    anchors.fill: parent

                    from: 1
                    value: visible ? riveItem.stateMachineInterface.listenTo(modelData.text).value : 0
                    to: 100
                    stepSize: 1

                    onValueChanged: riveItem.stateMachineInterface.setRiveProperty(modelData.text, value)
                }
            }

            Layout.fillHeight: true
            Layout.minimumWidth: 200
            Layout.minimumHeight: 200
        }

        Rectangle {
            color: "black"
            Layout.fillHeight: true
            Layout.fillWidth: true

            Rectangle {
                anchors.fill: parent
                anchors.margins: 30
                color: "grey"
                border.color: "red"

                RiveQtQuickItem {
                    id: riveItem

                    anchors.fill: parent
                    anchors.margins: 1

                    currentArtboardIndex: 0
                    currentStateMachineIndex: -1

                    renderQuality: RiveQtQuickItem.Medium
                    postprocessingMode: RiveQtQuickItem.None
                    fillMode: RiveQtQuickItem.PreserveAspectFit

                    onStateMachineStringInterfaceChanged: {
                        dynamicProperties.model = riveItem.stateMachineInterface.riveInputs
                    }

                    Text {
                        anchors {
                            top: parent.top
                            right: parent.right
                            margins: 5
                        }
                        color: "red"
                        text: riveItem.frameRate
                        font.pixelSize: 30
                    }
                }
            }
        }
    }
}
