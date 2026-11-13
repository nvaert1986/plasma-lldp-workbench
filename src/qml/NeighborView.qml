// SPDX-FileCopyrightText: 2026 plasma-lldp-workbench contributors
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import org.kde.kirigamiaddons.formcard as FormCard

ColumnLayout {
    id: view

    required property var modelData
    required property int index

    spacing: 0

    component Field: FormCard.FormTextDelegate {
        // Fields a neighbor did not send stay hidden
        visible: description.length > 0
        Layout.fillWidth: true
    }

    // Between two neighbors on the same interface
    FormCard.FormDelegateSeparator {
        visible: view.index > 0
    }

    FormCard.FormTextDelegate {
        Layout.fillWidth: true
        icon.name: "network-rj45-female"
        text: view.modelData.summary
        textItem.font.bold: true
        description: view.modelData.systemDescription
        descriptionItem.wrapMode: Text.Wrap
    }

    FormCard.FormDelegateSeparator {}

    Field {
        text: i18nc("@label", "Port description")
        description: view.modelData.portDescription
    }
    Field {
        text: i18nc("@label", "VLAN")
        description: view.modelData.vlans
    }
    Field {
        text: i18nc("@label", "Management address")
        description: view.modelData.managementAddresses
    }
    Field {
        text: i18nc("@label", "Capabilities")
        description: view.modelData.capabilities
    }
    Field {
        text: i18nc("@label", "Link")
        description: view.modelData.link
    }
    Field {
        text: i18nc("@label", "Power over Ethernet")
        description: view.modelData.power
    }
    Field {
        text: i18nc("@label LLDP-MED device class", "LLDP-MED class")
        description: view.modelData.medClass
    }
    Field {
        text: i18nc("@label LLDP-MED network policies", "Voice and video policy")
        description: view.modelData.medPolicies
    }
    Field {
        text: i18nc("@label", "Chassis ID")
        description: view.modelData.chassisId
    }
    Field {
        text: i18nc("@label", "Protocol")
        description: view.modelData.ttl > 0 ? i18nc("@info protocol and time to live", "%1, valid for %2 seconds", view.modelData.protocol, view.modelData.ttl)
                                           : view.modelData.protocol
    }
}
