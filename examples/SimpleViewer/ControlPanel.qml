// SPDX-FileCopyrightText: 2023 Jonas Kalinka <jonas.kalinka@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH

// SPDX-License-Identifier: LGPL-3.0-or-later
import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

Rectangle {
    id: root

    property int lastClicked: -1
    property alias model: buttonRepeater.model
    property alias title: title.text

    clip: true
    color: "white"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 2

        Text {
            id: title
            text: "Title"
            color: "black"
        }

        ScrollView {
            id: scrollView
            Layout.fillHeight: true
            Layout.fillWidth: true
            clip: true
            background: Rectangle {
                color: "white"
            }

            Column {
                id: buttonColumn
                anchors.fill: parent
                spacing: 1

                Repeater {
                    id: buttonRepeater

                    delegate: Button {
                        id: buttonDelegate
                        flat: true
                        padding: 2
                        text: "ID: " + modelData.id + ", Name: " + modelData.name
                              + (", Duration: " + modelData.duration + ", FPS: " + modelData.fps)
                        onClicked: {
                            root.lastClicked = modelData.id !== undefined ? modelData.id : 0
                        }
                    }
                }
            }
        }
    }
}
