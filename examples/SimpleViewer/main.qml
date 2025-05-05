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
    color: "#444c4e"
    header: TabBar {
        id: tabBar

        TabButton {
            text: qsTr("Inspector")
        }
        TabButton {
            text: qsTr("Discover")
        }
    }
    footer: ToolBar {
        Label {
            anchors.left: parent.left
            anchors.right: parent.right
            elide: Label.ElideRight
            color: "crimson"
            text: qsTr("Could not load rive file:") + riveItem.fileSource
            visible: riveItem.loadingStatus === RiveQtQuickItem.Error
        }
    }

    StackLayout {
        anchors.fill: parent

        currentIndex: tabBar.currentIndex

        Page {
            id: inspectorTab

            padding: 5

            RowLayout {
                anchors.fill: parent

                GroupBox {
                    id: fileListView

                    Layout.fillHeight: true
                    implicitWidth: 200

                    title: "Files"

                    ListView {
                        anchors.fill: parent

                        clip: true
                        model: FolderListModel {
                            folder: "qrc:/rive/"
                        }
                        delegate: ItemDelegate {
                            width: ListView.view.width
                            text: model.fileName

                            onClicked: dropView.fileSource = model.filePath
                        }
                        footerPositioning: ListView.OverlayFooter
                        footer: Button {
                            width: ListView.view.width
                            text: "Clear"

                            onClicked: dropView.fileSource = ""
                        }
                    }
                }

                RiveInspectorView {
                    id: dropView

                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    Label {
                        anchors.centerIn: parent
                        text: "Drop some .riv file"
                        visible: !dropView.fileSource
                    }

                    DropArea {
                        id: dropArea

                        anchors.fill: parent

                        onEntered: (drag) => drag.accept(Qt.LinkAction)
                        onDropped: (drop) => dropView.fileSource = drop.urls[0].toString().slice(8)
                    }
                }
            }
        }

        Page {
            id: discoverTab

            padding: 5
            footer: Slider {
                id: slider

                from: 1
                value: 25
                to: 100
                stepSize: 0.1
            }

            RiveQtQuickItem {
                id: riveItem

                anchors.fill: parent

                fileSource: ":/rive/water-bar-demo.riv"
                interactive: false
                currentArtboardIndex: 0
                currentStateMachineIndex: 0
                stateMachineInterface: RiveStateMachineInput {
                    property real level: slider.value
                }
            }
        }
    }
}
