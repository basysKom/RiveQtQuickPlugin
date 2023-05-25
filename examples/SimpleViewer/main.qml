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

    TabBar {
        id: bar
        width: parent.width
        TabButton {
            text: qsTr("Home")
        }
        TabButton {
            text: qsTr("Discover")
        }
        TabButton {
            text: qsTr("Activity")
        }
        TabButton {
            text: qsTr("Jelly")
        }
        TabButton {
            text: qsTr("Icons")
        }
    }

    StackLayout {
        anchors.top: bar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        currentIndex: bar.currentIndex
        Item {
            id: homeTab

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
            }
        }

        Item {
            id: discoverTab

            ColumnLayout {
                anchors.fill: parent

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

                Slider {
                    id: slider

                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    from: 1
                    value: 25
                    to: 100
                    stepSize: 0.1
                }
            }
        }

        Item {
            id: activityTab

            RowLayout {
                anchors.fill: parent

                ScrollView {
                    Layout.fillHeight: true
                    Layout.preferredWidth: 200
                    clip: true
                    background: Rectangle { color: "white" }

                    Column {
                        id: columnAnimations_juice
                        anchors.fill: parent
                        spacing: 16
                        padding: 16

                        Repeater {
                            model: juice.animations

                            delegate: Button {
                                text: "id: " + modelData.id + "Name: " + modelData.name + ", Duration: " + modelData.duration + ", FPS: " + modelData.fps

                                onClicked: juice.triggerAnimation(modelData.id)
                            }
                        }
                    }
                }

                RiveQtQuickItem {
                    id: juice

                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    Layout.minimumHeight: 500

                    fileSource: ":/juice.riv"

                    currentArtboardIndex: 0
                    currentStateMachineIndex: 0
                }
            }
        }

        Item {
            id: jellyTab

            RowLayout {
                anchors.fill: parent


                ScrollView {
                    Layout.fillHeight: true
                    Layout.preferredWidth: 200
                    clip: true
                    background: Rectangle { color: "white" }

                    Column {
                        id: jelly_columnArtboards
                        anchors.fill: parent
                        spacing: 16
                        padding: 16

                        Repeater {
                            model: jelly.artboards

                            delegate: Button {
                                text: "id: " + modelData.id + "Name: " + modelData.name

                                onClicked: jelly.currentArtboardIndex = modelData.id
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
                        id: jelly_columnAnimations
                        anchors.fill: parent
                        spacing: 16
                        padding: 16

                        Repeater {
                            model: jelly.animations

                            delegate: Button {
                                text: "id: " + modelData.id + "Name: " + modelData.name + ", Duration: " + modelData.duration + ", FPS: " + modelData.fps

                                onClicked: jelly.triggerAnimation(modelData.id)
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
                        id: jelly_columnStateMachines
                        anchors.fill: parent
                        spacing: 16
                        padding: 16

                        Repeater {
                            model: jelly.stateMachines

                            delegate: Button {
                                text: "id: " + modelData.id + "Name: " + modelData.name
                                onClicked: jelly.currentStateMachineIndex = modelData.id
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
                        id: jellyTriggers
                        anchors.fill: parent
                        spacing: 16
                        padding: 16

                        Repeater {
                            model: jelly.stateMachineInterface ? jelly.stateMachineInterface.triggers : 0

                            delegate: Button {
                                text: "Trigger: " + modelData
                                onClicked: jelly.stateMachineInterface.activateTrigger(modelData)
                            }
                        }
                    }
                }

                RiveQtQuickItem {
                    id: jelly

                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    Layout.minimumHeight: 500

                    fileSource: ":/jellyfish_test.riv"

                    currentArtboardIndex: 0
                    currentStateMachineIndex: 0
                }
            }
        }

        Item {
            id: iconsTab

            RowLayout {
                anchors.fill: parent

                ScrollView {
                    Layout.fillHeight: true
                    Layout.preferredWidth: 200
                    clip: true
                    background: Rectangle { color: "white" }

                    Column {
                        id: columnArtboards_iconsTab
                        anchors.fill: parent
                        spacing: 16
                        padding: 16

                        Repeater {
                            model: icons.artboards

                            delegate: Button {
                                text: "id: " + modelData.id + "Name: " + modelData.name

                                onClicked: icons.currentArtboardIndex = modelData.id
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
                        id: columnAnimations_iconsTab
                        anchors.fill: parent
                        spacing: 16
                        padding: 16

                        Repeater {
                            model: icons.animations

                            delegate: Button {
                                text: "id: " + modelData.id + "Name: " + modelData.name + ", Duration: " + modelData.duration + ", FPS: " + modelData.fps

                                onClicked: icons.triggerAnimation(modelData.id)
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
                        id: columnStateMachines_iconsTab
                        anchors.fill: parent
                        spacing: 16
                        padding: 16

                        Repeater {
                            model: icons.stateMachines

                            delegate: Button {
                                text: "id: " + modelData.id + "Name: " + modelData.name
                                onClicked: icons.currentStateMachineIndex = modelData.id
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
                        id: columnStateMachinesTriggers_iconsTab
                        anchors.fill: parent
                        spacing: 16
                        padding: 16

                        Repeater {
                            model: icons.stateMachineInterface ? icons.stateMachineInterface.triggers : 0

                            delegate: Button {
                                text: "Trigger: " + modelData
                                onClicked: icons.stateMachineInterface.activateTrigger(modelData)
                            }
                        }
                    }
                }

                RiveQtQuickItem {
                    id: icons

                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    Layout.minimumHeight: 500

                    fileSource: ":/1298-2487-animated-icon-set-1-color.riv"

                    currentArtboardIndex: 0
                    currentStateMachineIndex: 0

                    onStateMachineInterfaceChanged: {
                        if (stateMachineInterface) {
                            console.log(Object.keys(stateMachineInterface))
                        }
                    }
                }
            }
        }
    }
}
