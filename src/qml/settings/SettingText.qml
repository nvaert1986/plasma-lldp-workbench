// SPDX-FileCopyrightText: 2026 plasma-lldp-workbench contributors
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import org.kde.kirigamiaddons.formcard as FormCard
import "Paths.js" as Paths

// A text value of the settings tree; an empty field means lldpd's own value
FormCard.FormTextFieldDelegate {
    id: root

    required property string path
    readonly property string stored: String(Paths.at(editor.edited, editor.daemon, root.path) ?? "")

    // Typing assigns text, which would end a plain binding: Binding keeps following editor
    Binding on text {
        value: root.stored
    }

    onTextChanged: {
        if (text !== stored) {
            editor.set(path, text);
        }
    }
}
