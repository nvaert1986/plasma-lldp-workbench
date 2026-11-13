// SPDX-FileCopyrightText: 2026 plasma-lldp-workbench contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "lldpsettings.h"

#include "lldpatom.h"

#include <KLocalizedString>

#include <QRegularExpression>

using namespace Qt::StringLiterals;

namespace LldpWorkbench::Settings
{
namespace
{
using namespace Atoms;

// lldpcli keywords next to the numbers lldpctl and lldp-const.h use for them
struct Keyword {
    int value;
    const char *keyword;
};

const Keyword capabilityTable[] = {
    {0x01, "other"},
    {0x02, "repeater"},
    {0x04, "bridge"},
    {0x08, "wlan"},
    {0x10, "router"},
    {0x20, "telephone"},
    {0x40, "docsis"},
    {0x80, "station"},
};
const Keyword statusTable[] = {{4, "rx-and-tx"}, {2, "rx-only"}, {1, "tx-only"}, {3, "disabled"}};
const Keyword agentTable[] = {{1, "nearest-bridge"}, {2, "nearest-non-tpmr-bridge"}, {3, "nearest-customer-bridge"}};
const Keyword bondTable[] = {{1, "real"}, {2, "zero"}, {3, "fixed"}, {4, "local"}};
const Keyword portIdTable[] = {{5, "ifname"}, {3, "macaddress"}};
const Keyword portDescriptionTable[] = {{0, "auto"}, {1, "ifname"}, {2, "alias"}};
const Keyword applicationTable[] = {
    {1, "voice"},
    {2, "voice-signaling"},
    {3, "guest-voice"},
    {4, "guest-voice-signaling"},
    {5, "softphone-voice"},
    {6, "video-conferencing"},
    {7, "streaming-video"},
    {8, "video-signaling"},
};
const Keyword civicTable[] = {
    {0, "language"},
    {1, "country-subdivision"},
    {2, "county"},
    {3, "city"},
    {4, "city-division"},
    {5, "block"},
    {6, "street"},
    {16, "direction"},
    {17, "trailing-street-suffix"},
    {18, "street-suffix"},
    {19, "number"},
    {20, "number-suffix"},
    {21, "landmark"},
    {22, "additional"},
    {23, "name"},
    {24, "zip"},
    {25, "building"},
    {26, "unit"},
    {27, "floor"},
    {28, "room"},
    {29, "place-type"},
    {128, "script"},
};
const Keyword datumTable[] = {{1, "WGS84"}, {2, "NAD83"}, {3, "NAD83/MLLW"}};
const Keyword medPowerRoleTable[] = {{1, "pse"}, {2, "pd"}};
const Keyword medPowerSourceTable[] = {{1, "unknown"}, {2, "primary"}, {3, "backup"}, {5, "pse"}, {6, "local"}, {7, "both"}};
const Keyword priorityTable[] = {{0, "unknown"}, {1, "critical"}, {2, "high"}, {3, "low"}};
const Keyword pairsTable[] = {{1, "signal"}, {2, "spare"}};
// 802.3 power sources share numbers between the two roles
const Keyword pseSourceTable[] = {{0, "unknown"}, {1, "primary"}, {2, "backup"}};
const Keyword pdSourceTable[] = {{0, "unknown"}, {1, "pse"}, {2, "local"}, {3, "both"}};

template<size_t N>
QString keyword(const Keyword (&table)[N], int value, const QString &fallback = {})
{
    for (const Keyword &entry : table) {
        if (entry.value == value) {
            return QString::fromLatin1(entry.keyword);
        }
    }
    return fallback;
}

template<size_t N>
QStringList keywords(const Keyword (&table)[N])
{
    QStringList result;
    for (const Keyword &entry : table) {
        result << QString::fromLatin1(entry.keyword);
    }
    return result;
}

bool flag(lldpctl_atom_t *atom, lldpctl_key_t key)
{
    return lldpctl_atom_get_int(atom, key) > 0;
}

QVariantMap readPolicies(lldpctl_atom_t *port)
{
    QVariantMap policies;
    Atom list(lldpctl_atom_get(port, lldpctl_k_port_med_policies));
    if (!list) {
        return policies;
    }
    lldpctl_atom_t *policy = nullptr;
    lldpctl_atom_foreach(list.get(), policy)
    {
        const QString application = keyword(applicationTable, number(policy, lldpctl_k_med_policy_type));
        if (application.isEmpty()) {
            continue;
        }
        policies.insert(application,
                        QVariantMap{
                            {u"unknown"_s, flag(policy, lldpctl_k_med_policy_unknown)},
                            {u"tagged"_s, flag(policy, lldpctl_k_med_policy_tagged)},
                            {u"vlan"_s, number(policy, lldpctl_k_med_policy_vid)},
                            {u"priority"_s, number(policy, lldpctl_k_med_policy_priority)},
                            {u"dscp"_s, number(policy, lldpctl_k_med_policy_dscp)},
                        });
    }
    return policies;
}

QVariantMap readLocations(lldpctl_atom_t *port)
{
    QVariantMap locations = defaultLocations();
    Atom list(lldpctl_atom_get(port, lldpctl_k_port_med_locations));
    if (!list) {
        return locations;
    }
    lldpctl_atom_t *location = nullptr;
    lldpctl_atom_foreach(list.get(), location)
    {
        switch (lldpctl_atom_get_int(location, lldpctl_k_med_location_format)) {
        case 1: {
            // lldpctl writes "48.85667 N"; lldpcli wants "48.85667N"
            const QString unit = text(location, lldpctl_k_med_location_altitude_unit).toLower();
            locations.insert(u"coordinate"_s,
                             QVariantMap{
                                 {u"enabled"_s, true},
                                 {u"latitude"_s, text(location, lldpctl_k_med_location_latitude).remove(u' ')},
                                 {u"longitude"_s, text(location, lldpctl_k_med_location_longitude).remove(u' ')},
                                 {u"altitude"_s, text(location, lldpctl_k_med_location_altitude)},
                                 {u"altitudeUnit"_s, unit.startsWith(u'f') ? u"f"_s : u"m"_s},
                                 {u"datum"_s, keyword(datumTable, number(location, lldpctl_k_med_location_geoid), u"WGS84"_s)},
                             });
            break;
        }
        case 2: {
            QVariantList elements;
            if (Atom list(lldpctl_atom_get(location, lldpctl_k_med_location_ca_elements)); list) {
                lldpctl_atom_t *element = nullptr;
                lldpctl_atom_foreach(list.get(), element)
                {
                    elements << QVariantMap{
                        {u"type"_s, keyword(civicTable, number(element, lldpctl_k_med_civicaddress_type), u"additional"_s)},
                        {u"value"_s, text(element, lldpctl_k_med_civicaddress_value)},
                    };
                }
            }
            locations.insert(u"civic"_s,
                             QVariantMap{
                                 {u"enabled"_s, true},
                                 {u"country"_s, text(location, lldpctl_k_med_location_country)},
                                 {u"elements"_s, elements},
                             });
            break;
        }
        case 3:
            locations.insert(u"elin"_s, QVariantMap{{u"enabled"_s, true}, {u"number"_s, text(location, lldpctl_k_med_location_elin)}});
            break;
        }
    }
    return locations;
}

QVariantMap readMedPower(lldpctl_atom_t *port)
{
    QVariantMap power = defaultMedPower();
    Atom atom(lldpctl_atom_get(port, lldpctl_k_port_med_power));
    if (!atom) {
        return power;
    }
    const QString role = keyword(medPowerRoleTable, number(atom.get(), lldpctl_k_med_power_type));
    if (role.isEmpty()) {
        return power;
    }
    power.insert(u"role"_s, role);
    power.insert(u"source"_s, keyword(medPowerSourceTable, number(atom.get(), lldpctl_k_med_power_source), u"unknown"_s));
    power.insert(u"priority"_s, keyword(priorityTable, number(atom.get(), lldpctl_k_med_power_priority), u"unknown"_s));
    power.insert(u"value"_s, number(atom.get(), lldpctl_k_med_power_val));
    return power;
}

QVariantMap readDot3Power(lldpctl_atom_t *port)
{
    QVariantMap power = defaultDot3Power();
    Atom atom(lldpctl_atom_get(port, lldpctl_k_port_dot3_power));
    if (!atom) {
        return power;
    }
    const int type = number(atom.get(), lldpctl_k_dot3_power_devicetype);
    if (type != 1 && type != 2) {
        return power;
    }
    const int source = number(atom.get(), lldpctl_k_dot3_power_source);
    power.insert(u"role"_s, type == 1 ? u"pse"_s : u"pd"_s);
    power.insert(u"supported"_s, flag(atom.get(), lldpctl_k_dot3_power_supported));
    power.insert(u"enabled"_s, flag(atom.get(), lldpctl_k_dot3_power_enabled));
    power.insert(u"pairControl"_s, flag(atom.get(), lldpctl_k_dot3_power_paircontrol));
    power.insert(u"pairs"_s, keyword(pairsTable, number(atom.get(), lldpctl_k_dot3_power_pairs), u"signal"_s));
    power.insert(u"powerClass"_s, number(atom.get(), lldpctl_k_dot3_power_class));
    power.insert(u"type"_s, number(atom.get(), lldpctl_k_dot3_power_type));
    power.insert(u"source"_s, type == 1 ? keyword(pseSourceTable, source, u"unknown"_s) : keyword(pdSourceTable, source, u"unknown"_s));
    power.insert(u"priority"_s, keyword(priorityTable, number(atom.get(), lldpctl_k_dot3_power_priority), u"unknown"_s));
    power.insert(u"requested"_s, number(atom.get(), lldpctl_k_dot3_power_requested));
    power.insert(u"allocated"_s, number(atom.get(), lldpctl_k_dot3_power_allocated));
    return power;
}

QVariantList readCustomTlvs(lldpctl_atom_t *port)
{
    QVariantList tlvs;
    Atom list(lldpctl_atom_get(port, lldpctl_k_custom_tlvs));
    if (!list) {
        return tlvs;
    }
    lldpctl_atom_t *tlv = nullptr;
    lldpctl_atom_foreach(list.get(), tlv)
    {
        tlvs << QVariantMap{
            {u"oui"_s, bytes(tlv, lldpctl_k_custom_tlv_oui)},
            {u"subtype"_s, number(tlv, lldpctl_k_custom_tlv_oui_subtype)},
            {u"info"_s, bytes(tlv, lldpctl_k_custom_tlv_oui_info_string)},
        };
    }
    return tlvs;
}

QVariantMap readPort(lldpctl_atom_t *port)
{
    const bool local = text(port, lldpctl_k_port_id_subtype) == "local"_L1;
    return {
        {u"status"_s, keyword(statusTable, number(port, lldpctl_k_port_status), u"rx-and-tx"_s)},
        {u"portIdLocal"_s, local ? text(port, lldpctl_k_port_id) : QString()},
        {u"portDescription"_s, text(port, lldpctl_k_port_descr)},
        {u"vlanTx"_s, number(port, lldpctl_k_port_vlan_tx)},
        {u"vlanTxPriority"_s, 0},
        {u"vlanTxDei"_s, 0},
#ifdef HAVE_LLDPCTL_VLAN_PATTERN
        {u"vlanPattern"_s, text(port, lldpctl_k_port_vlan_advertise_pattern)},
#else
        {u"vlanPattern"_s, QString()},
#endif
        {u"policies"_s, readPolicies(port)},
        {u"locations"_s, readLocations(port)},
        {u"medPower"_s, readMedPower(port)},
        {u"dot3Power"_s, readDot3Power(port)},
        {u"customTlvs"_s, readCustomTlvs(port)},
    };
}

// The simple settings: one value, one command. An empty unset means lldpd has no command to
// return to its default, so only a restart does it.
enum class Kind {
    Text,
    Bool,
    Number,
    Choice,
    List,
};

struct Field {
    const char *key;
    Kind kind;
    QStringList set;
    QStringList unset;
};

QList<Field> globalFields()
{
    return {
        {"hostname", Kind::Text, {u"configure"_s, u"system"_s, u"hostname"_s}, {u"unconfigure"_s, u"system"_s, u"hostname"_s}},
        {"description", Kind::Text, {u"configure"_s, u"system"_s, u"description"_s}, {u"unconfigure"_s, u"system"_s, u"description"_s}},
        {"platform", Kind::Text, {u"configure"_s, u"system"_s, u"platform"_s}, {u"unconfigure"_s, u"system"_s, u"platform"_s}},
        {"chassisId", Kind::Text, {u"configure"_s, u"system"_s, u"chassisid"_s}, {u"unconfigure"_s, u"system"_s, u"chassisid"_s}},
        {"capabilities",
         Kind::List,
         {u"configure"_s, u"system"_s, u"capabilities"_s, u"enabled"_s},
         {u"unconfigure"_s, u"system"_s, u"capabilities"_s, u"enabled"_s}},
        {"capabilitiesAdvertise",
         Kind::Bool,
         {u"configure"_s, u"lldp"_s, u"capabilities-advertisements"_s},
         {u"unconfigure"_s, u"lldp"_s, u"capabilities-advertisements"_s}},
        {"managementAdvertise",
         Kind::Bool,
         {u"configure"_s, u"lldp"_s, u"management-addresses-advertisements"_s},
         {u"unconfigure"_s, u"lldp"_s, u"management-addresses-advertisements"_s}},
        {"managementPattern",
         Kind::Text,
         {u"configure"_s, u"system"_s, u"ip"_s, u"management"_s, u"pattern"_s},
         {u"unconfigure"_s, u"system"_s, u"ip"_s, u"management"_s, u"pattern"_s}},
        {"interfacePattern",
         Kind::Text,
         {u"configure"_s, u"system"_s, u"interface"_s, u"pattern"_s},
         {u"unconfigure"_s, u"system"_s, u"interface"_s, u"pattern"_s}},
        {"permanentPattern",
         Kind::Text,
         {u"configure"_s, u"system"_s, u"interface"_s, u"permanent"_s},
         {u"unconfigure"_s, u"system"_s, u"interface"_s, u"permanent"_s}},
        {"interfaceDescription",
         Kind::Bool,
         {u"configure"_s, u"system"_s, u"interface"_s, u"description"_s},
         {u"unconfigure"_s, u"system"_s, u"interface"_s, u"description"_s}},
        {"promiscuous",
         Kind::Bool,
         {u"configure"_s, u"system"_s, u"interface"_s, u"promiscuous"_s},
         {u"unconfigure"_s, u"system"_s, u"interface"_s, u"promiscuous"_s}},
        {"bondSlaveMacType", Kind::Choice, {u"configure"_s, u"system"_s, u"bond-slave-src-mac-type"_s}, {}},
        {"maxNeighbors", Kind::Number, {u"configure"_s, u"system"_s, u"max-neighbors"_s}, {}},
        {"agentType", Kind::Choice, {u"configure"_s, u"lldp"_s, u"agent-type"_s}, {}},
        {"portIdType", Kind::Choice, {u"configure"_s, u"lldp"_s, u"portidsubtype"_s}, {}},
        {"portDescriptionSource", Kind::Choice, {u"configure"_s, u"lldp"_s, u"portdescription-source"_s}, {}},
        {"txHold", Kind::Number, {u"configure"_s, u"lldp"_s, u"tx-hold"_s}, {}},
        {"fastStart", Kind::Bool, {u"configure"_s, u"lldp"_s, u"fast-start"_s, u"enable"_s}, {u"unconfigure"_s, u"lldp"_s, u"fast-start"_s}},
        {"fastStartInterval", Kind::Number, {u"configure"_s, u"lldp"_s, u"fast-start"_s, u"tx-interval"_s}, {}},
    };
}

const char *const inventoryFields[] = {"hardware-revision", "software-revision", "firmware-revision", "serial-number", "manufacturer", "model", "asset"};

Change restartChange(const QString &id, const QString &description)
{
    return {id, {}, true, description};
}

Change commandChange(const QString &id, const QStringList &args)
{
    return {id, args, false, line(args)};
}

// A simple field's command for its new value, or a restart when lldpd cannot unset it
Change fieldChange(const QString &id, const Field &field, const QVariant &value, const QString &label)
{
    switch (field.kind) {
    case Kind::Bool:
        if (value.toBool()) {
            return commandChange(id, field.set);
        }
        break;
    case Kind::List:
        if (!value.toStringList().isEmpty()) {
            return commandChange(id, field.set + QStringList{value.toStringList().join(u',')});
        }
        break;
    case Kind::Number:
        return commandChange(id, field.set + QStringList{QString::number(value.toInt())});
    case Kind::Text:
    case Kind::Choice:
        if (!value.toString().isEmpty()) {
            return commandChange(id, field.set + QStringList{value.toString()});
        }
        break;
    }
    if (field.unset.isEmpty()) {
        return restartChange(id, i18nc("@info %1 is a setting", "Return %1 to lldpd's default", label));
    }
    return commandChange(id, field.unset);
}

QStringList policyArgs(const QString &port, const QString &application, const QVariantMap &policy)
{
    QStringList args{u"configure"_s, u"ports"_s, port, u"med"_s, u"policy"_s, u"application"_s, application};
    if (policy.value(u"unknown"_s).toBool()) {
        args << u"unknown"_s;
    }
    if (policy.value(u"tagged"_s).toBool()) {
        args << u"tagged"_s;
    }
    if (const int vlan = policy.value(u"vlan"_s).toInt(); vlan > 0) {
        args << u"vlan"_s << QString::number(vlan);
    }
    args << u"priority"_s << QString::number(policy.value(u"priority"_s).toInt());
    args << u"dscp"_s << QString::number(policy.value(u"dscp"_s).toInt());
    return args;
}

QStringList locationArgs(const QString &port, const QString &format, const QVariantMap &location)
{
    const QStringList prefix{u"configure"_s, u"ports"_s, port, u"med"_s, u"location"_s};
    if (format == "coordinate"_L1) {
        return prefix
            + QStringList{u"coordinate"_s,
                          u"latitude"_s,
                          location.value(u"latitude"_s).toString(),
                          u"longitude"_s,
                          location.value(u"longitude"_s).toString(),
                          u"altitude"_s,
                          location.value(u"altitude"_s).toString(),
                          location.value(u"altitudeUnit"_s).toString(),
                          u"datum"_s,
                          location.value(u"datum"_s).toString()};
    }
    if (format == "civic"_L1) {
        QStringList args = prefix + QStringList{u"address"_s, u"country"_s, location.value(u"country"_s).toString()};
        for (const QVariant &element : location.value(u"elements"_s).toList()) {
            const QVariantMap map = element.toMap();
            if (!map.value(u"value"_s).toString().isEmpty()) {
                args << map.value(u"type"_s).toString() << map.value(u"value"_s).toString();
            }
        }
        return args;
    }
    return prefix + QStringList{u"elin"_s, location.value(u"number"_s).toString()};
}

QStringList medPowerArgs(const QString &port, const QVariantMap &power)
{
    return {u"configure"_s,
            u"ports"_s,
            port,
            u"med"_s,
            u"power"_s,
            power.value(u"role"_s).toString(),
            u"source"_s,
            power.value(u"source"_s).toString(),
            u"priority"_s,
            power.value(u"priority"_s).toString(),
            u"value"_s,
            QString::number(power.value(u"value"_s).toInt())};
}

QStringList dot3PowerArgs(const QString &port, const QVariantMap &power)
{
    QStringList args{u"configure"_s, u"ports"_s, port, u"dot3"_s, u"power"_s, power.value(u"role"_s).toString()};
    if (power.value(u"supported"_s).toBool()) {
        args << u"supported"_s;
    }
    if (power.value(u"enabled"_s).toBool()) {
        args << u"enabled"_s;
    }
    if (power.value(u"pairControl"_s).toBool()) {
        args << u"paircontrol"_s;
    }
    args << u"powerpairs"_s << power.value(u"pairs"_s).toString();
    // lldpctl counts classes from 1 (class 0); 0 means none was given
    if (const int powerClass = power.value(u"powerClass"_s).toInt(); powerClass > 0) {
        args << u"class"_s << u"class-%1"_s.arg(powerClass - 1);
    }
    if (const int type = power.value(u"type"_s).toInt(); type > 0) {
        args << u"type"_s << QString::number(type) << u"source"_s << power.value(u"source"_s).toString() << u"priority"_s
             << power.value(u"priority"_s).toString() << u"requested"_s << QString::number(power.value(u"requested"_s).toInt()) << u"allocated"_s
             << QString::number(power.value(u"allocated"_s).toInt());
    }
    return args;
}

QString tlvKey(const QVariantMap &tlv)
{
    return tlv.value(u"oui"_s).toString().toUpper() + u'/' + QString::number(tlv.value(u"subtype"_s).toInt());
}

QMap<QString, QVariantMap> tlvsByKey(const QVariantList &list)
{
    QMap<QString, QVariantMap> tlvs;
    for (const QVariant &entry : list) {
        // lldpcli replaces a TLV with the same OUI and subtype, so the last one wins
        tlvs.insert(tlvKey(entry.toMap()), entry.toMap());
    }
    return tlvs;
}

void portChanges(QList<Change> &result, const QString &name, const QVariantMap &before, const QVariantMap &after)
{
    const QString id = u"ports/"_s + name + u'/';
    const QStringList prefix{u"configure"_s, u"ports"_s, name};
    const auto changed = [&](const char *key) {
        return before.value(QLatin1String(key)) != after.value(QLatin1String(key));
    };

    if (changed("status")) {
        result << commandChange(id + u"status"_s, prefix + QStringList{u"lldp"_s, u"status"_s, after.value(u"status"_s).toString()});
    }
    if (changed("portIdLocal")) {
        const QString value = after.value(u"portIdLocal"_s).toString();
        result << (value.isEmpty() ? restartChange(id + u"portIdLocal"_s, i18nc("@info", "Stop using a fixed port ID on %1", name))
                                   : commandChange(id + u"portIdLocal"_s, prefix + QStringList{u"lldp"_s, u"portidsubtype"_s, u"local"_s, value}));
    }
    if (changed("portDescription")) {
        const QString value = after.value(u"portDescription"_s).toString();
        result << (value.isEmpty() ? restartChange(id + u"portDescription"_s, i18nc("@info", "Stop using a fixed port description on %1", name))
                                   : commandChange(id + u"portDescription"_s, prefix + QStringList{u"lldp"_s, u"portdescription"_s, value}));
    }
    if (changed("vlanTx") || changed("vlanTxPriority") || changed("vlanTxDei")) {
        const int vlan = after.value(u"vlanTx"_s).toInt();
        result << (vlan <= 0 ? restartChange(id + u"vlanTx"_s, i18nc("@info", "Stop sending LLDP tagged on %1", name))
                             : commandChange(id + u"vlanTx"_s,
                                             prefix
                                                 + QStringList{u"lldp"_s,
                                                               u"vlan-tx"_s,
                                                               QString::number(vlan),
                                                               u"prio"_s,
                                                               QString::number(after.value(u"vlanTxPriority"_s).toInt()),
                                                               u"dei"_s,
                                                               QString::number(after.value(u"vlanTxDei"_s).toInt())}));
    }
    if (changed("vlanPattern")) {
        const QString value = after.value(u"vlanPattern"_s).toString();
        result << commandChange(id + u"vlanPattern"_s,
                                value.isEmpty() ? QStringList{u"unconfigure"_s, u"ports"_s, name, u"lldp"_s, u"vlan-advertisements"_s, u"pattern"_s}
                                                : prefix + QStringList{u"lldp"_s, u"vlan-advertisements"_s, u"pattern"_s, value});
    }

    const QVariantMap oldPolicies = before.value(u"policies"_s).toMap();
    const QVariantMap newPolicies = after.value(u"policies"_s).toMap();
    for (auto it = newPolicies.cbegin(); it != newPolicies.cend(); ++it) {
        if (oldPolicies.value(it.key()) != it.value()) {
            result << commandChange(id + u"policies/"_s + it.key(), policyArgs(name, it.key(), it.value().toMap()));
        }
    }
    for (auto it = oldPolicies.cbegin(); it != oldPolicies.cend(); ++it) {
        if (!newPolicies.contains(it.key())) {
            result << restartChange(id + u"policies/"_s + it.key(), i18nc("@info %1 application, %2 interface", "Remove the %1 policy on %2", it.key(), name));
        }
    }

    const QVariantMap oldLocations = before.value(u"locations"_s).toMap();
    const QVariantMap newLocations = after.value(u"locations"_s).toMap();
    for (const QString &format : {u"coordinate"_s, u"civic"_s, u"elin"_s}) {
        const QVariantMap oldLocation = oldLocations.value(format).toMap();
        const QVariantMap newLocation = newLocations.value(format).toMap();
        if (oldLocation == newLocation) {
            continue;
        }
        if (newLocation.value(u"enabled"_s).toBool()) {
            result << commandChange(id + u"locations/"_s + format, locationArgs(name, format, newLocation));
        } else if (oldLocation.value(u"enabled"_s).toBool()) {
            result << restartChange(id + u"locations/"_s + format, i18nc("@info", "Remove the %1 location on %2", format, name));
        }
    }

    const QVariantMap oldMedPower = before.value(u"medPower"_s).toMap();
    const QVariantMap newMedPower = after.value(u"medPower"_s).toMap();
    if (oldMedPower != newMedPower) {
        if (newMedPower.value(u"role"_s).toString() != "none"_L1) {
            result << commandChange(id + u"medPower"_s, medPowerArgs(name, newMedPower));
        } else if (oldMedPower.value(u"role"_s).toString() != "none"_L1) {
            result << restartChange(id + u"medPower"_s, i18nc("@info", "Stop sending LLDP-MED power on %1", name));
        }
    }

    const QVariantMap oldDot3 = before.value(u"dot3Power"_s).toMap();
    const QVariantMap newDot3 = after.value(u"dot3Power"_s).toMap();
    if (oldDot3 != newDot3) {
        if (newDot3.value(u"role"_s).toString() != "none"_L1) {
            result << commandChange(id + u"dot3Power"_s, dot3PowerArgs(name, newDot3));
        } else if (oldDot3.value(u"role"_s).toString() != "none"_L1) {
            result << restartChange(id + u"dot3Power"_s, i18nc("@info", "Stop sending 802.3 power on %1", name));
        }
    }

    const QMap<QString, QVariantMap> oldTlvs = tlvsByKey(before.value(u"customTlvs"_s).toList());
    const QMap<QString, QVariantMap> newTlvs = tlvsByKey(after.value(u"customTlvs"_s).toList());
    for (auto it = newTlvs.cbegin(); it != newTlvs.cend(); ++it) {
        if (oldTlvs.value(it.key()) == it.value()) {
            continue;
        }
        QStringList args = prefix
            + QStringList{u"lldp"_s,
                          u"custom-tlv"_s,
                          u"replace"_s,
                          u"oui"_s,
                          it.value().value(u"oui"_s).toString(),
                          u"subtype"_s,
                          QString::number(it.value().value(u"subtype"_s).toInt())};
        if (const QString info = it.value().value(u"info"_s).toString(); !info.isEmpty()) {
            args << u"oui-info"_s << info;
        }
        result << commandChange(id + u"customTlvs/"_s + it.key(), args);
    }
    for (auto it = oldTlvs.cbegin(); it != oldTlvs.cend(); ++it) {
        if (!newTlvs.contains(it.key())) {
            result << commandChange(id + u"customTlvs/"_s + it.key(),
                                    {u"unconfigure"_s,
                                     u"ports"_s,
                                     name,
                                     u"lldp"_s,
                                     u"custom-tlv"_s,
                                     u"oui"_s,
                                     it.value().value(u"oui"_s).toString(),
                                     u"subtype"_s,
                                     QString::number(it.value().value(u"subtype"_s).toInt())});
        }
    }
}
}

QVariantMap read(QString *error)
{
    Connection connection = connect();
    Atom config(connection ? lldpctl_get_configuration(connection.get()) : nullptr);
    if (!config) {
        *error = connection ? lastError(connection.get()) : i18nc("@info", "Could not create a connection to lldpd.");
        return {};
    }

    const int txMs = number(config.get(), lldpctl_k_config_tx_interval_ms);
    QVariantMap global{
        {u"hostname"_s, text(config.get(), lldpctl_k_config_hostname)},
        {u"description"_s, text(config.get(), lldpctl_k_config_description)},
        {u"platform"_s, text(config.get(), lldpctl_k_config_platform)},
        {u"chassisId"_s, text(config.get(), lldpctl_k_config_cid_string)},
        {u"capabilities"_s, capabilityKeywords(number(config.get(), lldpctl_k_config_chassis_cap_override))},
        {u"capabilitiesAdvertise"_s, flag(config.get(), lldpctl_k_config_chassis_cap_advertise)},
        {u"managementAdvertise"_s, flag(config.get(), lldpctl_k_config_chassis_mgmt_advertise)},
        {u"managementPattern"_s, text(config.get(), lldpctl_k_config_mgmt_pattern)},
        {u"interfacePattern"_s, text(config.get(), lldpctl_k_config_iface_pattern)},
        {u"permanentPattern"_s, text(config.get(), lldpctl_k_config_perm_iface_pattern)},
        {u"interfaceDescription"_s, flag(config.get(), lldpctl_k_config_ifdescr_update)},
        {u"promiscuous"_s, flag(config.get(), lldpctl_k_config_iface_promisc)},
        {u"bondSlaveMacType"_s, keyword(bondTable, number(config.get(), lldpctl_k_config_bond_slave_src_mac_type), u"local"_s)},
        {u"maxNeighbors"_s, number(config.get(), lldpctl_k_config_max_neighbors)},
        {u"agentType"_s, keyword(agentTable, number(config.get(), lldpctl_k_config_lldp_agent_type), u"nearest-bridge"_s)},
        {u"portIdType"_s, keyword(portIdTable, number(config.get(), lldpctl_k_config_lldp_portid_type))},
#ifdef HAVE_LLDPCTL_PORTDESCR_TYPE
        {u"portDescriptionSource"_s, keyword(portDescriptionTable, number(config.get(), lldpctl_k_config_lldp_portdescr_type), u"auto"_s)},
#else
        {u"portDescriptionSource"_s, u"auto"_s},
#endif
        {u"txInterval"_s, txMs > 0 ? txMs : number(config.get(), lldpctl_k_config_tx_interval) * 1000},
        {u"txHold"_s, number(config.get(), lldpctl_k_config_tx_hold)},
        {u"fastStart"_s, flag(config.get(), lldpctl_k_config_fast_start_enabled)},
        {u"fastStartInterval"_s, number(config.get(), lldpctl_k_config_fast_start_interval)},
    };
    QVariantMap info{{u"receiveOnly"_s, flag(config.get(), lldpctl_k_config_receiveonly)}};
    // What this build's lldpctl knows; the pages hide the rest
#ifdef HAVE_LLDPCTL_VLAN_PATTERN
    info.insert(u"supportsVlanPattern"_s, true);
#endif
#ifdef HAVE_LLDPCTL_PORTDESCR_TYPE
    info.insert(u"supportsPortDescriptionSource"_s, true);
#endif

    QVariantMap inventory;
    if (Atom chassis(lldpctl_get_local_chassis(connection.get())); chassis) {
        const lldpctl_key_t inventoryKeys[] = {
            lldpctl_k_chassis_med_inventory_hw,
            lldpctl_k_chassis_med_inventory_sw,
            lldpctl_k_chassis_med_inventory_fw,
            lldpctl_k_chassis_med_inventory_sn,
            lldpctl_k_chassis_med_inventory_manuf,
            lldpctl_k_chassis_med_inventory_model,
            lldpctl_k_chassis_med_inventory_asset,
        };
        for (size_t i = 0; i < std::size(inventoryKeys); ++i) {
            inventory.insert(QString::fromLatin1(inventoryFields[i]), text(chassis.get(), inventoryKeys[i]));
        }
        info.insert(u"capabilitiesAvailable"_s, capabilityKeywords(number(chassis.get(), lldpctl_k_chassis_cap_available)));
        info.insert(u"capabilitiesEnabled"_s, capabilityKeywords(number(chassis.get(), lldpctl_k_chassis_cap_enabled)));
        info.insert(u"medClass"_s, number(chassis.get(), lldpctl_k_chassis_med_type));
        info.insert(u"announcedName"_s, text(chassis.get(), lldpctl_k_chassis_name));
        info.insert(u"announcedDescription"_s, text(chassis.get(), lldpctl_k_chassis_descr));
    }
    global.insert(u"inventory"_s, inventory);

    QVariantMap ports;
    if (Atom interfaces(lldpctl_get_interfaces(connection.get())); interfaces) {
        lldpctl_atom_t *interface = nullptr;
        lldpctl_atom_foreach(interfaces.get(), interface)
        {
            if (Atom port(lldpctl_get_port(interface)); port) {
                ports.insert(text(interface, lldpctl_k_interface_name), readPort(port.get()));
            }
        }
    }
    return {{u"global"_s, global}, {u"ports"_s, ports}, {u"info"_s, info}};
}

QList<Change> changes(const QVariantMap &before, const QVariantMap &after)
{
    QList<Change> result;
    const QVariantMap oldGlobal = before.value(u"global"_s).toMap();
    const QVariantMap newGlobal = after.value(u"global"_s).toMap();
    for (const Field &field : globalFields()) {
        const QString key = QString::fromLatin1(field.key);
        if (oldGlobal.value(key) != newGlobal.value(key)) {
            result << fieldChange(u"global/"_s + key, field, newGlobal.value(key), key);
        }
    }
    if (oldGlobal.value(u"txInterval"_s) != newGlobal.value(u"txInterval"_s)) {
        // Whole seconds read better in a saved file; lldpcli takes milliseconds with "ms"
        const int ms = newGlobal.value(u"txInterval"_s).toInt();
        result << commandChange(u"global/txInterval"_s,
                                {u"configure"_s, u"lldp"_s, u"tx-interval"_s, ms % 1000 == 0 ? QString::number(ms / 1000) : u"%1ms"_s.arg(ms)});
    }

    const QVariantMap oldInventory = oldGlobal.value(u"inventory"_s).toMap();
    const QVariantMap newInventory = newGlobal.value(u"inventory"_s).toMap();
    for (const char *name : inventoryFields) {
        const QString field = QString::fromLatin1(name);
        if (oldInventory.value(field) == newInventory.value(field)) {
            continue;
        }
        const QString value = newInventory.value(field).toString();
        result << commandChange(u"global/inventory/"_s + field,
                                value.isEmpty() ? QStringList{u"unconfigure"_s, u"inventory"_s, field} : QStringList{u"configure"_s, u"inventory"_s, field, value});
    }

    const QVariantMap oldPorts = before.value(u"ports"_s).toMap();
    const QVariantMap newPorts = after.value(u"ports"_s).toMap();
    for (auto it = newPorts.cbegin(); it != newPorts.cend(); ++it) {
        portChanges(result, it.key(), oldPorts.value(it.key()).toMap(), it.value().toMap());
    }
    return result;
}

QString line(const QStringList &args)
{
    static const QRegularExpression plain(u"^[A-Za-z0-9_.,:/!*@+=-]+$"_s);
    QStringList parts;
    for (const QString &arg : args) {
        if (plain.match(arg).hasMatch()) {
            parts << arg;
        } else {
            QString clean = arg;
            clean.remove(u'"');
            parts << u'"' + clean + u'"';
        }
    }
    return parts.join(u' ');
}

QStringList capabilityKeywords(int mask)
{
    QStringList result;
    for (const Keyword &entry : capabilityTable) {
        if (mask & entry.value) {
            result << QString::fromLatin1(entry.keyword);
        }
    }
    return result;
}

int capabilityMask(const QStringList &names)
{
    int mask = 0;
    for (const Keyword &entry : capabilityTable) {
        if (names.contains(QLatin1String(entry.keyword))) {
            mask |= entry.value;
        }
    }
    return mask;
}

QVariant valueAt(const QVariantMap &tree, const QString &path)
{
    QVariant current = tree;
    for (const QString &part : path.split(u'/')) {
        if (current.typeId() == QMetaType::QVariantList) {
            current = current.toList().value(part.toInt());
        } else {
            current = current.toMap().value(part);
        }
    }
    return current;
}

namespace
{
QVariant replaced(const QVariant &node, QStringList parts, const QVariant &value)
{
    if (parts.isEmpty()) {
        return value;
    }
    const QString part = parts.takeFirst();
    if (node.typeId() == QMetaType::QVariantList) {
        QVariantList list = node.toList();
        const int index = part.toInt();
        if (index >= 0 && index < list.size()) {
            list[index] = replaced(list.at(index), parts, value);
        }
        return list;
    }
    QVariantMap map = node.toMap();
    map.insert(part, replaced(map.value(part), parts, value));
    return map;
}
}

void setValueAt(QVariantMap &tree, const QString &path, const QVariant &value)
{
    tree = replaced(tree, path.split(u'/'), value).toMap();
}

QVariantMap defaultPolicy(const QString &application)
{
    // The usual markings: EF for voice, CS3 for signaling, AF41 for video
    int priority = 0;
    int dscp = 0;
    if (application.endsWith(u"voice"_s)) {
        priority = 5;
        dscp = 46;
    } else if (application.endsWith(u"signaling"_s)) {
        priority = 3;
        dscp = 24;
    } else if (application.contains(u"video"_s)) {
        priority = 4;
        dscp = 34;
    }
    return {{u"unknown"_s, true}, {u"tagged"_s, false}, {u"vlan"_s, 0}, {u"priority"_s, priority}, {u"dscp"_s, dscp}};
}

QVariantMap defaultLocations()
{
    return {
        {u"coordinate"_s,
         QVariantMap{{u"enabled"_s, false},
                     {u"latitude"_s, QString()},
                     {u"longitude"_s, QString()},
                     {u"altitude"_s, u"0"_s},
                     {u"altitudeUnit"_s, u"m"_s},
                     {u"datum"_s, u"WGS84"_s}}},
        {u"civic"_s, QVariantMap{{u"enabled"_s, false}, {u"country"_s, QString()}, {u"elements"_s, QVariantList()}}},
        {u"elin"_s, QVariantMap{{u"enabled"_s, false}, {u"number"_s, QString()}}},
    };
}

QVariantMap defaultMedPower()
{
    return {{u"role"_s, u"none"_s}, {u"source"_s, u"unknown"_s}, {u"priority"_s, u"unknown"_s}, {u"value"_s, 0}};
}

QVariantMap defaultDot3Power()
{
    return {
        {u"role"_s, u"none"_s},
        {u"supported"_s, true},
        {u"enabled"_s, true},
        {u"pairControl"_s, false},
        {u"pairs"_s, u"signal"_s},
        {u"powerClass"_s, 0},
        {u"type"_s, 0},
        {u"source"_s, u"unknown"_s},
        {u"priority"_s, u"unknown"_s},
        {u"requested"_s, 0},
        {u"allocated"_s, 0},
    };
}

QStringList applications()
{
    return keywords(applicationTable);
}

QStringList civicTypes()
{
    return keywords(civicTable);
}
}
