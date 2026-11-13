// SPDX-FileCopyrightText: 2026 plasma-lldp-workbench contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "settingseditor.h"

#include <KAuth/Action>
#include <KAuth/ExecuteJob>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>

#include <QProcess>
#include <QStandardPaths>
#include <QTimer>

using namespace Qt::StringLiterals;

namespace LldpWorkbench
{
namespace
{
// What Apply has set, keyed like Settings::Change::id, so Save Permanently writes the user's
// own choices rather than everything lldpd reports
KConfigGroup savedSettings()
{
    return KSharedConfig::openStateConfig()->group(u"SavedSettings"_s);
}

QString lldpcli()
{
    const QString found = QStandardPaths::findExecutable(u"lldpcli"_s, {u"/usr/sbin"_s, u"/usr/bin"_s, u"/sbin"_s, u"/bin"_s});
    return found.isEmpty() ? QStandardPaths::findExecutable(u"lldpcli"_s) : found;
}

QVariantMap toMap(const Daemon::Options &options)
{
    return {
        {u"medClass"_s, options.medClass},
        {u"noInventory"_s, options.noInventory},
        {u"hideKernel"_s, options.hideKernel},
        {u"receiveOnly"_s, options.receiveOnly},
    };
}

QString medClassText(int medClass)
{
    switch (medClass) {
    case 1:
        return i18nc("@item LLDP-MED class", "Class I, generic endpoint");
    case 2:
        return i18nc("@item LLDP-MED class", "Class II, media endpoint");
    case 3:
        return i18nc("@item LLDP-MED class", "Class III, communication device such as an IP phone");
    case 4:
        return i18nc("@item LLDP-MED class", "Network connectivity device");
    }
    return i18nc("@item LLDP-MED class", "No LLDP-MED");
}
}

SettingsEditor::SettingsEditor(QObject *parent)
    : QObject(parent)
{
}

QVariantMap SettingsEditor::edited() const
{
    return m_edited;
}

QVariantMap SettingsEditor::info() const
{
    return m_loaded.value(u"info"_s).toMap();
}

QStringList SettingsEditor::portNames() const
{
    return m_loaded.value(u"ports"_s).toMap().keys();
}

QStringList SettingsEditor::pending() const
{
    QStringList result;
    for (const Settings::Change &change : Settings::changes(m_loaded, m_edited)) {
        result << change.description;
    }
    const Daemon::Options wanted = daemonOptions();
    if (wanted.medClass != m_daemonLoaded.medClass) {
        result << i18nc("@info", "Start lldpd with LLDP-MED: %1", medClassText(wanted.medClass));
    }
    if (wanted.noInventory != m_daemonLoaded.noInventory) {
        result << (wanted.noInventory ? i18nc("@info", "Start lldpd without sending inventory") : i18nc("@info", "Start lldpd sending inventory"));
    }
    if (wanted.hideKernel != m_daemonLoaded.hideKernel) {
        result << (wanted.hideKernel ? i18nc("@info", "Start lldpd hiding the kernel version") : i18nc("@info", "Start lldpd showing the kernel version"));
    }
    if (wanted.receiveOnly != m_daemonLoaded.receiveOnly) {
        result << (wanted.receiveOnly ? i18nc("@info", "Start lldpd in receive-only mode") : i18nc("@info", "Start lldpd able to send"));
    }
    return result;
}

bool SettingsEditor::dirty() const
{
    return m_edited != m_loaded || daemonOptions() != m_daemonLoaded;
}

bool SettingsEditor::needsRestart() const
{
    if (daemonOptions() != m_daemonLoaded) {
        return true;
    }
    const QList<Settings::Change> changes = Settings::changes(m_loaded, m_edited);
    return std::any_of(changes.cbegin(), changes.cend(), [](const Settings::Change &change) {
        return change.needsRestart;
    });
}

QString SettingsEditor::loadError() const
{
    return m_loadError;
}

QVariantMap SettingsEditor::daemon() const
{
    return m_daemonEdited;
}

QString SettingsEditor::service() const
{
    return Daemon::joinCommand(QStringList{m_service.program} + m_service.arguments);
}

QString SettingsEditor::serviceError() const
{
    return m_service.error;
}

QString SettingsEditor::savedConfig() const
{
    const KConfigGroup group = savedSettings();
    QStringList keys = group.keyList();
    keys.sort();
    QStringList lines{u"# Written by plasma-lldp-workbench"_s};
    for (const QString &key : std::as_const(keys)) {
        lines << group.readEntry(key, QString());
    }
    return lines.join(u'\n') + u'\n';
}

bool SettingsEditor::busy() const
{
    return m_busy;
}

QStringList SettingsEditor::applications() const
{
    return Settings::applications();
}

QStringList SettingsEditor::civicTypes() const
{
    return Settings::civicTypes();
}

Daemon::Options SettingsEditor::daemonOptions() const
{
    Daemon::Options options;
    options.medClass = m_daemonEdited.value(u"medClass"_s).toInt();
    options.noInventory = m_daemonEdited.value(u"noInventory"_s).toBool();
    options.hideKernel = m_daemonEdited.value(u"hideKernel"_s).toBool();
    options.receiveOnly = m_daemonEdited.value(u"receiveOnly"_s).toBool();
    return options;
}

void SettingsEditor::reload()
{
    QString error;
    m_loaded = Settings::read(&error);
    m_loadError = error;
    m_edited = m_loaded;
    m_service = Daemon::readService();
    m_daemonLoaded = Daemon::parse(m_service.arguments);
    m_daemonEdited = toMap(m_daemonLoaded);
    Q_EMIT loaded();
    Q_EMIT changed();
}

QVariant SettingsEditor::value(const QString &path) const
{
    if (path.startsWith(u"daemon/"_s)) {
        return m_daemonEdited.value(path.mid(7));
    }
    return Settings::valueAt(m_edited, path);
}

void SettingsEditor::set(const QString &path, const QVariant &value)
{
    if (path.startsWith(u"daemon/"_s)) {
        setDaemon(path.mid(7), value);
        return;
    }
    if (Settings::valueAt(m_edited, path) == value) {
        return;
    }
    Settings::setValueAt(m_edited, path, value);
    Q_EMIT changed();
}

void SettingsEditor::setDaemon(const QString &key, const QVariant &value)
{
    if (m_daemonEdited.value(key) == value) {
        return;
    }
    m_daemonEdited.insert(key, value);
    Q_EMIT changed();
}

void SettingsEditor::addPolicy(const QString &port, const QString &application)
{
    set(u"ports/%1/policies/%2"_s.arg(port, application), Settings::defaultPolicy(application));
}

void SettingsEditor::removePolicy(const QString &port, const QString &application)
{
    const QString path = u"ports/%1/policies"_s.arg(port);
    QVariantMap policies = Settings::valueAt(m_edited, path).toMap();
    policies.remove(application);
    set(path, policies);
}

void SettingsEditor::addListEntry(const QString &path)
{
    QVariantList list = Settings::valueAt(m_edited, path).toList();
    if (path.endsWith(u"/customTlvs"_s)) {
        list << QVariantMap{{u"oui"_s, u"00,00,00"_s}, {u"subtype"_s, 0}, {u"info"_s, QString()}};
    } else {
        list << QVariantMap{{u"type"_s, u"street"_s}, {u"value"_s, QString()}};
    }
    set(path, list);
}

void SettingsEditor::removeListEntry(const QString &path, int index)
{
    QVariantList list = Settings::valueAt(m_edited, path).toList();
    if (index >= 0 && index < list.size()) {
        list.removeAt(index);
        set(path, list);
    }
}

void SettingsEditor::applyProfile(const QString &profile)
{
    int medClass = 0;
    QStringList requested;
    QStringList capabilities;
    bool fastStart = m_edited.value(u"global"_s).toMap().value(u"fastStart"_s).toBool();
    QVariantMap medPower = Settings::defaultMedPower();
    QVariantMap dot3Power = Settings::defaultDot3Power();

    if (profile == "phone"_L1) {
        // A desk phone: asks the switch for its voice VLAN and draws class 2 PoE
        medClass = 3;
        requested = {u"voice"_s, u"voice-signaling"_s};
        fastStart = true;
        medPower = {{u"role"_s, u"pd"_s}, {u"source"_s, u"pse"_s}, {u"priority"_s, u"high"_s}, {u"value"_s, 6500}};
        dot3Power.insert(u"role"_s, u"pd"_s);
        dot3Power.insert(u"powerClass"_s, 3);
    } else if (profile == "video"_L1) {
        medClass = 2;
        requested = {u"video-conferencing"_s, u"video-signaling"_s};
        fastStart = true;
    } else if (profile == "softphone"_L1) {
        medClass = 1;
        requested = {u"softphone-voice"_s};
        fastStart = true;
    } else if (profile == "accessPoint"_L1) {
        capabilities = {u"bridge"_s, u"wlan"_s};
        dot3Power.insert(u"role"_s, u"pd"_s);
        dot3Power.insert(u"pairs"_s, u"spare"_s);
        dot3Power.insert(u"powerClass"_s, 5);
    } else if (profile == "switch"_L1) {
        medClass = 4;
        capabilities = {u"bridge"_s, u"router"_s};
    }

    // lldpd refuses capabilities this machine does not have
    const QStringList available = info().value(u"capabilitiesAvailable"_s).toStringList();
    capabilities.erase(std::remove_if(capabilities.begin(),
                                      capabilities.end(),
                                      [&available](const QString &capability) {
                                          return !available.contains(capability);
                                      }),
                       capabilities.end());

    Settings::setValueAt(m_edited, u"global/capabilities"_s, capabilities);
    Settings::setValueAt(m_edited, u"global/fastStart"_s, fastStart);
    for (const QString &port : portNames()) {
        QVariantMap policies;
        for (const QString &application : std::as_const(requested)) {
            policies.insert(application, Settings::defaultPolicy(application));
        }
        Settings::setValueAt(m_edited, u"ports/%1/policies"_s.arg(port), policies);
        Settings::setValueAt(m_edited, u"ports/%1/medPower"_s.arg(port), medPower);
        Settings::setValueAt(m_edited, u"ports/%1/dot3Power"_s.arg(port), dot3Power);
    }
    m_daemonEdited.insert(u"medClass"_s, medClass);
    Q_EMIT changed();
}

void SettingsEditor::discard()
{
    m_edited = m_loaded;
    m_daemonEdited = toMap(m_daemonLoaded);
    Q_EMIT changed();
}

void SettingsEditor::apply()
{
    if (m_busy) {
        return;
    }
    m_queue.clear();
    m_errors.clear();
    m_restartPending = false;
    for (const Settings::Change &change : Settings::changes(m_loaded, m_edited)) {
        if (change.needsRestart) {
            // Gone after the restart; the saved file must not bring it back either
            m_restartPending = true;
            record(change);
        } else {
            m_queue << change;
        }
    }
    m_daemonPending = daemonOptions() != m_daemonLoaded;
    setBusy(true);
    runNext();
}

void SettingsEditor::runNext()
{
    if (m_queue.isEmpty()) {
        if (m_errors.isEmpty() && (m_restartPending || m_daemonPending)) {
            runHelper(true, m_daemonPending);
            return;
        }
        reload();
        finish(m_errors.isEmpty(), m_errors.isEmpty() ? i18nc("@info", "Applied until lldpd restarts") : m_errors.join(u'\n'));
        return;
    }
    const Settings::Change change = m_queue.takeFirst();
    auto process = new QProcess(this);
    connect(process, &QProcess::finished, this, [this, process, change](int code, QProcess::ExitStatus status) {
        const QString output = QString::fromUtf8(process->readAllStandardError()).trimmed();
        process->deleteLater();
        // lldpcli reports refused values on stderr
        if (status == QProcess::NormalExit && code == 0 && output.isEmpty()) {
            record(change);
        } else {
            m_errors << u"%1: %2"_s.arg(change.description, output.isEmpty() ? i18nc("@info", "lldpcli failed") : output);
        }
        runNext();
    });
    connect(process, &QProcess::errorOccurred, this, [this, process, change](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart) {
            return;
        }
        process->deleteLater();
        m_errors << u"%1: %2"_s.arg(change.description, i18nc("@info", "lldpcli could not be started"));
        runNext();
    });
    process->start(lldpcli(), change.args);
}

