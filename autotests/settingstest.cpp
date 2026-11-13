// SPDX-FileCopyrightText: 2026 plasma-lldp-workbench contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "daemonoptions.h"
#include "lldpsettings.h"

#include <QTest>

using namespace LldpWorkbench;
using namespace Qt::StringLiterals;

class SettingsTest : public QObject
{
    Q_OBJECT

private:
    // A state as Settings::read() returns it, with one interface
    static QVariantMap state()
    {
        QVariantMap global{
            {u"hostname"_s, QString()},
            {u"description"_s, QString()},
            {u"capabilities"_s, QStringList()},
            {u"fastStart"_s, false},
            {u"txInterval"_s, 30000},
            {u"agentType"_s, u"nearest-bridge"_s},
            {u"portIdType"_s, QString()},
            {u"inventory"_s, QVariantMap{{u"model"_s, QString()}}},
        };
        QVariantMap port{
            {u"status"_s, u"rx-and-tx"_s},
            {u"portIdLocal"_s, QString()},
            {u"vlanTx"_s, 0},
            {u"vlanTxPriority"_s, 0},
            {u"vlanTxDei"_s, 0},
            {u"policies"_s, QVariantMap()},
            {u"locations"_s, Settings::defaultLocations()},
            {u"medPower"_s, Settings::defaultMedPower()},
            {u"dot3Power"_s, Settings::defaultDot3Power()},
            {u"customTlvs"_s, QVariantList()},
        };
        return {{u"global"_s, global}, {u"ports"_s, QVariantMap{{u"eth0.12"_s, port}}}};
    }

    static QStringList lines(const QList<Settings::Change> &changes)
    {
        QStringList result;
        for (const Settings::Change &change : changes) {
            result << (change.needsRestart ? u"restart: "_s + change.id : Settings::line(change.args));
        }
        return result;
    }

private Q_SLOTS:
    void noChangesForTheSameState()
    {
        QVERIFY(Settings::changes(state(), state()).isEmpty());
    }

    void globalSettings()
    {
        const QVariantMap before = state();
        QVariantMap after = before;
        Settings::setValueAt(after, u"global/hostname"_s, u"support laptop"_s);
        Settings::setValueAt(after, u"global/capabilities"_s, QStringList{u"bridge"_s, u"wlan"_s});
        Settings::setValueAt(after, u"global/fastStart"_s, true);
        Settings::setValueAt(after, u"global/txInterval"_s, 1500);
        Settings::setValueAt(after, u"global/agentType"_s, u"nearest-customer-bridge"_s);
        Settings::setValueAt(after, u"global/inventory/model"_s, u"Example phone"_s);
        QCOMPARE(lines(Settings::changes(before, after)),
                 (QStringList{
                     u"configure system hostname \"support laptop\""_s,
                     u"configure system capabilities enabled bridge,wlan"_s,
                     u"configure lldp agent-type nearest-customer-bridge"_s,
                     u"configure lldp fast-start enable"_s,
                     u"configure lldp tx-interval 1500ms"_s,
                     u"configure inventory model \"Example phone\""_s,
                 }));

        // Back to lldpd's defaults: unconfigure where lldpcli has it, a restart where not
        QCOMPARE(lines(Settings::changes(after, before)),
                 (QStringList{
                     u"unconfigure system hostname"_s,
                     u"unconfigure system capabilities enabled"_s,
                     u"configure lldp agent-type nearest-bridge"_s,
                     u"unconfigure lldp fast-start"_s,
                     u"configure lldp tx-interval 30"_s,
                     u"unconfigure inventory model"_s,
                 }));
    }

    void phonePolicyAndPower()
    {
        const QVariantMap before = state();
        QVariantMap after = before;
        // Interface names with dots must survive the path syntax
        Settings::setValueAt(after, u"ports/eth0.12/policies/voice"_s, Settings::defaultPolicy(u"voice"_s));
        Settings::setValueAt(after,
                             u"ports/eth0.12/medPower"_s,
                             QVariantMap{{u"role"_s, u"pd"_s}, {u"source"_s, u"pse"_s}, {u"priority"_s, u"high"_s}, {u"value"_s, 6500}});
        QVariantMap dot3 = Settings::defaultDot3Power();
        dot3.insert(u"role"_s, u"pd"_s);
        dot3.insert(u"powerClass"_s, 3);
        Settings::setValueAt(after, u"ports/eth0.12/dot3Power"_s, dot3);
        QCOMPARE(lines(Settings::changes(before, after)),
                 (QStringList{
                     u"configure ports eth0.12 med policy application voice unknown priority 5 dscp 46"_s,
                     u"configure ports eth0.12 med power pd source pse priority high value 6500"_s,
                     u"configure ports eth0.12 dot3 power pd supported enabled powerpairs signal class class-2"_s,
                 }));

        // lldpcli cannot take these back; only a restart of lldpd does
        QCOMPARE(lines(Settings::changes(after, before)),
                 (QStringList{
                     u"restart: ports/eth0.12/policies/voice"_s,
                     u"restart: ports/eth0.12/medPower"_s,
                     u"restart: ports/eth0.12/dot3Power"_s,
                 }));
    }

