// SPDX-FileCopyrightText: 2026 plasma-lldp-workbench contributors
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.InlineMessage {
    Layout.fillWidth: true
    Layout.maximumWidth: Kirigami.Units.gridUnit * 30
    Layout.alignment: Qt.AlignHCenter
    Layout.topMargin: Kirigami.Units.largeSpacing
    visible: true
    type: Kirigami.MessageType.Warning
    text: i18nc("@info", "Every device on the network segment sees what this computer sends. Switches that profile devices, such as Cisco ISE or FortiNAC, can move the port to another VLAN or policy because of it. Only send on networks where that is allowed.")
}
