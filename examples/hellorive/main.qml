// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtCore
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import RiveQtQuickPlugin

ApplicationWindow {
    id: window

    width: 800
    height: 600
    visible: true
    color: "#444c4e"
    title: qsTr("Rive Plugin Demo")
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

    SplitView {
        anchors.fill: parent

        RiveQtQuickItem {
            id: riveItem

            SplitView.fillWidth: true
            SplitView.minimumWidth: window.width / 2

            fillMode: RiveQtQuickItem.PreserveAspectFit
            // not used by software backend
            renderQuality: RiveQtQuickItem.Medium
            postprocessingMode: RiveQtQuickItem.SMAA

            fileSource: ":/rive/travel-icons-pack.riv"

            DropArea {
                id: dropArea

                anchors.fill: parent

                onEntered: (drag) => drag.accept(Qt.LinkAction)
                onDropped: (drop) => riveItem.fileSource = drop.urls[0].toString().slice(8)
            }
        }

        Pane {
            id: controlPanel

            SplitView.minimumWidth: window.width / 5

            ColumnLayout {
                anchors.left: parent.left
                anchors.right: parent.right

                Label {
                    Layout.fillWidth: true

                    text: qsTr("Artboards")
                }

                ComboBox {
                    Layout.fillWidth: true

                    model: riveItem.artboards.map(artboard => artboard.name)

                    onActivated: (index) => riveItem.currentArtboardIndex = index
                }

                Label {
                    Layout.fillWidth: true

                    text: qsTr("Animations")
                }

                ComboBox {
                    Layout.fillWidth: true

                    model: riveItem.animations.map(animation => `${animation.name} (${animation.duration} ms)`)

                    onActivated: (index) => riveItem.currentAnimationIndex = index
                }

                Label {
                    Layout.fillWidth: true

                    text: qsTr("State Machines")
                }

                ComboBox {
                    Layout.fillWidth: true

                    model: [qsTr('Disabled')].concat(riveItem.stateMachines.map(stateMachine => stateMachine.name))

                    onActivated: (index) => riveItem.currentStateMachineIndex = index - 1
                }
            }
        }
    }
}