    void locationsAndPortSettings()
    {
        const QVariantMap before = state();
        QVariantMap after = before;
        Settings::setValueAt(after, u"ports/eth0.12/status"_s, u"rx-only"_s);
        Settings::setValueAt(after, u"ports/eth0.12/vlanTx"_s, 20);
        Settings::setValueAt(after, u"ports/eth0.12/vlanTxPriority"_s, 5);
        Settings::setValueAt(after, u"ports/eth0.12/locations/elin/enabled"_s, true);
        Settings::setValueAt(after, u"ports/eth0.12/locations/elin/number"_s, u"112"_s);
        Settings::setValueAt(after, u"ports/eth0.12/locations/civic/enabled"_s, true);
        Settings::setValueAt(after, u"ports/eth0.12/locations/civic/country"_s, u"NL"_s);
        Settings::setValueAt(after,
                             u"ports/eth0.12/locations/civic/elements"_s,
                             QVariantList{QVariantMap{{u"type"_s, u"city"_s}, {u"value"_s, u"Example City"_s}}, QVariantMap{{u"type"_s, u"room"_s}, {u"value"_s, QString()}}});
        QCOMPARE(lines(Settings::changes(before, after)),
                 (QStringList{
                     u"configure ports eth0.12 lldp status rx-only"_s,
                     u"configure ports eth0.12 lldp vlan-tx 20 prio 5 dei 0"_s,
                     // Empty address parts are left out
                     u"configure ports eth0.12 med location address country NL city \"Example City\""_s,
                     u"configure ports eth0.12 med location elin 112"_s,
                 }));
    }

    void customTlvs()
    {
        const QVariantMap before = state();
        QVariantMap after = before;
        Settings::setValueAt(after,
                             u"ports/eth0.12/customTlvs"_s,
                             QVariantList{QVariantMap{{u"oui"_s, u"00,12,0f"_s}, {u"subtype"_s, 1}, {u"info"_s, u"AA,BB"_s}}});
        QCOMPARE(lines(Settings::changes(before, after)),
                 (QStringList{u"configure ports eth0.12 lldp custom-tlv replace oui 00,12,0f subtype 1 oui-info AA,BB"_s}));
        QCOMPARE(lines(Settings::changes(after, before)), (QStringList{u"unconfigure ports eth0.12 lldp custom-tlv oui 00,12,0f subtype 1"_s}));
    }

    void paths()
    {
        QVariantMap tree = state();
        Settings::setValueAt(tree, u"ports/eth0.12/customTlvs"_s, QVariantList{QVariantMap{{u"subtype"_s, 1}}});
        Settings::setValueAt(tree, u"ports/eth0.12/customTlvs/0/subtype"_s, 7);
        QCOMPARE(Settings::valueAt(tree, u"ports/eth0.12/customTlvs/0/subtype"_s).toInt(), 7);
        // An index past the end changes nothing
        Settings::setValueAt(tree, u"ports/eth0.12/customTlvs/3/subtype"_s, 9);
        QCOMPARE(Settings::valueAt(tree, u"ports/eth0.12/customTlvs"_s).toList().size(), 1);
    }

    void capabilities()
    {
        QCOMPARE(Settings::capabilityKeywords(0x04 | 0x80), (QStringList{u"bridge"_s, u"station"_s}));
        QCOMPARE(Settings::capabilityMask({u"router"_s, u"telephone"_s}), 0x10 | 0x20);
    }

    void daemonOptions()
    {
        const QStringList arguments{u"-d"_s, u"-M3"_s, u"-ci"_s, u"-I"_s, u"eth*"_s, u"$EXTRA"_s};
        const Daemon::Options options = Daemon::parse(arguments);
        QCOMPARE(options.medClass, 3);
        QVERIFY(options.noInventory);
        QVERIFY(!options.hideKernel);

        Daemon::Options wanted;
        wanted.medClass = 1;
        wanted.hideKernel = true;
        // Unmanaged options and $VARIABLES stay, the managed ones are replaced
        QCOMPARE(Daemon::withOptions(arguments, wanted), (QStringList{u"-d"_s, u"-c"_s, u"-I"_s, u"eth*"_s, u"$EXTRA"_s, u"-M"_s, u"1"_s, u"-k"_s}));
        QCOMPARE(Daemon::withOptions({}, {}), QStringList());
    }

    void serviceConfiguration()
    {
        QCOMPARE(Daemon::execStartArguments(u"{ path=/usr/sbin/lldpd ; argv[]=/usr/sbin/lldpd -c -M 3 ; ignore_errors=no ; pid=1 }"_s),
                 (QStringList{u"/usr/sbin/lldpd"_s, u"-c"_s, u"-M"_s, u"3"_s}));

        const QString confd = u"# options\n#LLDPD_OPTS=\"\"\nLLDPD_OPTS=\"-c\"\n"_s;
        QCOMPARE(Daemon::confdOptions(confd), QStringList{u"-c"_s});
        QCOMPARE(Daemon::withConfdOptions(confd, {u"-c"_s, u"-M"_s, u"3"_s}), u"# options\n#LLDPD_OPTS=\"\"\nLLDPD_OPTS=\"-c -M 3\"\n"_s);
        // Only commented out: a new line is added
        QCOMPARE(Daemon::withConfdOptions(u"#LLDPD_OPTS=\"\""_s, {u"-k"_s}), u"#LLDPD_OPTS=\"\"\nLLDPD_OPTS=\"-k\"\n"_s);
    }
};

QTEST_GUILESS_MAIN(SettingsTest)

#include "settingstest.moc"
