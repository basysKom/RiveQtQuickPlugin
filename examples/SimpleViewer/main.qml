// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 Jonas Kalinka <jonas.kalinka@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH

// SPDX-License-Identifier: LGPL-3.0-or-later
import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import Qt.labs.folderlistmodel 2.15

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
    }

    StackLayout {
        anchors.top: bar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        currentIndex: bar.currentIndex

        RowLayout {
            anchors.fill: parent

            ScrollView {
                id: scrollView

                Layout.fillHeight: true
                Layout.preferredWidth: (!dropView.fileSource || hovered) ? 300 : 150

                hoverEnabled: true

                padding: 16

                clip: true
                background: Rectangle {
                    color: "lightgray"
                }

                FolderListModel {
                    id: folderModel
                    folder: "qrc:/rive"
                    showDirs: false

                    Component.onCompleted: {
                        console.log("item count", count)
                    }
                    onStatusChanged: if (folderModel.status == FolderListModel.Ready) {
                                         console.log('Loaded', count)
                                     }
                }

                Column {
                    id: buttonColumn

                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    spacing: 16

                    Repeater {
                        id: buttonRepeater
                        model: folderModel
                        delegate: Button {
                            text: model.fileName
                            onClicked: {
                                console.log("clicked", model.fileName)
                                dropView.fileSource = ":/rive/" + model.fileName
                            }
                        }
                    }
                }
            }
            RiveInspectorView {
                id: dropView

                Layout.fillHeight: true
                Layout.fillWidth: true

                Item {
                    anchors.fill: parent

                    Text {
                        visible: !dropView.fileSource
                        text: "Drop some .riv file"
                        color: "white"
                        anchors.centerIn: parent
                    }

                    DropArea {
                        id: dropArea
                        anchors.fill: parent
                        onEntered: {
                            drag.accept(Qt.LinkAction)
                        }
                        onDropped: {
                            console.log(drop.urls)
                            dropView.fileSource = drop.urls[0].toString().slice(7)
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
                    fileSource: ":/rive/water-bar-demo.riv"

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
    }
}
