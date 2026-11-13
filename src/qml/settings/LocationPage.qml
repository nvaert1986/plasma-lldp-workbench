// SPDX-FileCopyrightText: 2026 plasma-lldp-workbench contributors
// SPDX-License-Identifier: GPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard
import "Paths.js" as Paths

FormCard.FormCardPage {
    id: root

    title: i18nc("@title", "Location")
    footer: PendingBar {}

    FormCard.FormSectionText {
        text: i18nc("@info", "LLDP-MED location, used for emergency calls. Needs an LLDP-MED device class.")
    }

    Repeater {
        model: editor.portNames

        delegate: ColumnLayout {
            id: port

            required property string modelData
            readonly property string base: "ports/" + modelData + "/locations/"
            readonly property int elementCount: (Paths.at(editor.edited, editor.daemon, port.base + "civic/elements") ?? []).length

            Layout.fillWidth: true
            spacing: 0

            FormCard.FormHeader {
                title: port.modelData
            }

            FormCard.FormCard {
                SettingSwitch {
                    id: coordinate

                    path: port.base + "coordinate/enabled"
                    text: i18nc("@option:check", "Coordinates")
                }
                SettingText {
                    visible: coordinate.checked
                    path: port.base + "coordinate/latitude"
                    label: i18nc("@label:textbox", "Latitude")
                    placeholderText: i18nc("@info:placeholder", "52.0907N")
                }
                SettingText {
                    visible: coordinate.checked
                    path: port.base + "coordinate/longitude"
                    label: i18nc("@label:textbox", "Longitude")
                    placeholderText: i18nc("@info:placeholder", "5.1214E")
                }
                SettingText {
                    visible: coordinate.checked
                    path: port.base + "coordinate/altitude"
                    label: i18nc("@label:textbox", "Altitude")
                }
                SettingChoice {
                    visible: coordinate.checked
                    path: port.base + "coordinate/altitudeUnit"
                    text: i18nc("@label:listbox", "Altitude in")
                    options: [
                        {value: "m", text: i18nc("@item altitude unit", "Meters")},
                        {value: "f", text: i18nc("@item altitude unit", "Floors")}
                    ]
                }
                SettingChoice {
                    visible: coordinate.checked
                    path: port.base + "coordinate/datum"
                    text: i18nc("@label:listbox", "Datum")
                    options: [
                        {value: "WGS84", text: "WGS84"},
                        {value: "NAD83", text: "NAD83"},
                        {value: "NAD83/MLLW", text: "NAD83/MLLW"}
                    ]
                }

                FormCard.FormDelegateSeparator {}

                SettingSwitch {
                    id: civic

                    path: port.base + "civic/enabled"
                    text: i18nc("@option:check", "Civic address")
                }
                SettingText {
                    visible: civic.checked
                    path: port.base + "civic/country"
                    label: i18nc("@label:textbox", "Country code")
                    placeholderText: "NL"
                    maximumLength: 2
                }
                Repeater {
                    model: civic.checked ? port.elementCount : 0

                    delegate: RowLayout {
                        id: element

                        required property int index
                        readonly property string path: port.base + "civic/elements/" + index + "/"

                        Layout.fillWidth: true
                        spacing: 0

                        SettingChoice {
                            Layout.preferredWidth: Kirigami.Units.gridUnit * 10
                            path: element.path + "type"
                            text: i18nc("@label:listbox", "Part")
                            options: editor.civicTypes.map(type => ({value: type, text: type}))
                        }
                        SettingText {
                            Layout.fillWidth: true
                            path: element.path + "value"
                            label: i18nc("@label:textbox", "Value")
                        }
                        QQC2.ToolButton {
                            icon.name: "list-remove"
                            text: i18nc("@action:button", "Remove")
                            display: QQC2.AbstractButton.IconOnly
                            onClicked: editor.removeListEntry(port.base + "civic/elements", element.index)
                        }
                    }
                }
                FormCard.FormButtonDelegate {
                    visible: civic.checked
                    icon.name: "list-add"
                    text: i18nc("@action:button", "Add Address Part")
                    onClicked: editor.addListEntry(port.base + "civic/elements")
                }

                FormCard.FormDelegateSeparator {}

                SettingSwitch {
                    id: elin

                    path: port.base + "elin/enabled"
                    text: i18nc("@option:check", "Emergency location number (ELIN)")
                }
                SettingText {
                    visible: elin.checked
                    path: port.base + "elin/number"
                    label: i18nc("@label:textbox", "Number")
                }
            }
        }
    }
}
