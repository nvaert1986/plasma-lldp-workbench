// SPDX-FileCopyrightText: 2026 plasma-lldp-workbench contributors
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigamiaddons.formcard as FormCard

// One of a list of {value, text} for a value of the settings tree
FormCard.FormComboBoxDelegate {
    id: root

    required property string path
    property var options: []

    function sync() {
        currentIndex = indexOfValue(editor.value(path));
    }

    function disableWheel(item) {
        for (let i = 0; i < item.children.length; ++i) {
            const child = item.children[i];
            if (child instanceof QQC2.ComboBox) {
                child.wheelEnabled = false;
            }
            disableWheel(child);
        }
    }

    model: options
    textRole: "text"
    valueRole: "value"

    Connections {
        target: editor

        function onChanged() {
            root.sync();
        }
    }

    // Scrolling the page must never change a setting; this caused an interface to be
    // switched off by accident once
    Component.onCompleted: {
        disableWheel(root);
        sync();
    }
    onOptionsChanged: sync()
    onActivated: editor.set(path, currentValue)
}
