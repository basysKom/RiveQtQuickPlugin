// SPDX-FileCopyrightText: 2023 Jonas Kalinka <jonas.kalinka@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

GroupBox {
    id: container

    property alias model: buttonList.model
    property alias delegate: buttonList.delegate

    implicitWidth: 200

    ListView {
        id: buttonList

        anchors.fill: parent

        clip: true
    }
}
