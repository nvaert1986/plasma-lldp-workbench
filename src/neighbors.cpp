// SPDX-FileCopyrightText: 2026 plasma-lldp-workbench contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "neighbors.h"

#include <KLazyLocalizedString>
#include <KLocalizedString>

using namespace Qt::StringLiterals;

namespace LldpWorkbench
{
namespace
{
// Bits of the system capabilities TLV (IEEE 802.1AB, LLDP_CAP_* in lldp-const.h)
struct Capability {
    int bit;
    KLazyLocalizedString name;
};

const Capability capabilityTable[] = {
    {0x01, kli18nc("@item LLDP capability", "Other")},
    {0x02, kli18nc("@item LLDP capability", "Repeater")},
    {0x04, kli18nc("@item LLDP capability", "Bridge")},
    {0x08, kli18nc("@item LLDP capability", "Wireless access point")},
    {0x10, kli18nc("@item LLDP capability", "Router")},
    {0x20, kli18nc("@item LLDP capability", "Telephone")},
    {0x40, kli18nc("@item LLDP capability", "DOCSIS cable device")},
    {0x80, kli18nc("@item LLDP capability", "Station")},
    {0x100, kli18nc("@item LLDP capability", "C-VLAN component")},
    {0x200, kli18nc("@item LLDP capability", "S-VLAN component")},
    {0x400, kli18nc("@item LLDP capability", "Two-port MAC relay")},
};

// A port ID that is a MAC address says nothing to a person; the description usually does
QString portText(const Neighbor &neighbor)
{
    if (neighbor.portIdType == "mac"_L1 && !neighbor.portDescription.isEmpty()) {
        return neighbor.portDescription;
    }
    return neighbor.portId.isEmpty() ? neighbor.portDescription : neighbor.portId;
}

QString systemText(const Neighbor &neighbor)
{
    return neighbor.systemName.isEmpty() ? neighbor.chassisId : neighbor.systemName;
}

QString vlanText(const Neighbor &neighbor)
{
    QStringList parts;
    if (neighbor.pvid > 0) {
        parts << i18nc("@info untagged VLAN of a switch port", "%1 (untagged)", neighbor.pvid);
    }
    parts << neighbor.vlans;
    return parts.join(u", "_s);
}
}

QStringList capabilityNames(int mask)
{
    QStringList names;
    for (const Capability &capability : capabilityTable) {
        if (mask & capability.bit) {
            names << capability.name.toString();
        }
    }
    return names;
}

QString portStatusText(int status)
{
    switch (status) {
    case RxAndTx:
        return i18nc("@item LLDP port mode", "Send and receive");
    case RxOnly:
        return i18nc("@item LLDP port mode", "Receive only");
    case TxOnly:
        return i18nc("@item LLDP port mode", "Send only");
    case Disabled:
        return i18nc("@item LLDP port mode", "Off");
    }
    return i18nc("@item LLDP port mode", "Unknown");
}

QString summary(const Neighbor &neighbor)
{
    QStringList parts{systemText(neighbor), portText(neighbor)};
    if (neighbor.pvid > 0) {
        parts << i18nc("@info VLAN number", "VLAN %1", neighbor.pvid);
    }
    parts.removeAll(QString());
    return parts.join(u" · "_s);
}

QString summary(const LocalPort &port)
{
    if (port.neighbors.isEmpty()) {
        return {};
    }
    QString text = summary(port.neighbors.constFirst());
    if (port.neighbors.size() > 1) {
        text = i18ncp("@info %1 is a neighbor summary", "%1 (and %2 more)", "%1 (and %2 more)", text, port.neighbors.size() - 1);
    }
    return text;
}

QString clipboardText(const LocalPort &port)
{
    QStringList lines{i18nc("@info", "Interface: %1", port.interfaceName)};
    if (port.neighbors.isEmpty()) {
        lines << i18nc("@info", "No LLDP neighbor");
    }
    for (const Neighbor &neighbor : port.neighbors) {
        lines << QString();
        const auto add = [&lines](const KLocalizedString &line, const QString &value) {
            if (!value.isEmpty()) {
                lines << line.subs(value).toString();
            }
        };
        add(ki18nc("@info %1 is a system name", "System: %1"), systemText(neighbor));
        add(ki18nc("@info %1 is a port name", "Port: %1"), portText(neighbor));
        if (neighbor.portDescription != portText(neighbor)) {
            add(ki18nc("@info", "Port description: %1"), neighbor.portDescription);
        }
        add(ki18nc("@info", "VLAN: %1"), vlanText(neighbor));
        add(ki18nc("@info", "Management address: %1"), neighbor.managementAddresses.join(u", "_s));
        add(ki18nc("@info", "Capabilities: %1"), capabilityNames(neighbor.capabilities).join(u", "_s));
        add(ki18nc("@info", "Link: %1"), neighbor.link);
        add(ki18nc("@info", "Power over Ethernet: %1"), neighbor.power);
        add(ki18nc("@info", "Voice/video policy: %1"), neighbor.medPolicies.join(u"; "_s));
        add(ki18nc("@info", "System description: %1"), neighbor.systemDescription);
        add(ki18nc("@info", "Chassis ID: %1"), neighbor.chassisId);
    }
    return lines.join(u'\n');
}

QVariantMap toVariant(const Neighbor &neighbor)
{
    const QString port = portText(neighbor);
    return {
        {u"summary"_s, summary(neighbor)},
        {u"protocol"_s, neighbor.protocol},
        {u"systemName"_s, systemText(neighbor)},
        {u"systemDescription"_s, neighbor.systemDescription},
        {u"chassisId"_s, neighbor.chassisId},
        {u"port"_s, port},
        {u"portDescription"_s, neighbor.portDescription == port ? QString() : neighbor.portDescription},
        {u"vlans"_s, vlanText(neighbor)},
        {u"managementAddresses"_s, neighbor.managementAddresses.join(u", "_s)},
        {u"capabilities"_s, capabilityNames(neighbor.capabilities).join(u", "_s)},
        {u"link"_s, neighbor.link},
        {u"medClass"_s, neighbor.medClass},
        {u"medPolicies"_s, neighbor.medPolicies.join(u"\n"_s)},
        {u"power"_s, neighbor.power},
        {u"ttl"_s, neighbor.ttl},
    };
}
}
