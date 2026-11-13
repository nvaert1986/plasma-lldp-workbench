// SPDX-FileCopyrightText: 2026 plasma-lldp-workbench contributors
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import org.kde.kirigamiaddons.formcard as FormCard

FormCard.FormCardPage {
    id: root

    title: i18nc("@title", "Timing")
    footer: PendingBar {}

    FormCard.FormHeader {
        title: i18nc("@title:group", "Sending")
    }

    FormCard.FormCard {
        SettingNumber {
            path: "global/txInterval"
            factor: 1000
            label: i18nc("@label:spinbox", "Send every")
            from: 1
            to: 3600
            textFromValue: (value, locale) => i18ncp("@item:valuesuffix", "%1 second", "%1 seconds", value)
            valueFromText: (text, locale) => parseInt(text)
        }
        SettingNumber {
            path: "global/txHold"
            label: i18nc("@label:spinbox", "Hold multiplier")
            description: i18nc("@info", "Neighbors keep this computer for the interval times this number.")
            from: 1
            to: 100
        }
        SettingSwitch {
            path: "global/fastStart"
            text: i18nc("@option:check", "Fast start")
            description: i18nc("@info", "Send faster for the first frames after a new neighbor appears, as LLDP-MED devices do.")
        }
        SettingNumber {
            path: "global/fastStartInterval"
            label: i18nc("@label:spinbox", "Fast start interval")
            from: 1
            to: 3600
            textFromValue: (value, locale) => i18ncp("@item:valuesuffix", "%1 second", "%1 seconds", value)
            valueFromText: (text, locale) => parseInt(text)
        }
        SettingChoice {
            path: "global/agentType"
            text: i18nc("@label:listbox", "Destination")
            description: i18nc("@info", "The 802.1AB-2009 destination addresses. Switches listen on the nearest bridge address unless they are configured otherwise.")
            options: [
                {value: "nearest-bridge", text: i18nc("@item LLDP destination", "Nearest bridge (01:80:C2:00:00:0E)")},
                {value: "nearest-non-tpmr-bridge", text: i18nc("@item LLDP destination", "Nearest non-TPMR bridge (01:80:C2:00:00:03)")},
                {value: "nearest-customer-bridge", text: i18nc("@item LLDP destination", "Nearest customer bridge (01:80:C2:00:00:00)")}
            ]
        }
    }

    FormCard.FormHeader {
        title: i18nc("@title:group", "Receiving")
    }

    FormCard.FormCard {
        SettingNumber {
            path: "global/maxNeighbors"
            label: i18nc("@label:spinbox", "Neighbors per interface")
            description: i18nc("@info", "Applies to neighbors seen from now on.")
            from: 1
            to: 1000
        }
    }
}
