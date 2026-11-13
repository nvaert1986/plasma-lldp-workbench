// SPDX-FileCopyrightText: 2026 plasma-lldp-workbench contributors
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

ColumnLayout {
    id: card

    required property string name
    required property string statusText
    required property var neighbors
    required property int neighborCount

    spacing: 0

    FormCard.FormHeader {
        title: i18nc("@title:group %1 is a network interface, %2 what LLDP does there", "%1 · %2", card.name, card.statusText)
        actions: [
            Kirigami.Action {
                icon.name: "edit-copy"
                text: i18nc("@action:button", "Copy")
                tooltip: i18nc("@info:tooltip", "Copy this interface's neighbors as text")
                onTriggered: {
                    backend.copyInterface(card.name);
                    applicationWindow().showPassiveNotification(i18nc("@info", "Copied to the clipboard"));
                }
            }
        ]
    }

    FormCard.FormCard {
        FormCard.FormTextDelegate {
            visible: card.neighborCount === 0
            icon.name: "network-disconnect"
            text: i18nc("@info", "No LLDP neighbor heard yet")
            description: i18nc("@info", "Switches usually announce themselves every 30 seconds. LLDP may also be switched off on the switch.")
        }

        Repeater {
            model: card.neighbors

            delegate: NeighborView {
                Layout.fillWidth: true
            }
        }
    }
}
