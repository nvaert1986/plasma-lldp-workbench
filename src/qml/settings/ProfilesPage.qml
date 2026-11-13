// SPDX-FileCopyrightText: 2026 plasma-lldp-workbench contributors
// SPDX-License-Identifier: GPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick
import org.kde.kirigamiaddons.formcard as FormCard

FormCard.FormCardPage {
    id: root

    title: i18nc("@title", "Profiles")
    footer: PendingBar {}

    SendingWarning {}

    FormCard.FormHeader {
        title: i18nc("@title:group", "Act as")
    }

    FormCard.FormCard {
        Repeater {
            model: [
                {
                    id: "computer",
                    icon: "computer",
                    text: i18nc("@action:button profile", "Plain computer"),
                    description: i18nc("@info", "No LLDP-MED, no policies or power, lldpd's own capabilities.")
                },
                {
                    id: "phone",
                    icon: "call-start",
                    text: i18nc("@action:button profile", "IP phone"),
                    description: i18nc("@info", "LLDP-MED class III, asks for the voice and voice signaling VLAN, draws class 2 Power over Ethernet.")
                },
                {
                    id: "softphone",
                    icon: "audio-headset",
                    text: i18nc("@action:button profile", "Softphone"),
                    description: i18nc("@info", "LLDP-MED class I, asks for the softphone voice VLAN.")
                },
                {
                    id: "video",
                    icon: "camera-video",
                    text: i18nc("@action:button profile", "Video conferencing endpoint"),
                    description: i18nc("@info", "LLDP-MED class II, asks for the video conferencing and video signaling VLAN.")
                },
                {
                    id: "accessPoint",
                    icon: "network-wireless",
                    text: i18nc("@action:button profile", "Wireless access point"),
                    description: i18nc("@info", "Bridge and access point capabilities, draws class 4 Power over Ethernet on the spare pairs.")
                },
                {
                    id: "switch",
                    icon: "network-server",
                    text: i18nc("@action:button profile", "Switch"),
                    description: i18nc("@info", "LLDP-MED network connectivity device with bridge and router capabilities.")
                }
            ]

            delegate: FormCard.FormButtonDelegate {
                required property var modelData

                icon.name: modelData.icon
                text: modelData.text
                description: modelData.description
                onClicked: editor.applyProfile(modelData.id)
            }
        }
    }

    FormCard.FormSectionText {
        text: i18nc("@info", "A profile only fills in values: check them on the other pages, then press Apply. Capabilities this computer does not have are left out, because lldpd refuses them.")
    }
}
