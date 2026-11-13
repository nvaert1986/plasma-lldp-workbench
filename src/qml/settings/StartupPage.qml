// SPDX-FileCopyrightText: 2026 plasma-lldp-workbench contributors
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

FormCard.FormCardPage {
    id: root

    title: i18nc("@title", "Startup Options")
    footer: PendingBar {}

    Kirigami.InlineMessage {
        Layout.fillWidth: true
        Layout.maximumWidth: Kirigami.Units.gridUnit * 30
        Layout.alignment: Qt.AlignHCenter
        Layout.topMargin: Kirigami.Units.largeSpacing
        visible: editor.serviceError.length > 0
        type: Kirigami.MessageType.Error
        text: editor.serviceError
    }

    FormCard.FormHeader {
        title: i18nc("@title:group", "How lldpd Starts")
    }

    FormCard.FormCard {
        SettingChoice {
            path: "daemon/medClass"
            text: i18nc("@label:listbox", "LLDP-MED")
            options: [
                {value: 0, text: i18nc("@item LLDP-MED class", "No LLDP-MED")},
                {value: 1, text: i18nc("@item LLDP-MED class", "Class I, generic endpoint")},
                {value: 2, text: i18nc("@item LLDP-MED class", "Class II, media endpoint")},
                {value: 3, text: i18nc("@item LLDP-MED class", "Class III, communication device such as an IP phone")},
                {value: 4, text: i18nc("@item LLDP-MED class", "Network connectivity device")}
            ]
        }
        SettingSwitch {
            path: "daemon/noInventory"
            text: i18nc("@option:check", "Do not send the LLDP-MED inventory")
        }
        SettingSwitch {
            path: "daemon/hideKernel"
            text: i18nc("@option:check", "Hide the kernel version")
            description: i18nc("@info", "The system description then only says Linux.")
        }
        SettingSwitch {
            path: "daemon/receiveOnly"
            text: i18nc("@option:check", "Receive only")
            description: i18nc("@info", "lldpd never sends, whatever the interfaces are set to.")
        }
        FormCard.FormTextDelegate {
            text: i18nc("@label", "Command line")
            description: editor.service
            descriptionItem.font.family: "monospace"
            descriptionItem.wrapMode: Text.WrapAnywhere
        }
    }

    FormCard.FormSectionText {
        text: i18nc("@info", "lldpd reads these only when it starts. Applying them restarts lldpd, which asks for your password. A restart loses settings made in this app, so they are saved permanently first.")
    }
}
