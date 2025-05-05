// SPDX-FileCopyrightText: 2023 Jonas Kalinka <jonas.kalinka@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

import RiveQtQuickPlugin 1.0

Item {
    id: container

    property alias fileSource: riveItem.fileSource

    RowLayout {
        anchors.fill: parent

        ControlPanel {
            Layout.fillHeight: true

            title: "Artboards"
            model: riveItem.artboards
            visible: model !== undefined ? model.length > 0 : false
            delegate: CheckDelegate {
                width: ListView.view.width
                autoExclusive: true
                checked: riveItem.currentArtboardIndex === modelData.id
                text: modelData.name

                onClicked: riveItem.currentArtboardIndex = modelData.id
            }
        }

        ControlPanel {
            Layout.fillHeight: true

            title: "Animations"
            model: riveItem.animations
            visible: model !== undefined ? model.length > 0 : false
            delegate: CheckDelegate {
                width: ListView.view.width
                checked: riveItem.currentAnimationIndex === modelData.id
                text: modelData.name

                onClicked: riveItem.currentAnimationIndex = riveItem.currentAnimationIndex === modelData.id ? -1 : modelData.id
            }
        }

        ControlPanel {
            Layout.fillHeight: true

            title: "State Machines"
            model: riveItem.stateMachines
            visible: model !== undefined ? model.length > 0 : false
            delegate: CheckDelegate {
                width: ListView.view.width
                checked: riveItem.currentStateMachineIndex === modelData.id
                text: modelData.name

                onClicked: riveItem.currentStateMachineIndex = riveItem.currentStateMachineIndex === modelData.id ? -1 : modelData.id
            }
        }

        ControlPanel {
            id: dynamicProperties

            Layout.fillHeight: true

            title: "Properties"
            model: riveItem.stateMachineInterface.riveInputs
            visible: riveItem.stateMachineInterface.riveInputs.length > 0

            delegate: ColumnLayout {
                width: ListView.view.width

                ItemDelegate {
                    Layout.fillWidth: true

                    visible: modelData.type != RiveStateMachineInput.RiveBoolean
                    text: {
                        if (modelData.type == RiveStateMachineInput.RiveTrigger) {
                            return modelData.text
                        } else if (modelData.type == RiveStateMachineInput.RiveNumber) {
                            return modelData.text + ": " + riveItem.stateMachineInterface.listenTo(modelData.text).value
                        } else {
                            return modelData.text + " " + modelData.type
                        }
                    }

                    onClicked: {
                        if (modelData.type == RiveStateMachineInput.RiveTrigger) {
                            riveItem.stateMachineInterface.callTrigger(modelData.text)
                        }
                    }
                }

                SwitchDelegate {
                    Layout.fillWidth: true

                    text: modelData.text
                    visible: modelData.type == RiveStateMachineInput.RiveBoolean
                    checked: riveItem.stateMachineInterface.listenTo(modelData.text).value

                    onClicked: riveItem.stateMachineInterface.setRiveProperty(modelData.text,
                                                                              !riveItem.stateMachineInterface.listenTo(modelData.text).value)
                }

                Slider {
                    id: slider

                    Layout.fillWidth: true

                    from: 1
                    value: visible ? riveItem.stateMachineInterface.listenTo(modelData.text).value : 0
                    to: 100
                    stepSize: 1
                    visible: modelData.type == RiveStateMachineInput.RiveNumber

                    onValueChanged: riveItem.stateMachineInterface.setRiveProperty(modelData.text, value)
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true

            color: "grey"
            border.color: "red"

            RiveQtQuickItem {
                id: riveItem

                anchors.fill: parent
                anchors.margins: parent.border.width

                currentArtboardIndex: 0
                currentStateMachineIndex: -1

                renderQuality: RiveQtQuickItem.Medium
                postprocessingMode: RiveQtQuickItem.None
                fillMode: RiveQtQuickItem.PreserveAspectFit

                Label {
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
