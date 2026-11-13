// SPDX-FileCopyrightText: 2026 plasma-lldp-workbench contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "interfacemodel.h"
#include "lldpclient.h"

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QtQmlIntegration/qqmlintegration.h>

class KStatusNotifierItem;
class QWindow;

namespace LldpWorkbench
{
class LldpWatcher;

class Backend : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by the application")
    Q_PROPERTY(int state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString errorText READ errorText NOTIFY stateChanged)
    Q_PROPERTY(QString socketPath READ socketPath CONSTANT)
    Q_PROPERTY(QString userName READ userName CONSTANT)
    Q_PROPERTY(LldpWorkbench::InterfaceModel *interfaces READ interfaces CONSTANT)
    Q_PROPERTY(QString summary READ summary NOTIFY summaryChanged)
    // Every interface lldpd manages, also those the neighbor view hides: {name, status}
    Q_PROPERTY(QVariantList ports READ ports NOTIFY portsChanged)
    Q_PROPERTY(bool autostart READ autostart WRITE setAutostart NOTIFY autostartChanged)

public:
    // Mirrors LldpClient::State for QML
    enum StateValue {
        Ok,
        NotRunning,
        NoAccess,
        Failed,
    };
    Q_ENUM(StateValue)

    explicit Backend(bool demo = false, QObject *parent = nullptr);
    ~Backend() override;

    // The tray icon shows and hides this window
    void setWindow(QWindow *window);

    int state() const;
    QString errorText() const;
    QString socketPath() const;
    QString userName() const;
    InterfaceModel *interfaces();
    QString summary() const;

    QVariantList ports() const;

    bool autostart() const;
    void setAutostart(bool enabled);

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void copyInterface(const QString &name) const;
    Q_INVOKABLE void copyAll() const;
    Q_INVOKABLE void copyText(const QString &text) const;

Q_SIGNALS:
    void stateChanged();
    void summaryChanged();
    void portsChanged();
    void autostartChanged();

private:
    void apply(LldpClient::Snapshot snapshot);
    void notifyChanges(const QList<LocalPort> &before, const QList<LocalPort> &after);
    void updateTray();
    void updateWatcher();

    LldpClient::State m_state = LldpClient::State::Error;
    QString m_error;
    InterfaceModel m_interfaces;
    bool m_loaded = false;
    bool m_demo = false;

    LldpWatcher *m_watcher = nullptr;
    QTimer m_retry;
    QTimer m_debounce;
    KStatusNotifierItem *m_tray = nullptr;
};
}
