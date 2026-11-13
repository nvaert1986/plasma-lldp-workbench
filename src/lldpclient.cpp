// SPDX-FileCopyrightText: 2026 plasma-lldp-workbench contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "lldpclient.h"

#include "lldpatom.h"

#include <KLocalizedString>

#include <QFileInfo>

#include <unistd.h>

using namespace Qt::StringLiterals;

namespace LldpWorkbench::LldpClient
{
namespace
{
using namespace Atoms;

// Checked before connecting, because lldpctl reports a refused socket and a missing daemon
// with the same error
State socketState()
{
    const QString path = socketPath();
    if (!QFileInfo::exists(path)) {
        return State::NotRunning;
    }
    if (::access(QFile::encodeName(path).constData(), R_OK | W_OK) != 0) {
        return State::NoAccess;
    }
    return State::Ok;
}

QString socketError(State state)
{
    switch (state) {
    case State::NotRunning:
        return i18nc("@info", "lldpd is not running.");
    case State::NoAccess:
        return i18nc("@info", "No permission to talk to lldpd.");
    default:
        return {};
    }
}

QStringList medPolicies(lldpctl_atom_t *neighbor)
{
    QStringList policies;
    Atom list(lldpctl_atom_get(neighbor, lldpctl_k_port_med_policies));
    if (!list) {
        return policies;
    }
    lldpctl_atom_t *policy = nullptr;
    lldpctl_atom_foreach(list.get(), policy)
    {
        if (lldpctl_atom_get_int(policy, lldpctl_k_med_policy_type) <= 0) {
            continue;
        }
        const QString application = text(policy, lldpctl_k_med_policy_type);
        if (lldpctl_atom_get_int(policy, lldpctl_k_med_policy_unknown) > 0) {
            policies << i18nc("@info %1 is an application such as Voice", "%1: not defined", application);
            continue;
        }
        const int vlan = number(policy, lldpctl_k_med_policy_vid);
        const bool tagged = lldpctl_atom_get_int(policy, lldpctl_k_med_policy_tagged) > 0;
        policies << i18nc("@info %1 application, %2 VLAN, %3 tagged or untagged, %4 priority, %5 DSCP",
                          "%1: VLAN %2 (%3), priority %4, DSCP %5",
                          application,
                          vlan,
                          tagged ? i18nc("@info VLAN", "tagged") : i18nc("@info VLAN", "untagged"),
                          number(policy, lldpctl_k_med_policy_priority),
                          number(policy, lldpctl_k_med_policy_dscp));
    }
    return policies;
}

QString power(lldpctl_atom_t *neighbor)
{
    Atom dot3(lldpctl_atom_get(neighbor, lldpctl_k_port_dot3_power));
    if (!dot3) {
        return {};
    }
    const long deviceType = lldpctl_atom_get_int(dot3.get(), lldpctl_k_dot3_power_devicetype);
    if (deviceType <= 0) {
        return {};
    }
    // LLDP_DOT3_POWER_PSE: the neighbor supplies power (a switch); otherwise it draws it
    QStringList parts{deviceType == 1 ? i18nc("@info PoE", "Supplies power") : i18nc("@info PoE", "Draws power")};
    if (const QString powerClass = text(dot3.get(), lldpctl_k_dot3_power_class); !powerClass.isEmpty()) {
        parts << powerClass;
    }
    // Both in milliwatts
    if (const int allocated = number(dot3.get(), lldpctl_k_dot3_power_allocated); allocated > 0) {
        parts << i18nc("@info PoE", "%1 W allocated", allocated / 1000.0);
    }
    if (const int requested = number(dot3.get(), lldpctl_k_dot3_power_requested); requested > 0) {
        parts << i18nc("@info PoE", "%1 W requested", requested / 1000.0);
    }
    return parts.join(u", "_s);
}

Neighbor readNeighbor(lldpctl_atom_t *atom)
{
    Neighbor neighbor;
    neighbor.protocol = text(atom, lldpctl_k_port_protocol);
    neighbor.portId = text(atom, lldpctl_k_port_id);
    neighbor.portIdType = text(atom, lldpctl_k_port_id_subtype);
    neighbor.portDescription = text(atom, lldpctl_k_port_descr);
    neighbor.ttl = number(atom, lldpctl_k_port_ttl);
    neighbor.pvid = number(atom, lldpctl_k_port_vlan_pvid);
    neighbor.link = text(atom, lldpctl_k_port_dot3_mautype);
    neighbor.medPolicies = medPolicies(atom);
    neighbor.power = power(atom);

    if (Atom vlans(lldpctl_atom_get(atom, lldpctl_k_port_vlans)); vlans) {
        lldpctl_atom_t *vlan = nullptr;
        lldpctl_atom_foreach(vlans.get(), vlan)
        {
            const int id = number(vlan, lldpctl_k_vlan_id);
            const QString name = text(vlan, lldpctl_k_vlan_name);
            neighbor.vlans << (name.isEmpty() ? QString::number(id) : u"%1 %2"_s.arg(id).arg(name));
        }
    }

    if (Atom chassis(lldpctl_atom_get(atom, lldpctl_k_port_chassis)); chassis) {
        neighbor.systemName = text(chassis.get(), lldpctl_k_chassis_name);
        neighbor.systemDescription = text(chassis.get(), lldpctl_k_chassis_descr);
        neighbor.chassisId = text(chassis.get(), lldpctl_k_chassis_id);
        neighbor.chassisIdType = text(chassis.get(), lldpctl_k_chassis_id_subtype);
        neighbor.capabilities = number(chassis.get(), lldpctl_k_chassis_cap_enabled);
        neighbor.medClass = text(chassis.get(), lldpctl_k_chassis_med_type);
        if (neighbor.ttl == 0) {
            neighbor.ttl = number(chassis.get(), lldpctl_k_chassis_ttl);
        }
        if (Atom addresses(lldpctl_atom_get(chassis.get(), lldpctl_k_chassis_mgmt)); addresses) {
            lldpctl_atom_t *address = nullptr;
            lldpctl_atom_foreach(addresses.get(), address)
            {
                neighbor.managementAddresses << text(address, lldpctl_k_mgmt_ip);
            }
        }
    }
    return neighbor;
}
}

QString socketPath()
{
    return QString::fromUtf8(lldpctl_get_default_transport());
}

Snapshot read()
{
    Snapshot snapshot;
    snapshot.state = socketState();
    if (snapshot.state != State::Ok) {
        snapshot.error = socketError(snapshot.state);
        return snapshot;
    }
    Connection connection = connect();
    if (!connection) {
        snapshot.state = State::Error;
        snapshot.error = i18nc("@info", "Could not create a connection to lldpd.");
        return snapshot;
    }

    Atom interfaces(lldpctl_get_interfaces(connection.get()));
    if (!interfaces) {
        // A socket left behind by a daemon that stopped
        const bool refused = lldpctl_last_error(connection.get()) == LLDPCTL_ERR_CANNOT_CONNECT;
        snapshot.state = refused ? State::NotRunning : State::Error;
        snapshot.error = refused ? socketError(State::NotRunning) : lastError(connection.get());
        return snapshot;
    }
    lldpctl_atom_t *interface = nullptr;
    lldpctl_atom_foreach(interfaces.get(), interface)
    {
        Atom port(lldpctl_get_port(interface));
        if (!port) {
            continue;
        }
        LocalPort local;
        local.interfaceName = text(interface, lldpctl_k_interface_name);
        local.status = number(port.get(), lldpctl_k_port_status);
        local.portId = text(port.get(), lldpctl_k_port_id);
        local.portDescription = text(port.get(), lldpctl_k_port_descr);
        if (Atom neighbors(lldpctl_atom_get(port.get(), lldpctl_k_port_neighbors)); neighbors) {
            lldpctl_atom_t *neighbor = nullptr;
            lldpctl_atom_foreach(neighbors.get(), neighbor)
            {
                local.neighbors << readNeighbor(neighbor);
            }
        }
        snapshot.ports << local;
    }

    return snapshot;
}

Snapshot demo()
{
    Neighbor access;
    access.protocol = u"LLDP"_s;
    access.systemName = u"access-sw01.example.net"_s;
    access.systemDescription = u"Example switch OS 17.9, release software"_s;
    access.chassisId = u"02:00:5e:10:00:01"_s;
    access.chassisIdType = u"mac"_s;
    access.portId = u"Gi1/0/12"_s;
    access.portIdType = u"ifname"_s;
    access.portDescription = u"Desk 4.12"_s;
    access.managementAddresses = {u"192.0.2.10"_s};
    access.capabilities = 0x04 | 0x10;
    access.pvid = 10;
    access.vlans = {u"10 DATA"_s, u"20 VOICE"_s};
    access.ttl = 120;
    access.link = u"1000BaseTFD"_s;
    access.medClass = u"Network Connectivity Device"_s;
    access.medPolicies = {u"Voice: VLAN 20 (tagged), priority 5, DSCP 46"_s};
    access.power = u"Supplies power, class 4, 25.5 W allocated"_s;

    LocalPort wired;
    wired.interfaceName = u"enp0s31f6"_s;
    wired.status = RxOnly;
    wired.neighbors = {access};
    LocalPort dock;
    dock.interfaceName = u"enx020000000001"_s;
    dock.status = RxOnly;

    Snapshot snapshot;
    snapshot.state = State::Ok;
    snapshot.ports = {wired, dock};
    return snapshot;
}

}
