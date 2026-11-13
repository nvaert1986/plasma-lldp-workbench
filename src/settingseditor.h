// SPDX-FileCopyrightText: 2026 plasma-lldp-workbench contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "daemonoptions.h"
#include "lldpsettings.h"

#include <QObject>
#include <QVariantMap>
#include <QtQmlIntegration/qqmlintegration.h>

namespace LldpWorkbench
{
// Edits to lldpd's settings wait here until Apply: the settings pages bind to edited, every
// change goes through set(), and pending lists what Apply will do.
class SettingsEditor : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by the application")
    Q_PROPERTY(QVariantMap edited READ edited NOTIFY changed)
    Q_PROPERTY(QVariantMap info READ info NOTIFY loaded)
    Q_PROPERTY(QStringList portNames READ portNames NOTIFY loaded)
    Q_PROPERTY(QStringList pending READ pending NOTIFY changed)
    Q_PROPERTY(bool dirty READ dirty NOTIFY changed)
    Q_PROPERTY(bool needsRestart READ needsRestart NOTIFY changed)
    Q_PROPERTY(QString loadError READ loadError NOTIFY loaded)
    Q_PROPERTY(QVariantMap daemon READ daemon NOTIFY changed)
    Q_PROPERTY(QString service READ service NOTIFY loaded)
    Q_PROPERTY(QString serviceError READ serviceError NOTIFY loaded)
    Q_PROPERTY(QString savedConfig READ savedConfig NOTIFY savedChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QStringList applications READ applications CONSTANT)
    Q_PROPERTY(QStringList civicTypes READ civicTypes CONSTANT)

public:
    explicit SettingsEditor(QObject *parent = nullptr);

    QVariantMap edited() const;
    QVariantMap info() const;
    QStringList portNames() const;
    QStringList pending() const;
    bool dirty() const;
    bool needsRestart() const;
    QString loadError() const;
    QVariantMap daemon() const;
    QString service() const;
    QString serviceError() const;
    QString savedConfig() const;
    bool busy() const;
    QStringList applications() const;
    QStringList civicTypes() const;

    Q_INVOKABLE void reload();
    // Paths as in Settings; "daemon/<key>" reaches the startup options
    Q_INVOKABLE QVariant value(const QString &path) const;
    Q_INVOKABLE void set(const QString &path, const QVariant &value);
    Q_INVOKABLE void setDaemon(const QString &key, const QVariant &value);
    Q_INVOKABLE void addPolicy(const QString &port, const QString &application);
    Q_INVOKABLE void removePolicy(const QString &port, const QString &application);
    Q_INVOKABLE void addListEntry(const QString &path);
    Q_INVOKABLE void removeListEntry(const QString &path, int index);
    Q_INVOKABLE void applyProfile(const QString &profile);
    Q_INVOKABLE void discard();
    Q_INVOKABLE void apply();
    Q_INVOKABLE void savePermanently();

Q_SIGNALS:
    void changed();
    // lldpd was read again: interfaces, what it reports and how it starts may differ
    void loaded();
    void savedChanged();
    void busyChanged();
    // After Apply or Save Permanently, for a message to the user
    void finished(bool success, const QString &message);

private:
    Daemon::Options daemonOptions() const;
    void runNext();
    void runHelper(bool restart, bool daemonChanged);
    void finish(bool success, const QString &message);
    void record(const Settings::Change &change);
    void setBusy(bool busy);

    QVariantMap m_loaded;
    QVariantMap m_edited;
    QString m_loadError;
    Daemon::Service m_service;
    Daemon::Options m_daemonLoaded;
    QVariantMap m_daemonEdited;

    QList<Settings::Change> m_queue;
    QStringList m_errors;
    bool m_restartPending = false;
    bool m_daemonPending = false;
    bool m_busy = false;
};
}
