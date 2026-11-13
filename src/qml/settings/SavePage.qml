// SPDX-FileCopyrightText: 2026 plasma-lldp-workbench contributors
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import org.kde.kirigamiaddons.formcard as FormCard

FormCard.FormCardPage {
    id: root

    title: i18nc("@title", "Save")
    footer: PendingBar {}

    FormCard.FormHeader {
        title: i18nc("@title:group", "Keep After a Restart")
    }

    FormCard.FormCard {
        FormCard.FormTextDelegate {
            text: i18nc("@info", "Applied settings last until lldpd restarts")
            description: i18nc("@info", "Saving writes the settings you applied in this app to /etc/lldpd.d/plasma-lldp-workbench.conf, which lldpd reads when it starts. This asks for your password.")
            descriptionItem.wrapMode: Text.Wrap
        }

        FormCard.FormDelegateSeparator {}

        FormCard.FormTextAreaDelegate {
            label: i18nc("@label:textbox", "Contents")
            text: editor.savedConfig
            readOnly: true
        }

        FormCard.FormDelegateSeparator {}

        FormCard.FormButtonDelegate {
            icon.name: "document-save"
            text: i18nc("@action:button", "Save Permanently")
            enabled: !editor.busy
            onClicked: editor.savePermanently()
        }
        FormCard.FormButtonDelegate {
            icon.name: "edit-copy"
            text: i18nc("@action:button", "Copy")
            onClicked: backend.copyText(editor.savedConfig)
        }
    }
}
