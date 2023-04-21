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

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

import RiveQtQuickPlugin 1.0

ApplicationWindow {
    visible: true
    width: 1800
    height: 1024
    color: "#222222"

    Item {
        anchors.fill: parent

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

                    width: 400
                    height: 400

                    Layout.fillWidth: true
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
                    from: 1
                    value: 25
                    to: 100
                    stepSize: 0.1
                }

                RiveQtQuickItem {
                    id: riveItem

                    Layout.fillWidth: true
                    width: 400
                    height: 400

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
}
