// SPDX-FileCopyrightText: 2026 plasma-lldp-workbench contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "interfacemodel.h"
#include "neighbors.h"

#include <QSignalSpy>
#include <QTest>

using namespace LldpWorkbench;
using namespace Qt::StringLiterals;

class NeighborsTest : public QObject
{
    Q_OBJECT

private:
    static Neighbor accessSwitch()
    {
        Neighbor neighbor;
        neighbor.protocol = u"LLDP"_s;
        neighbor.systemName = u"core-sw01.example.net"_s;
        neighbor.chassisId = u"00:11:22:33:44:55"_s;
        neighbor.portId = u"Gi1/0/12"_s;
        neighbor.portIdType = u"ifname"_s;
        neighbor.portDescription = u"Desk 4.12"_s;
        neighbor.pvid = 10;
        neighbor.vlans = {u"10 DATA"_s, u"20 VOICE"_s};
        neighbor.capabilities = 0x04 | 0x10;
        neighbor.managementAddresses = {u"192.0.2.10"_s};
        return neighbor;
    }

private Q_SLOTS:
    void capabilities()
    {
        QCOMPARE(capabilityNames(0x04 | 0x10), (QStringList{u"Bridge"_s, u"Router"_s}));
        QCOMPARE(capabilityNames(0), QStringList());
    }

    void summaryUsesNamePortAndVlan()
    {
        QCOMPARE(summary(accessSwitch()), u"core-sw01.example.net · Gi1/0/12 · VLAN 10"_s);
    }

    void summaryPrefersDescriptionOverMacPort()
    {
        Neighbor neighbor = accessSwitch();
        neighbor.portId = u"00:11:22:33:44:66"_s;
        neighbor.portIdType = u"mac"_s;
        neighbor.pvid = 0;
        QCOMPARE(summary(neighbor), u"core-sw01.example.net · Desk 4.12"_s);
    }

    void summaryFallsBackToChassisId()
    {
        Neighbor neighbor = accessSwitch();
        neighbor.systemName.clear();
        QCOMPARE(summary(neighbor), u"00:11:22:33:44:55 · Gi1/0/12 · VLAN 10"_s);
    }

    void summaryCountsExtraNeighbors()
    {
        LocalPort port;
        port.interfaceName = u"eth0"_s;
        QCOMPARE(summary(port), QString());
        port.neighbors = {accessSwitch(), accessSwitch()};
        QCOMPARE(summary(port), u"core-sw01.example.net · Gi1/0/12 · VLAN 10 (and 1 more)"_s);
    }

    void clipboard()
    {
        LocalPort port;
        port.interfaceName = u"eth0"_s;
        port.neighbors = {accessSwitch()};
        const QString text = clipboardText(port);
        QVERIFY(text.startsWith(u"Interface: eth0\n"_s));
        QVERIFY(text.contains(u"System: core-sw01.example.net\n"_s));
        QVERIFY(text.contains(u"Port: Gi1/0/12\n"_s));
        QVERIFY(text.contains(u"Port description: Desk 4.12\n"_s));
        QVERIFY(text.contains(u"VLAN: 10 (untagged), 10 DATA, 20 VOICE\n"_s));
        QVERIFY(text.contains(u"Capabilities: Bridge, Router\n"_s));
        // Values the switch did not send leave no empty lines behind
        QVERIFY(!text.contains(u"Link:"_s));

        LocalPort empty;
        empty.interfaceName = u"eth1"_s;
        QCOMPARE(clipboardText(empty), u"Interface: eth1\nNo LLDP neighbor"_s);
    }

    void modelSortsAndSkipsIdenticalUpdates()
    {
        InterfaceModel model;
        QSignalSpy resets(&model, &QAbstractItemModel::modelReset);

        LocalPort quiet;
        quiet.interfaceName = u"enp1s0"_s;
        LocalPort busy;
        busy.interfaceName = u"enp2s0"_s;
        busy.neighbors = {accessSwitch()};

        model.setPorts({quiet, busy});
        QCOMPARE(resets.count(), 1);
        QCOMPARE(model.rowCount(), 2);
        // The interface that hears a switch comes first
        QCOMPARE(model.index(0).data(InterfaceModel::NameRole).toString(), u"enp2s0"_s);
        QCOMPARE(model.index(0).data(InterfaceModel::NeighborCountRole).toInt(), 1);
        QCOMPARE(model.index(0).data(InterfaceModel::SummaryRole).toString(), u"core-sw01.example.net · Gi1/0/12 · VLAN 10"_s);

        model.setPorts({busy, quiet});
        QCOMPARE(resets.count(), 1);

        busy.neighbors.first().pvid = 20;
        model.setPorts({quiet, busy});
        QCOMPARE(resets.count(), 2);
    }

    void modelHidesIdleInterfacesOnRequest()
    {
        InterfaceModel model;
        LocalPort quiet;
        quiet.interfaceName = u"enp1s0"_s;
        LocalPort busy;
        busy.interfaceName = u"enp2s0"_s;
        busy.neighbors = {accessSwitch()};
        model.setPorts({quiet, busy});

        model.setShowIdle(false);
        QCOMPARE(model.rowCount(), 1);
        QCOMPARE(model.index(0).data(InterfaceModel::NameRole).toString(), u"enp2s0"_s);
        // Sending settings and the saved configuration still cover every interface
        QCOMPARE(model.ports().size(), 2);

        model.setShowIdle(true);
        QCOMPARE(model.rowCount(), 2);
    }
};

QTEST_GUILESS_MAIN(NeighborsTest)

#include "neighborstest.moc"
