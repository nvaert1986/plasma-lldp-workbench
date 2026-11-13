// SPDX-FileCopyrightText: 2026 plasma-lldp-workbench contributors
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

// What Apply will do, and the result of the last Apply
ColumnLayout {
    id: bar

    property string result
    property bool resultSuccess: true

    spacing: 0
    visible: editor.dirty || editor.busy || result.length > 0

    Connections {
        target: editor

        function onFinished(success, message) {
            bar.resultSuccess = success;
            bar.result = message;
            hideResult.restart();
        }
    }

    Timer {
        id: hideResult

        interval: 8000
        onTriggered: bar.result = ""
    }

    Kirigami.Separator {
        Layout.fillWidth: true
    }

    Kirigami.InlineMessage {
        Layout.fillWidth: true
        Layout.margins: Kirigami.Units.smallSpacing
        visible: bar.result.length > 0 && !editor.dirty
        position: Kirigami.InlineMessage.Footer
        type: bar.resultSuccess ? Kirigami.MessageType.Positive : Kirigami.MessageType.Error
        text: bar.result
    }

    Kirigami.InlineMessage {
        id: message

        Layout.fillWidth: true
        Layout.margins: Kirigami.Units.smallSpacing
        visible: editor.dirty || editor.busy
        position: Kirigami.InlineMessage.Footer
        type: editor.needsRestart ? Kirigami.MessageType.Warning : Kirigami.MessageType.Information
        text: {
            if (editor.busy) {
                return i18nc("@info", "Applying…");
            }
            const count = i18ncp("@info", "%1 change waiting to be applied.", "%1 changes waiting to be applied.", editor.pending.length);
            return editor.needsRestart ? count + " " + i18nc("@info", "lldpd restarts for some of them, which asks for your password.") : count;
        }
        actions: [
            Kirigami.Action {
                icon.name: "view-list-details"
                text: i18nc("@action:button", "Show Changes")
                checkable: true
                checked: details.visible
                onToggled: details.visible = checked
            },
            Kirigami.Action {
                icon.name: "edit-undo"
                text: i18nc("@action:button", "Discard")
                enabled: !editor.busy
                onTriggered: editor.discard()
            },
            Kirigami.Action {
                icon.name: "dialog-ok-apply"
                text: i18nc("@action:button", "Apply")
                enabled: !editor.busy
                onTriggered: editor.apply()
            }
        ]
    }

    QQC2.ScrollView {
        id: details

        Layout.fillWidth: true
        Layout.maximumHeight: Kirigami.Units.gridUnit * 8
        Layout.leftMargin: Kirigami.Units.largeSpacing
        Layout.rightMargin: Kirigami.Units.largeSpacing
        Layout.bottomMargin: Kirigami.Units.smallSpacing
        visible: false

        Kirigami.SelectableLabel {
            text: editor.pending.join("\n")
            font.family: "monospace"
            wrapMode: Text.NoWrap
        }
    }
}
