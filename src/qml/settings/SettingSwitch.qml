// SPDX-FileCopyrightText: 2026 plasma-lldp-workbench contributors
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import org.kde.kirigamiaddons.formcard as FormCard
import "Paths.js" as Paths

FormCard.FormSwitchDelegate {
    id: root

    required property string path
    // lldpd keeps some switches as 0 and 1
    property bool numeric: false

    Binding on checked {
        value: !!Paths.at(editor.edited, editor.daemon, root.path)
    }

    onToggled: editor.set(path, numeric ? (checked ? 1 : 0) : checked)
}
