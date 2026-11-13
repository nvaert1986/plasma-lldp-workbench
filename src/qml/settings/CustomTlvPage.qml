// SPDX-FileCopyrightText: 2026 plasma-lldp-workbench contributors
// SPDX-License-Identifier: GPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigamiaddons.formcard as FormCard
import "Paths.js" as Paths

FormCard.FormCardPage {
    id: root

    title: i18nc("@title", "Custom TLVs")
    footer: PendingBar {}

    FormCard.FormSectionText {
        text: i18nc("@info", "Organizationally specific TLVs, sent as they are. Bytes are written in hexadecimal and separated by commas, such as 00,12,0F.")
    }

    Repeater {
        model: editor.portNames

        delegate: ColumnLayout {
            id: port

            required property string modelData
            readonly property string base: "ports/" + modelData + "/customTlvs"
            readonly property int count: (Paths.at(editor.edited, editor.daemon, port.base) ?? []).length

            Layout.fillWidth: true
            spacing: 0

            FormCard.FormHeader {
                title: port.modelData
            }

            FormCard.FormCard {
                Repeater {
                    model: port.count

                    delegate: ColumnLayout {
                        id: tlv

                        required property int index
                        readonly property string path: port.base + "/" + index + "/"

                        Layout.fillWidth: true
                        spacing: 0

                        FormCard.FormDelegateSeparator {
                            visible: tlv.index > 0
                        }
                        FormCard.FormTextDelegate {
                            text: i18nc("@title:group", "TLV %1", tlv.index + 1)
                            textItem.font.bold: true
                            trailing: QQC2.ToolButton {
                                icon.name: "list-remove"
                                text: i18nc("@action:button", "Remove")
                                display: QQC2.AbstractButton.IconOnly
                                QQC2.ToolTip.text: text
                                QQC2.ToolTip.visible: hovered
                                onClicked: editor.removeListEntry(port.base, tlv.index)
                            }
                        }
                        SettingText {
                            path: tlv.path + "oui"
                            label: i18nc("@label:textbox", "Organization (OUI, 3 bytes)")
                            validator: RegularExpressionValidator {
                                regularExpression: /[0-9A-Fa-f]{2},[0-9A-Fa-f]{2},[0-9A-Fa-f]{2}/
                            }
                        }
                        SettingNumber {
                            path: tlv.path + "subtype"
                            label: i18nc("@label:spinbox", "Subtype")
                            from: 0
                            to: 255
                        }
                        SettingText {
                            path: tlv.path + "info"
                            label: i18nc("@label:textbox", "Content (bytes)")
                            placeholderText: i18nc("@info:placeholder", "Empty")
                            validator: RegularExpressionValidator {
                                regularExpression: /([0-9A-Fa-f]{2}(,[0-9A-Fa-f]{2})*)?/
                            }
                        }
                    }
                }

                FormCard.FormButtonDelegate {
                    icon.name: "list-add"
                    text: i18nc("@action:button", "Add TLV")
                    onClicked: editor.addListEntry(port.base)
                }
            }
        }
    }
}
