// SPDX-FileCopyrightText: 2026 plasma-lldp-workbench contributors
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

FormCard.FormCardPage {
    id: page

    title: i18nc("@title", "Neighbors")

    actions: [
        Kirigami.Action {
            fromQAction: app.action("copy_all")
        },
        Kirigami.Action {
            fromQAction: app.action("view_redisplay")
        },
        Kirigami.Action {
            fromQAction: app.action("options_configure")
        }
    ]

    Repeater {
        id: interfaces

        model: backend.state === Backend.Ok ? backend.interfaces : null

        delegate: InterfaceCard {
            Layout.fillWidth: true
        }
    }

    Kirigami.PlaceholderMessage {
        Layout.fillWidth: true
        Layout.topMargin: Kirigami.Units.gridUnit * 6
        Layout.leftMargin: Kirigami.Units.gridUnit * 2
        Layout.rightMargin: Kirigami.Units.gridUnit * 2
        visible: backend.state !== Backend.Ok || interfaces.count === 0

        icon.name: {
            if (backend.state === Backend.NoAccess) {
                return "object-locked";
            }
            return backend.state === Backend.Ok ? "network-rj45-female" : "dialog-warning";
        }
        text: {
            switch (backend.state) {
            case Backend.NotRunning:
                return i18nc("@info", "lldpd is not running");
            case Backend.NoAccess:
                return i18nc("@info", "No permission to talk to lldpd");
            case Backend.Failed:
                return i18nc("@info", "Could not read from lldpd");
            }
            // Interfaces exist, but View hides the ones without a neighbor
            return backend.ports.length > 0 ? i18nc("@info", "No LLDP neighbor heard yet") : i18nc("@info", "lldpd uses no network interfaces");
        }
        explanation: {
            switch (backend.state) {
            case Backend.NotRunning:
                return i18nc("@info", "Start the lldpd service as root, with “systemctl enable --now lldpd” on systemd or “rc-update add lldpd default” and “rc-service lldpd start” on OpenRC.");
            case Backend.NoAccess:
                return i18nc("@info %1 is a user name", "Your user must be in the lldpd group. Run “gpasswd -a %1 lldpd” as root, then log out and back in.", backend.userName);
            case Backend.Failed:
                return backend.errorText;
            }
            return backend.ports.length > 0 ? i18nc("@info", "Switches usually announce themselves every 30 seconds. LLDP may also be switched off on the switch.")
                                            : i18nc("@info", "lldpd only listens on wired interfaces that are up. Connect a network cable or check lldpd's interface settings.");
        }
        helpfulAction: backend.state === Backend.NoAccess ? copyCommand : null

        Kirigami.Action {
            id: copyCommand

            icon.name: "edit-copy"
            text: i18nc("@action:button", "Copy Command")
            onTriggered: backend.copyText("gpasswd -a " + backend.userName + " lldpd")
        }
    }
}
