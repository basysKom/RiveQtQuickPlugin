// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

import RiveQtQuickPlugin 1.0

ApplicationWindow {
    visible: true
    width: 1800
    height: 1024
    color: "#222222"

    RowLayout {
        anchors.fill: parent

        ScrollView {
            Layout.fillHeight: true
            Layout.preferredWidth: 200
            clip: true
            background: Rectangle { color: "white" }

            Column {
                id: columnArtboards
                anchors.fill: parent
                spacing: 16
                padding: 16

                Repeater {
                    model: bulletMan.artboards

                    delegate: Button {
                        text: "id: " + modelData.id + "Name: " + modelData.name

                        onClicked: bulletMan.currentArtboardIndex = modelData.id
                    }
                }
            }
        }

        ScrollView {
            Layout.fillHeight: true
            Layout.preferredWidth: 200
            clip: true
            background: Rectangle { color: "white" }

            Column {
                id: columnAnimations
                anchors.fill: parent
                spacing: 16
                padding: 16

                Repeater {
                    model: bulletMan.animations

                    delegate: Button {
                        text: "id: " + modelData.id + "Name: " + modelData.name + ", Duration: " + modelData.duration + ", FPS: " + modelData.fps

                        onClicked: bulletMan.triggerAnimation(modelData.id)
                    }
                }
            }
        }

        ScrollView {
            Layout.fillHeight: true
            Layout.preferredWidth: 200

            clip: true
            background: Rectangle { color: "white" }

            Column {
                id: columnStateMachines
                anchors.fill: parent
                spacing: 16
                padding: 16

                Repeater {
                    model: bulletMan.stateMachines

                    delegate: Button {
                        text: "id: " + modelData.id + "Name: " + modelData.name
                        onClicked: bulletMan.currentStateMachineIndex = modelData.id
                    }
                }
            }
        }

        ScrollView {
            Layout.fillHeight: true
            Layout.preferredWidth: 200

            clip: true
            background: Rectangle { color: "white" }

            Column {
                id: columnStateMachinesTriggers
                anchors.fill: parent
                spacing: 16
                padding: 16

                Repeater {
                    model: bulletMan.stateMachineInterface ? bulletMan.stateMachineInterface.triggers : 0

                    delegate: Button {
                        text: "Trigger: " + modelData
                        onClicked: bulletMan.stateMachineInterface.activateTrigger(modelData)
                    }
                }
            }
        }

        ColumnLayout {
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.preferredWidth: 400

            RiveQtQuickItem {
                id: bulletMan

                Layout.fillWidth: true
                Layout.fillHeight: true

                Layout.minimumHeight: 500

                fileSource: ":/bullet_man.riv"

                currentArtboardIndex: 0
                currentStateMachineIndex: 0

                onStateMachineInterfaceChanged: {
                    if (stateMachineInterface) {
                        console.log(Object.keys(stateMachineInterface))
                    }
                }
            }

            Slider {
                id: slider

                Layout.fillWidth: true
                Layout.fillHeight: true
                from: 1
                value: 25
                to: 100
                stepSize: 0.1
            }

            RiveQtQuickItem {
                id: riveItem

                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumHeight: 500
                fileSource: ":/water-bar-demo.riv"

                currentArtboardIndex: 0
                currentStateMachineIndex: 0

                interactive: false

                Binding {
                    target: riveItem.stateMachineInterface
                    property: "Level"
                    value: slider.value
                    when: riveItem.currentStateMachineIndex !== null
                }
            }
        }
    }
}
