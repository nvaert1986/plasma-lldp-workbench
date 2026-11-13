// SPDX-FileCopyrightText: 2026 plasma-lldp-workbench contributors
// SPDX-License-Identifier: GPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import org.kde.kirigamiaddons.formcard as FormCard
import "Paths.js" as Paths

FormCard.FormCardPage {
    id: root

    readonly property var roles: [
        {value: "none", text: i18nc("@item power role", "Not sent")},
        {value: "pd", text: i18nc("@item power role", "Powered device (draws power)")},
        {value: "pse", text: i18nc("@item power role", "Power sourcing equipment (supplies power)")}
    ]
    readonly property var priorities: [
        {value: "unknown", text: i18nc("@item power priority", "Unknown")},
        {value: "critical", text: i18nc("@item power priority", "Critical")},
        {value: "high", text: i18nc("@item power priority", "High")},
        {value: "low", text: i18nc("@item power priority", "Low")}
    ]

    function sources(role) {
        if (role === "pse") {
            return [
                {value: "unknown", text: i18nc("@item power source", "Unknown")},
                {value: "primary", text: i18nc("@item power source", "Primary")},
                {value: "backup", text: i18nc("@item power source", "Backup")}
            ];
        }
        return [
            {value: "unknown", text: i18nc("@item power source", "Unknown")},
            {value: "pse", text: i18nc("@item power source", "The switch")},
            {value: "local", text: i18nc("@item power source", "Local")},
            {value: "both", text: i18nc("@item power source", "The switch and local")}
        ];
    }

    function watts(value) {
        return i18nc("@item:valuesuffix power", "%1 W", (value / 1000).toLocaleString(Qt.locale(), "f", 1));
    }

    title: i18nc("@title", "Power over Ethernet")
    footer: PendingBar {}

    FormCard.FormSectionText {
        text: i18nc("@info", "What this computer claims about Power over Ethernet. It changes nothing electrically; the switch may reserve power because of it.")
    }

    Repeater {
        model: editor.portNames

        delegate: ColumnLayout {
            id: port

            required property string modelData
            readonly property string med: "ports/" + modelData + "/medPower/"
            readonly property string dot3: "ports/" + modelData + "/dot3Power/"
            readonly property string medRole: Paths.at(editor.edited, editor.daemon, port.med + "role") ?? "none"
            readonly property string dot3Role: Paths.at(editor.edited, editor.daemon, port.dot3 + "role") ?? "none"
            readonly property int dot3Type: Number(Paths.at(editor.edited, editor.daemon, port.dot3 + "type") ?? 0)

            Layout.fillWidth: true
            spacing: 0

            FormCard.FormHeader {
                title: i18nc("@title:group %1 is a network interface", "%1: LLDP-MED Power", port.modelData)
            }

            FormCard.FormCard {
                SettingChoice {
                    path: port.med + "role"
                    text: i18nc("@label:listbox", "Role")
                    options: root.roles
                }
                SettingChoice {
                    visible: port.medRole !== "none"
                    path: port.med + "source"
                    text: i18nc("@label:listbox", "Power source")
                    options: root.sources(port.medRole)
                }
                SettingChoice {
                    visible: port.medRole !== "none"
                    path: port.med + "priority"
                    text: i18nc("@label:listbox", "Priority")
                    options: root.priorities
                }
                SettingNumber {
                    visible: port.medRole !== "none"
                    path: port.med + "value"
                    label: i18nc("@label:spinbox", "Power")
                    from: 0
                    to: 102300
                    stepSize: 100
                    textFromValue: (value, locale) => root.watts(value)
                    valueFromText: (text, locale) => Math.round(parseFloat(text.replace(",", ".")) * 1000)
                }
            }

            FormCard.FormHeader {
                title: i18nc("@title:group %1 is a network interface", "%1: 802.3 Power", port.modelData)
            }

            FormCard.FormCard {
                SettingChoice {
                    path: port.dot3 + "role"
                    text: i18nc("@label:listbox", "Role")
                    options: root.roles
                }
                SettingSwitch {
                    visible: port.dot3Role !== "none"
                    path: port.dot3 + "supported"
                    text: i18nc("@option:check", "Power over the cable supported")
                }
                SettingSwitch {
                    visible: port.dot3Role !== "none"
                    path: port.dot3 + "enabled"
                    text: i18nc("@option:check", "Power over the cable enabled")
                }
                SettingSwitch {
                    visible: port.dot3Role !== "none"
                    path: port.dot3 + "pairControl"
                    text: i18nc("@option:check", "Pair selection can be controlled")
                }
                SettingChoice {
                    visible: port.dot3Role !== "none"
                    path: port.dot3 + "pairs"
                    text: i18nc("@label:listbox", "Pairs")
                    options: [
                        {value: "signal", text: i18nc("@item PoE pairs", "Signal pairs")},
                        {value: "spare", text: i18nc("@item PoE pairs", "Spare pairs")}
                    ]
                }
                SettingChoice {
                    visible: port.dot3Role !== "none"
                    path: port.dot3 + "powerClass"
                    text: i18nc("@label:listbox", "Class")
                    options: [
                        {value: 0, text: i18nc("@item PoE class", "Not sent")},
                        {value: 1, text: i18nc("@item PoE class", "Class 0, up to 13 W")},
                        {value: 2, text: i18nc("@item PoE class", "Class 1, up to 3.8 W")},
                        {value: 3, text: i18nc("@item PoE class", "Class 2, up to 6.5 W")},
                        {value: 4, text: i18nc("@item PoE class", "Class 3, up to 13 W")},
                        {value: 5, text: i18nc("@item PoE class", "Class 4, up to 25.5 W (PoE+)")}
                    ]
                }
                SettingChoice {
                    visible: port.dot3Role !== "none"
                    path: port.dot3 + "type"
                    text: i18nc("@label:listbox", "802.3at power")
                    options: [
                        {value: 0, text: i18nc("@item PoE type", "Not sent")},
                        {value: 1, text: i18nc("@item PoE type", "Type 1")},
                        {value: 2, text: i18nc("@item PoE type", "Type 2")}
                    ]
                }
                SettingChoice {
                    visible: port.dot3Role !== "none" && port.dot3Type > 0
                    path: port.dot3 + "source"
                    text: i18nc("@label:listbox", "Power source")
                    options: root.sources(port.dot3Role)
                }
                SettingChoice {
                    visible: port.dot3Role !== "none" && port.dot3Type > 0
                    path: port.dot3 + "priority"
                    text: i18nc("@label:listbox", "Priority")
                    options: root.priorities
                }
                SettingNumber {
                    visible: port.dot3Role !== "none" && port.dot3Type > 0
                    path: port.dot3 + "requested"
                    label: i18nc("@label:spinbox", "Requested")
                    from: 0
                    to: 102300
                    stepSize: 100
                    textFromValue: (value, locale) => root.watts(value)
                    valueFromText: (text, locale) => Math.round(parseFloat(text.replace(",", ".")) * 1000)
                }
                SettingNumber {
                    visible: port.dot3Role !== "none" && port.dot3Type > 0
                    path: port.dot3 + "allocated"
                    label: i18nc("@label:spinbox", "Allocated")
                    from: 0
                    to: 102300
                    stepSize: 100
                    textFromValue: (value, locale) => root.watts(value)
                    valueFromText: (text, locale) => Math.round(parseFloat(text.replace(",", ".")) * 1000)
                }
            }
        }
    }
}
