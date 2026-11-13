// SPDX-FileCopyrightText: 2026 plasma-lldp-workbench contributors
// SPDX-License-Identifier: GPL-3.0-or-later

.pragma library

// A value of the settings tree. Taking edited and daemon as arguments makes a binding follow
// them; "daemon/<key>" reaches the startup options.
function at(edited, daemon, path) {
    if (path.startsWith("daemon/")) {
        return daemon[path.slice(7)];
    }
    let node = edited;
    for (const part of path.split("/")) {
        if (node === undefined || node === null) {
            return undefined;
        }
        node = node[part];
    }
    return node;
}
