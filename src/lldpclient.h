// SPDX-FileCopyrightText: 2026 plasma-lldp-workbench contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "neighbors.h"

namespace LldpWorkbench
{
// Talks to lldpd over its control socket. Every call opens its own short connection: lldpd
// answers locally and quickly, and a connection that is never shared needs no locking.
namespace LldpClient
{
enum class State {
    Ok,
    NotRunning,
    NoAccess,
    Error,
};

struct Snapshot {
    State state = State::Error;
    QString error;
    QList<LocalPort> ports;
};

QString socketPath();
Snapshot read();
// Invented neighbors for screenshots and testing without lldpd
Snapshot demo();
}
}