void SettingsEditor::runHelper(bool restart, bool daemonChanged)
{
    const Daemon::Options options = daemonOptions();
    KAuth::Action action(u"io.github.nvaert1986.lldpworkbench.apply"_s);
    action.setHelperId(u"io.github.nvaert1986.lldpworkbench"_s);
    action.setArguments({
        // A restart loses every runtime setting, so the user's settings are saved first
        {u"writeConfig"_s, true},
        {u"config"_s, savedConfig()},
        {u"daemon"_s, daemonChanged},
        {u"medClass"_s, options.medClass},
        {u"noInventory"_s, options.noInventory},
        {u"hideKernel"_s, options.hideKernel},
        {u"receiveOnly"_s, options.receiveOnly},
        {u"restart"_s, restart},
    });
    KAuth::ExecuteJob *job = action.execute();
    connect(job, &KJob::result, this, [this, job, restart] {
        if (job->error()) {
            reload();
            finish(false, job->errorText().isEmpty() ? job->errorString() : job->errorText());
            return;
        }
        // lldpd needs a moment before its socket answers again
        QTimer::singleShot(restart ? 2000 : 0, this, [this, restart] {
            reload();
            finish(true, restart ? i18nc("@info", "Saved permanently and lldpd restarted") : i18nc("@info", "Saved permanently"));
        });
    });
    job->start();
}

void SettingsEditor::savePermanently()
{
    if (m_busy) {
        return;
    }
    setBusy(true);
    runHelper(false, false);
}

void SettingsEditor::finish(bool success, const QString &message)
{
    setBusy(false);
    Q_EMIT finished(success, message);
}

void SettingsEditor::record(const Settings::Change &change)
{
    KConfigGroup group = savedSettings();
    if (change.args.isEmpty() || change.args.constFirst() == "unconfigure"_L1) {
        group.deleteEntry(change.id);
    } else {
        group.writeEntry(change.id, Settings::line(change.args));
    }
    group.sync();
    Q_EMIT savedChanged();
}

void SettingsEditor::setBusy(bool busy)
{
    if (m_busy != busy) {
        m_busy = busy;
        Q_EMIT busyChanged();
    }
}
}
