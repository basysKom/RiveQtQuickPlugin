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

    property int contentWidth: 600
    property int controlPanelWidth: 250

    width: contentWidth + controlPanelWidth
    height: 600
    visible: true
    color: "#444c4e"

    title: qsTr("Rive Plugin Demo")

    Item {
        id: content
        anchors {
            top: parent.top
            bottom: parent.bottom
            left: parent.left
            right: controlPanel.left
        }

        RiveQtQuickItem {
            id: riveItem
            anchors.fill: parent
            fillMode: RiveQtQuickItem.PreserveAspectFit

            // not used by software backend
            renderQuality: RiveQtQuickItem.Medium
            postprocessingMode: RiveQtQuickItem.SMAA

            fileSource: ":/rive/travel-icons-pack.riv"

            onArtboardsChanged: console.log("ARTBOARDS", artboards);
        }

        Text {
            id: errorMessage
            anchors.centerIn: parent
            width: window.width
            horizontalAlignment: Text.AlignHCenter
            font.pointSize: 24
            color: "crimson"
            text: qsTr("Could not load rive file:\n") + riveItem.fileSource 
            visible: riveItem.loadingStatus === RiveQtQuickItem.Error
        }

        DropArea {
            id: dropArea
            anchors.fill: parent
            onEntered: {
                drag.accept(Qt.LinkAction)
            }
            onDropped: {
                riveItem.fileSource = drop.urls[0].toString().slice(7)
            }
        }
    }

    Rectangle {
        id: controlPanel
        
        anchors {
            top: parent.top
            bottom: parent.bottom
            right: parent.right
        }

        width: controlPanelWidth

        color: "#203133"

        Rectangle {
            id: separator
            anchors {
                top: parent.top
                bottom: parent.bottom
                left: parent.left
            }
            width: 2
            color: "black"
            opacity: 0.3
        }

        Column {
            id: column

            anchors {
                fill: parent
                leftMargin: 16
                rightMargin: 16
                topMargin: 32
            }

            spacing: 8

            Label {
                width: parent.width
                wrapMode: Label.Wrap
                horizontalAlignment: Qt.AlignLeft
                text: qsTr("Artboards")
            }

            ComboBox {
                model: riveItem.artboards.map(artboard => artboard.name)
                anchors.left: parent.left
                anchors.right: parent.right

                onActivated: (index) => riveItem.currentArtboardIndex = index
            }

            Item { width: 1; height: 32 }

            Label {
                width: parent.width
                wrapMode: Label.Wrap
                horizontalAlignment: Qt.AlignLeft
                text: qsTr("Animations")
            }

            ComboBox {
                model: riveItem.animations.map(animation => `${animation.name} (${animation.duration} ms)`)
                anchors.left: parent.left
                anchors.right: parent.right

                onActivated: (index) => riveItem.currentAnimationIndex = index
            }

           Item { width: 1; height: 32 }

           Label {
                width: parent.width
                wrapMode: Label.Wrap
                horizontalAlignment: Qt.AlignLeft
                text: qsTr("State Machines")
            }

            ComboBox {
                model: [qsTr('Disabled')].concat(riveItem.stateMachines.map(stateMachine => stateMachine.name))
                anchors.left: parent.left
                anchors.right: parent.right

                onActivated: (index) => riveItem.currentStateMachineIndex = index - 1
            }


        }

    }

}
    
