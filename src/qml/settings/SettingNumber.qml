// SPDX-FileCopyrightText: 2026 plasma-lldp-workbench contributors
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigamiaddons.formcard as FormCard
import "Paths.js" as Paths

FormCard.FormSpinBoxDelegate {
    id: root

    required property string path
    // The stored value is this many times the shown one, such as milliseconds shown as seconds
    property int factor: 1
    readonly property int stored: Math.round(Number(Paths.at(editor.edited, editor.daemon, root.path) ?? 0) / root.factor)

    function disableWheel(item) {
        for (let i = 0; i < item.children.length; ++i) {
            const child = item.children[i];
            if (child instanceof QQC2.SpinBox) {
                child.wheelEnabled = false;
            }
            disableWheel(child);
        }
    }

    Binding on value {
        value: root.stored
    }

    // Scrolling the page must never change a setting
    Component.onCompleted: disableWheel(root)
    onValueModified: editor.set(path, value * factor)
}
