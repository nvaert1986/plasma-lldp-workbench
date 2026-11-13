// SPDX-FileCopyrightText: 2026 plasma-lldp-workbench contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QList>
#include <QString>
#include <QStringList>
#include <QVariantMap>

namespace LldpWorkbench
{
// What a switch (or any other LLDP speaker) announced on one of our interfaces
struct Neighbor {
    QString protocol;
    QString systemName;
    QString systemDescription;
    QString chassisId;
    QString chassisIdType;
    QString portId;
    QString portIdType;
    QString portDescription;
    QStringList managementAddresses;
    int capabilities = 0;
    int pvid = 0;
    QStringList vlans;
    int ttl = 0;
    QString link;
    QString medClass;
    QStringList medPolicies;
    QString power;

    bool operator==(const Neighbor &) const = default;
};

// One interface lldpd manages, with what it does there and who it hears
struct LocalPort {
    QString interfaceName;
    int status = 0;
    QString portId;
    QString portDescription;
    QList<Neighbor> neighbors;

    bool operator==(const LocalPort &) const = default;
};

// Values of lldpd's port status map (lldpctl_k_port_status)
enum PortStatus {
    TxOnly = 1,
    RxOnly = 2,
    Disabled = 3,
    RxAndTx = 4,
};

QStringList capabilityNames(int mask);
QString portStatusText(int status);

// "SW01 · Gi1/0/12 · VLAN 10": the switch, its port and the untagged VLAN
QString summary(const Neighbor &neighbor);
QString summary(const LocalPort &port);

// Plain text for a ticket or a chat
QString clipboardText(const LocalPort &port);

QVariantMap toVariant(const Neighbor &neighbor);
}
