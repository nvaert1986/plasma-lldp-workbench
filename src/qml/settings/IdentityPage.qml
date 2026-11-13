// SPDX-FileCopyrightText: 2026 plasma-lldp-workbench contributors
// SPDX-License-Identifier: GPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick
import org.kde.kirigamiaddons.formcard as FormCard
import "Paths.js" as Paths

FormCard.FormCardPage {
    id: root

    readonly property var capabilities: Paths.at(editor.edited, editor.daemon, "global/capabilities") ?? []

    title: i18nc("@title", "Identity")
    footer: PendingBar {}

    SendingWarning {}

    FormCard.FormHeader {
        title: i18nc("@title:group", "This Computer")
    }

    FormCard.FormCard {
        SettingText {
            path: "global/hostname"
            label: i18nc("@label:textbox", "System name")
            placeholderText: editor.info.announcedName ?? ""
        }
        SettingText {
            path: "global/description"
            label: i18nc("@label:textbox", "System description")
            placeholderText: editor.info.announcedDescription ?? ""
        }
        SettingText {
            path: "global/chassisId"
            label: i18nc("@label:textbox", "Chassis ID")
            placeholderText: i18nc("@info:placeholder", "MAC address of an interface")
        }
        SettingText {
            path: "global/platform"
            label: i18nc("@label:textbox", "Platform")
            description: i18nc("@info", "Only used by CDP.")
        }
    }

    FormCard.FormHeader {
        title: i18nc("@title:group", "Capabilities")
    }

    FormCard.FormCard {
        SettingSwitch {
            path: "global/capabilitiesAdvertise"
            text: i18nc("@option:check", "Send the capabilities")
        }

        FormCard.FormSwitchDelegate {
            id: custom

            text: i18nc("@option:check", "Choose the capabilities")
            description: i18nc("@info", "Otherwise lldpd decides from the network setup.")
            checked: root.capabilities.length > 0
            onToggled: editor.set("global/capabilities", checked ? (editor.info.capabilitiesEnabled ?? []) : [])
        }

        Repeater {
            model: editor.info.capabilitiesAvailable ?? []

            delegate: FormCard.FormCheckDelegate {
                required property string modelData

                text: modelData
                enabled: custom.checked
                checked: root.capabilities.indexOf(modelData) >= 0
                onToggled: {
                    const list = root.capabilities.filter(capability => capability !== modelData);
                    if (checked) {
                        list.push(modelData);
                    }
                    editor.set("global/capabilities", list);
                }
            }
        }

        FormCard.FormTextDelegate {
            description: i18nc("@info", "lldpd only offers the capabilities this computer has. A phone is recognised through LLDP-MED, not a telephone capability.")
        }
    }

    FormCard.FormHeader {
        title: i18nc("@title:group", "Management Address")
    }

    FormCard.FormCard {
        SettingSwitch {
            path: "global/managementAdvertise"
            text: i18nc("@option:check", "Send management addresses")
        }
        SettingText {
            path: "global/managementPattern"
            label: i18nc("@label:textbox", "Addresses or interfaces")
            placeholderText: i18nc("@info:placeholder", "The first IPv4 and IPv6 address")
            description: i18nc("@info", "Comma separated. * matches anything, ! leaves out, !*:* leaves out IPv6.")
        }
    }
}
