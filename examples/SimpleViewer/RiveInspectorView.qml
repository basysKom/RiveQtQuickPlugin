
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
            visible: model.length > 0
            onLastClickedChanged: riveItem.currentArtboardIndex = lastClicked

            Layout.fillHeight: true
            Layout.preferredWidth: advandedText ? 300 : 200
        }

        ControlPanel {
            title: "Animations"

            model: riveItem.animations
            visible: model.length > 0
            onLastClickedChanged: riveItem.triggerAnimation(lastClicked)
            advandedText: true

            Layout.fillHeight: true
            Layout.preferredWidth: advandedText ? 300 : 200
        }

        ControlPanel {
            title: "State Machines"

            model: riveItem.stateMachines
            visible: model.length > 0
            onLastClickedChanged: riveItem.currentStateMachineIndex = lastClicked

            Layout.fillHeight: true
            Layout.preferredWidth: advandedText ? 300 : 200
        }

        ControlPanel {
            title: "Triggers"

            model: riveItem.stateMachineInterface ? riveItem.stateMachineInterface.triggers : 0
            visible: model.length > 0
            onLastClickedChanged: riveItem.stateMachineInterface.activateTrigger(model[lastClicked])

            Layout.fillHeight: true
            Layout.preferredWidth: advandedText ? 300 : 200
        }

        RiveQtQuickItem {
            id: riveItem

            Layout.fillWidth: true
            Layout.fillHeight: true

            Layout.minimumHeight: 500

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
