// SPDX-FileCopyrightText: 2026 plasma-lldp-workbench contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QStringList>
#include <QVariantMap>

namespace LldpWorkbench::Settings
{
// lldpd's runtime settings as a tree that QML can show and edit:
//   global/<key>, global/inventory/<field>, ports/<interface>/<key>, info/<key> (read only)
// Lists and maps inside a port: policies/<application>, locations/{coordinate,civic,elin},
// medPower, dot3Power, customTlvs (a list).
QVariantMap read(QString *error);

// One step towards the edited state. args is an lldpcli command; it is empty when lldpd can
// only drop the value by restarting, which needsRestart then says.
struct Change {
    QString id;
    QStringList args;
    bool needsRestart = false;
    QString description;
};

QList<Change> changes(const QVariantMap &before, const QVariantMap &after);

// lldpcli syntax, quoting arguments with spaces; the form lldpd reads from /etc/lldpd.d
QString line(const QStringList &args);

QStringList capabilityKeywords(int mask);
int capabilityMask(const QStringList &keywords);

// Path access with "/" between levels: interface names may contain dots (eth0.12)
QVariant valueAt(const QVariantMap &tree, const QString &path);
void setValueAt(QVariantMap &tree, const QString &path, const QVariant &value);

// Starting points for new list entries and profiles
QVariantMap defaultPolicy(const QString &application);
QVariantMap defaultLocations();
QVariantMap defaultMedPower();
QVariantMap defaultDot3Power();

QStringList applications();
QStringList civicTypes();
}
