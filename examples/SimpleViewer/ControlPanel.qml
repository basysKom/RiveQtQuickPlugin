// SPDX-FileCopyrightText: 2023 Jonas Kalinka <jonas.kalinka@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH

// SPDX-License-Identifier: LGPL-3.0-or-later
import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

Rectangle {
    id: root

    signal clicked(var modelData)

    property alias model: buttonRepeater.model
    property alias title: title.text
    property int highlightedIndex: -1

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

                        checkable: true
                        checked: highlightedIndex === index

                        text: {
                            let result = ""

                            if (modelData.id !== undefined) {
                                result = "ID: " + modelData.id + " "
                            }

                            if (modelData.name) {
                                result += "Name: " + modelData.name
                            } else {
                                result = modelData
                            }

                            return result
                        }

                        onClicked: {
                            root.clicked(modelData)
                        }
                    }
                }
            }
        }
    }
}
