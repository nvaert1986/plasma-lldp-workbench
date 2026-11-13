// SPDX-FileCopyrightText: 2026 plasma-lldp-workbench contributors
// SPDX-License-Identifier: GPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import org.kde.kirigamiaddons.formcard as FormCard
import "Paths.js" as Paths

FormCard.FormCardPage {
    id: root

    title: i18nc("@title", "Interfaces")
    footer: PendingBar {}

    FormCard.FormHeader {
        title: i18nc("@title:group", "All Interfaces")
    }

    FormCard.FormCard {
        SettingText {
            path: "global/interfacePattern"
            label: i18nc("@label:textbox", "Interfaces to use")
            placeholderText: i18nc("@info:placeholder", "All physical interfaces")
            description: i18nc("@info", "Comma separated. * matches anything, ! leaves out, !! keeps.")
        }
        SettingText {
            path: "global/permanentPattern"
            label: i18nc("@label:textbox", "Remember settings of")
            placeholderText: i18nc("@info:placeholder", "No interfaces")
            description: i18nc("@info", "Interfaces whose settings stay when they disappear, such as a dock's.")
        }
        SettingChoice {
            path: "global/portIdType"
            text: i18nc("@label:listbox", "Port ID")
            options: [
                {value: "", text: i18nc("@item port ID", "Automatic")},
                {value: "ifname", text: i18nc("@item port ID", "Interface name")},
                {value: "macaddress", text: i18nc("@item port ID", "MAC address")}
            ]
        }
        SettingChoice {
            visible: editor.info.supportsPortDescriptionSource === true
            path: "global/portDescriptionSource"
            text: i18nc("@label:listbox", "Port description")
            options: [
                {value: "auto", text: i18nc("@item port description", "Automatic")},
                {value: "ifname", text: i18nc("@item port description", "Interface name")},
                {value: "alias", text: i18nc("@item port description", "Interface alias")}
            ]
        }
        SettingChoice {
            path: "global/bondSlaveMacType"
            text: i18nc("@label:listbox", "Source MAC on bond members")
            options: [
                {value: "local", text: i18nc("@item MAC address", "Real MAC, locally administered")},
                {value: "real", text: i18nc("@item MAC address", "Real MAC")},
                {value: "zero", text: i18nc("@item MAC address", "All zeros")},
                {value: "fixed", text: i18nc("@item MAC address", "Fixed (00:60:08:69:97:EF)")}
            ]
        }
        SettingSwitch {
            path: "global/interfaceDescription"
            text: i18nc("@option:check", "Write the neighbor into the interface description")
        }
        SettingSwitch {
            path: "global/promiscuous"
            text: i18nc("@option:check", "Promiscuous mode")
            description: i18nc("@info", "Helps when VLAN offloading hides LLDP frames, as with some Cisco 2960 switches. Stays on after lldpd stops.")
        }
    }

    Repeater {
        model: editor.portNames

        delegate: ColumnLayout {
            id: port

            required property string modelData
            readonly property string base: "ports/" + modelData + "/"

            Layout.fillWidth: true
            spacing: 0

            FormCard.FormHeader {
                title: port.modelData
            }

            FormCard.FormCard {
                SettingChoice {
                    path: port.base + "status"
                    text: i18nc("@label:listbox", "LLDP")
                    options: [
                        {value: "rx-and-tx", text: i18nc("@item LLDP port mode", "Send and receive")},
                        {value: "rx-only", text: i18nc("@item LLDP port mode", "Receive only")},
                        {value: "tx-only", text: i18nc("@item LLDP port mode", "Send only")},
                        {value: "disabled", text: i18nc("@item LLDP port mode", "Off")}
                    ]
                }
                SettingText {
                    path: port.base + "portIdLocal"
                    label: i18nc("@label:textbox", "Fixed port ID")
                    placeholderText: i18nc("@info:placeholder", "Automatic")
                }
                SettingText {
                    path: port.base + "portDescription"
                    label: i18nc("@label:textbox", "Port description")
                }
                SettingNumber {
                    path: port.base + "vlanTx"
                    label: i18nc("@label:spinbox", "Send LLDP tagged in VLAN")
                    from: 0
                    to: 4094
                    textFromValue: (value, locale) => value === 0 ? i18nc("@item VLAN", "Untagged") : String(value)
                    valueFromText: (text, locale) => parseInt(text) || 0
                }
                SettingNumber {
                    path: port.base + "vlanTxPriority"
                    label: i18nc("@label:spinbox", "Priority of the tagged frames")
                    from: 0
                    to: 7
                    visible: Number(Paths.at(editor.edited, editor.daemon, port.base + "vlanTx")) > 0
                }
                SettingSwitch {
                    path: port.base + "vlanTxDei"
                    numeric: true
                    text: i18nc("@option:check", "Drop eligible")
                    visible: Number(Paths.at(editor.edited, editor.daemon, port.base + "vlanTx")) > 0
                }
                SettingText {
                    visible: editor.info.supportsVlanPattern === true
                    path: port.base + "vlanPattern"
                    label: i18nc("@label:textbox", "VLANs to advertise")
                    placeholderText: i18nc("@info:placeholder", "All")
                }
            }
        }
    }
}
