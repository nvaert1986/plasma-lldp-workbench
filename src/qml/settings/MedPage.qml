// SPDX-FileCopyrightText: 2026 plasma-lldp-workbench contributors
// SPDX-License-Identifier: GPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard
import "Paths.js" as Paths
import "Labels.js" as Labels

FormCard.FormCardPage {
    id: root

    title: i18nc("@title", "LLDP-MED")
    footer: PendingBar {}

    Kirigami.InlineMessage {
        Layout.fillWidth: true
        Layout.maximumWidth: Kirigami.Units.gridUnit * 30
        Layout.alignment: Qt.AlignHCenter
        Layout.topMargin: Kirigami.Units.largeSpacing
        visible: (editor.daemon.medClass ?? 0) === 0
        type: Kirigami.MessageType.Information
        text: i18nc("@info", "LLDP-MED is off. Policies, location, power and inventory are only sent once a device class is chosen.")
    }

    FormCard.FormHeader {
        title: i18nc("@title:group", "Device Class")
    }

    FormCard.FormCard {
        SettingChoice {
            path: "daemon/medClass"
            text: i18nc("@label:listbox", "Act as")
            description: i18nc("@info", "lldpd only takes this when it starts, so it restarts, which asks for your password.")
            options: [
                {value: 0, text: i18nc("@item LLDP-MED class", "No LLDP-MED")},
                {value: 1, text: i18nc("@item LLDP-MED class", "Class I, generic endpoint")},
                {value: 2, text: i18nc("@item LLDP-MED class", "Class II, media endpoint")},
                {value: 3, text: i18nc("@item LLDP-MED class", "Class III, communication device such as an IP phone")},
                {value: 4, text: i18nc("@item LLDP-MED class", "Network connectivity device")}
            ]
        }
    }

    FormCard.FormHeader {
        title: i18nc("@title:group", "Inventory")
    }

    FormCard.FormCard {
        SettingSwitch {
            path: "daemon/noInventory"
            text: i18nc("@option:check", "Do not send the inventory")
        }
        Repeater {
            model: [
                {field: "manufacturer", label: i18nc("@label:textbox", "Manufacturer")},
                {field: "model", label: i18nc("@label:textbox", "Model")},
                {field: "serial-number", label: i18nc("@label:textbox", "Serial number")},
                {field: "asset", label: i18nc("@label:textbox", "Asset ID")},
                {field: "hardware-revision", label: i18nc("@label:textbox", "Hardware revision")},
                {field: "firmware-revision", label: i18nc("@label:textbox", "Firmware revision")},
                {field: "software-revision", label: i18nc("@label:textbox", "Software revision")}
            ]

            delegate: SettingText {
                required property var modelData

                path: "global/inventory/" + modelData.field
                label: modelData.label
            }
        }
    }

    Repeater {
        model: editor.portNames

        delegate: ColumnLayout {
            id: port

            required property string modelData
            readonly property string base: "ports/" + modelData + "/policies"
            // A string, so typing a value does not rebuild the list and lose focus
            readonly property string applications: Object.keys(Paths.at(editor.edited, editor.daemon, port.base) ?? {}).join(",")
            readonly property var missing: editor.applications.filter(application => port.applications.split(",").indexOf(application) < 0)

            Layout.fillWidth: true
            spacing: 0

            FormCard.FormHeader {
                title: i18nc("@title:group %1 is a network interface", "%1: Voice and Video Policies", port.modelData)
            }

            FormCard.FormCard {
                FormCard.FormTextDelegate {
                    visible: port.applications.length === 0
                    text: i18nc("@info", "No policies")
                    description: i18nc("@info", "An endpoint lists the applications it needs, and the switch answers with their VLAN and markings.")
                }

                Repeater {
                    model: port.applications.length > 0 ? port.applications.split(",") : []

                    delegate: ColumnLayout {
                        id: policy

                        required property string modelData
                        required property int index
                        readonly property string path: port.base + "/" + modelData + "/"

                        Layout.fillWidth: true
                        spacing: 0

                        FormCard.FormDelegateSeparator {
                            visible: policy.index > 0
                        }
                        FormCard.FormTextDelegate {
                            text: Labels.application(policy.modelData, i18nc)
                            textItem.font.bold: true
                            trailing: QQC2.ToolButton {
                                icon.name: "list-remove"
                                text: i18nc("@action:button", "Remove")
                                display: QQC2.AbstractButton.IconOnly
                                QQC2.ToolTip.text: text
                                QQC2.ToolTip.visible: hovered
                                onClicked: editor.removePolicy(port.modelData, policy.modelData)
                            }
                        }
                        SettingSwitch {
                            path: policy.path + "unknown"
                            text: i18nc("@option:check", "Ask the switch")
                            description: i18nc("@info", "The VLAN is not known yet; the switch should send it. This is what a phone does when it starts.")
                        }
                        SettingSwitch {
                            path: policy.path + "tagged"
                            text: i18nc("@option:check", "Tagged")
                        }
                        SettingNumber {
                            path: policy.path + "vlan"
                            label: i18nc("@label:spinbox", "VLAN")
                            from: 0
                            to: 4094
                            textFromValue: (value, locale) => value === 0 ? i18nc("@item VLAN", "None") : String(value)
                            valueFromText: (text, locale) => parseInt(text) || 0
                        }
                        SettingNumber {
                            path: policy.path + "priority"
                            label: i18nc("@label:spinbox", "Priority (802.1p)")
                            from: 0
                            to: 7
                        }
                        SettingNumber {
                            path: policy.path + "dscp"
                            label: i18nc("@label:spinbox", "DSCP")
                            from: 0
                            to: 63
                        }
                    }
                }

                FormCard.FormDelegateSeparator {}

                FormCard.AbstractFormDelegate {
                    visible: port.missing.length > 0
                    background: null
                    contentItem: RowLayout {
                        QQC2.ComboBox {
                            id: newApplication

                            Layout.fillWidth: true
                            wheelEnabled: false
                            model: port.missing.map(application => ({value: application, text: Labels.application(application, i18nc)}))
                            textRole: "text"
                            valueRole: "value"
                        }
                        QQC2.Button {
                            icon.name: "list-add"
                            text: i18nc("@action:button", "Add Policy")
                            onClicked: editor.addPolicy(port.modelData, newApplication.currentValue)
                        }
                    }
                }
            }
        }
    }
}
