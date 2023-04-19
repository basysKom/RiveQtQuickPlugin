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

        Row {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 600

            ScrollView {
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: 200
                clip: true
                background: Rectangle { color: "white" }

                Column {
                    id: columnArtboards
                    anchors.fill: parent
                    spacing: 16
                    padding: 16

                    Repeater {
                        model: riveItem.artboards

                        delegate: Button {
                            text: "id: " + modelData.id + "Name: " + modelData.name

                            onClicked: riveItem.currentArtboardIndex = modelData.id
                        }
                    }
                }
            }

            ScrollView {
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: 200
                clip: true
                background: Rectangle { color: "white" }

                Column {
                    id: columnAnimations
                    anchors.fill: parent
                    spacing: 16
                    padding: 16

                    Repeater {
                        model: riveItem.animations

                        delegate: Button {
                            text: "id: " + modelData.id + "Name: " + modelData.name + ", Duration: " + modelData.duration + ", FPS: " + modelData.fps

                            onClicked: riveItem.triggerAnimation(modelData.id)
                        }
                    }
                }
            }

            ScrollView {
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: 200

                clip: true
                background: Rectangle { color: "white" }

                Column {
                    id: columnStateMachines
                    anchors.fill: parent
                    spacing: 16
                    padding: 16

                    Repeater {
                        model: riveItem.stateMachines

                        delegate: Button {
                            text: "id: " + modelData.id + "Name: " + modelData.name
                            onClicked: riveItem.currentStateMachineIndex = modelData.id
                        }
                    }
                }
            }

            ScrollView {
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: 200

                clip: true
                background: Rectangle { color: "white" }

                Column {
                    id: columnStateMachinesTriggers
                    anchors.fill: parent
                    spacing: 16
                    padding: 16

                    Repeater {
                        model: riveItem.stateMachineInterface.triggers

                        delegate: Button {
                            text: "Trigger: " + modelData
                            onClicked: riveItem.stateMachineInterface.activateTrigger(modelData)
                        }
                    }
                }
            }

            RiveQtQuickItem {
                id: riveItem
                width: 800
                height: 800
                fileSource: ":/bullet_man.riv"

                onStateMachineInterfaceChanged: {
                    // this give a list of all possible properties of the file
                    // those properties can be used in bindings and updated through bindings!
                    console.log("currentStateMachineIndex",
                                JSON.stringify(riveItem.stateMachineInterface))
                    console.log(Object.keys(riveItem.stateMachineInterface))
                }
            }
        }
    }
}
