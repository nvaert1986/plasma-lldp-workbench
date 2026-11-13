// SPDX-FileCopyrightText: 2026 plasma-lldp-workbench contributors
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.settings as Settings
import org.kde.kirigamiaddons.statefulapp as StatefulApp

StatefulApp.StatefulWindow {
    id: window

    // Remembers the window size between runs
    windowName: "Main"
    application: app

    title: i18nc("@title:window", "LLDP Workbench")
    width: Kirigami.Units.gridUnit * 42
    height: Kirigami.Units.gridUnit * 36
    minimumWidth: Kirigami.Units.gridUnit * 22
    minimumHeight: Kirigami.Units.gridUnit * 16
    visible: !startHidden

    // The tray icon keeps running; Quit in the File menu or the tray menu ends the app
    onClosing: close => {
        close.accepted = false;
        window.hide();
    }

    // A window of its own with a page per topic, like KDE's other settings windows
    function openSettings(module) {
        // Unapplied edits survive closing and reopening the window
        if (!editor.dirty) {
            editor.reload();
        }
        settingsView.open(module);
    }

    menuBar: QQC2.MenuBar {
        QQC2.Menu {
            title: i18nc("@title:menu", "&File")

            Kirigami.Action {
                fromQAction: app.action("copy_all")
            }
            QQC2.MenuSeparator {}
            Kirigami.Action {
                fromQAction: app.action("file_close")
            }
            Kirigami.Action {
                fromQAction: app.action("file_quit")
            }
        }
        QQC2.Menu {
            title: i18nc("@title:menu", "&View")

            Kirigami.Action {
                fromQAction: app.action("view_redisplay")
            }
            QQC2.MenuSeparator {}
            Kirigami.Action {
                fromQAction: app.action("show_idle_interfaces")
            }
        }
        QQC2.Menu {
            title: i18nc("@title:menu", "&Settings")

            Kirigami.Action {
                fromQAction: app.action("options_configure")
            }
            Kirigami.Action {
                fromQAction: app.action("start_at_login")
            }
            QQC2.MenuSeparator {}
            Kirigami.Action {
                fromQAction: app.action("options_configure_keybinding")
            }
        }
        QQC2.Menu {
            title: i18nc("@title:menu", "&Help")

            Kirigami.Action {
                fromQAction: app.action("open_kcommand_bar")
            }
            QQC2.MenuSeparator {}
            Kirigami.Action {
                fromQAction: app.action("open_about_page")
            }
        }
    }

    Connections {
        target: app

        function onCopiedAll() {
            window.showPassiveNotification(i18nc("@info", "Copied to the clipboard"));
        }
        function onCloseWindowRequested() {
            window.hide();
        }
        function onConfigureSendingRequested() {
            window.openSettings("");
        }
    }

    pageStack.initialPage: NeighborsPage {}

    Settings.ConfigurationView {
        id: settingsView

        function page(name) {
            return () => Qt.createComponent("io.github.nvaert1986.lldpworkbench", name);
        }

        window: window
        title: i18nc("@title:window", "LLDP Settings")
        modules: [
            Settings.ConfigurationModule {
                moduleId: "profiles"
                text: i18nc("@title", "Profiles")
                icon.name: "bookmarks"
                page: settingsView.page("ProfilesPage")
            },
            Settings.ConfigurationModule {
                moduleId: "identity"
                text: i18nc("@title", "Identity")
                icon.name: "user-identity"
                page: settingsView.page("IdentityPage")
            },
            Settings.ConfigurationModule {
                moduleId: "interfaces"
                text: i18nc("@title", "Interfaces")
                icon.name: "network-wired"
                page: settingsView.page("InterfacesPage")
            },
            Settings.ConfigurationModule {
                moduleId: "timing"
                text: i18nc("@title", "Timing")
                icon.name: "chronometer"
                page: settingsView.page("TimingPage")
            },
            Settings.ConfigurationModule {
                moduleId: "med"
                text: i18nc("@title", "LLDP-MED")
                icon.name: "call-start"
                page: settingsView.page("MedPage")
            },
            Settings.ConfigurationModule {
                moduleId: "location"
                text: i18nc("@title", "Location")
                icon.name: "mark-location"
                page: settingsView.page("LocationPage")
            },
            Settings.ConfigurationModule {
                moduleId: "power"
                text: i18nc("@title", "Power over Ethernet")
                icon.name: "preferences-system-power"
                page: settingsView.page("PowerPage")
            },
            Settings.ConfigurationModule {
                moduleId: "customTlvs"
                text: i18nc("@title", "Custom TLVs")
                icon.name: "code-context"
                page: settingsView.page("CustomTlvPage")
            },
            Settings.ConfigurationModule {
                moduleId: "startup"
                text: i18nc("@title", "Startup Options")
                icon.name: "system-run"
                page: settingsView.page("StartupPage")
            },
            Settings.ConfigurationModule {
                moduleId: "save"
                text: i18nc("@title", "Save")
                icon.name: "document-save"
                page: settingsView.page("SavePage")
            }
        ]
    }

    Component.onCompleted: {
        if (startPage.length > 0) {
            openSettings(startPage);
        }
    }
}
