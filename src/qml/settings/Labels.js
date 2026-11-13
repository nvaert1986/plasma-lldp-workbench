// SPDX-FileCopyrightText: 2026 plasma-lldp-workbench contributors
// SPDX-License-Identifier: GPL-3.0-or-later

.pragma library

// Readable names for lldpcli's keywords
function application(keyword, i18nc) {
    switch (keyword) {
    case "voice":
        return i18nc("@item LLDP-MED application", "Voice");
    case "voice-signaling":
        return i18nc("@item LLDP-MED application", "Voice signaling");
    case "guest-voice":
        return i18nc("@item LLDP-MED application", "Guest voice");
    case "guest-voice-signaling":
        return i18nc("@item LLDP-MED application", "Guest voice signaling");
    case "softphone-voice":
        return i18nc("@item LLDP-MED application", "Softphone voice");
    case "video-conferencing":
        return i18nc("@item LLDP-MED application", "Video conferencing");
    case "streaming-video":
        return i18nc("@item LLDP-MED application", "Streaming video");
    case "video-signaling":
        return i18nc("@item LLDP-MED application", "Video signaling");
    }
    return keyword;
}
