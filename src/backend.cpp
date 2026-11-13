// SPDX-FileCopyrightText: 2026 plasma-lldp-workbench contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "backend.h"

#include "lldpwatcher.h"

#include <KLocalizedString>
#include <KNotification>
#include <KStatusNotifierItem>

#include <QClipboard>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QStandardPaths>
#include <QWindow>

using namespace Qt::StringLiterals;

namespace LldpWorkbench
{
namespace
{
const auto desktopId = u"io.github.nvaert1986.lldpworkbench"_s;

QString autostartPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + u"/autostart/"_s + desktopId + u".desktop"_s;
}
}

Backend::Backend(bool demo, QObject *parent)
    : QObject(parent)
    , m_demo(demo)
    , m_watcher(new LldpWatcher(this))
{
    // Several changes arrive together when a switch announces itself; one read covers them
    m_debounce.setSingleShot(true);
    m_debounce.setInterval(200);
    connect(&m_debounce, &QTimer::timeout, this, &Backend::refresh);
    connect(m_watcher, &LldpWatcher::neighborsChanged, &m_debounce, qOverload<>(&QTimer::start));

    // Picks up lldpd starting, the group membership arriving and settings changed with
    // lldpcli, none of which lldpd announces to watchers
    m_retry.setInterval(5000);
    connect(&m_retry, &QTimer::timeout, this, &Backend::refresh);
    m_retry.start();

    m_tray = new KStatusNotifierItem(desktopId, this);
    m_tray->setTitle(i18nc("@title", "LLDP Workbench"));
    m_tray->setIconByName(u"network-rj45-female"_s);
    m_tray->setCategory(KStatusNotifierItem::Hardware);

    refresh();
}

Backend::~Backend()
{
    m_watcher->stop();
}

void Backend::setWindow(QWindow *window)
{
    m_tray->setAssociatedWindow(window);
}

int Backend::state() const
{
    switch (m_state) {
    case LldpClient::State::Ok:
        return Ok;
    case LldpClient::State::NotRunning:
        return NotRunning;
    case LldpClient::State::NoAccess:
        return NoAccess;
    case LldpClient::State::Error:
        break;
    }
    return Failed;
}

QString Backend::errorText() const
{
    return m_error;
}

QString Backend::socketPath() const
{
    return LldpClient::socketPath();
}

QString Backend::userName() const
{
    return qEnvironmentVariable("USER");
}

InterfaceModel *Backend::interfaces()
{
    return &m_interfaces;
}

QString Backend::summary() const
{
    for (const LocalPort &port : m_interfaces.ports()) {
        if (!port.neighbors.isEmpty()) {
            return LldpWorkbench::summary(port);
        }
    }
    return {};
}

QVariantList Backend::ports() const
{
    QVariantList ports;
    for (const LocalPort &port : m_interfaces.ports()) {
        ports << QVariantMap{{u"name"_s, port.interfaceName}, {u"status"_s, port.status}};
    }
    return ports;
}

bool Backend::autostart() const
{
    return QFile::exists(autostartPath());
}

void Backend::setAutostart(bool enabled)
{
    if (enabled == autostart()) {
        return;
    }
    const QString path = autostartPath();
    if (enabled) {
        QDir().mkpath(QFileInfo(path).path());
        QFile file(path);
        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            // Starts in the tray: at login the window would only be in the way
            file.write(QStringLiteral("[Desktop Entry]\n"
                                      "Type=Application\n"
                                      "Name=%1\n"
                                      "Exec=plasma-lldp-workbench --hidden\n"
                                      "Icon=network-rj45-female\n"
                                      "X-KDE-autostart-phase=2\n")
                           .arg(i18nc("@title", "LLDP Workbench"))
                           .toUtf8());
        }
    } else {
        QFile::remove(path);
    }
    Q_EMIT autostartChanged();
}

void Backend::refresh()
{
    apply(m_demo ? LldpClient::demo() : LldpClient::read());
}

void Backend::apply(LldpClient::Snapshot snapshot)
{
    const QString oldSummary = summary();
    const QList<LocalPort> before = m_interfaces.ports();

    if (snapshot.state != m_state || snapshot.error != m_error) {
        m_state = snapshot.state;
        m_error = snapshot.error;
        Q_EMIT stateChanged();
    }
    if (m_state != LldpClient::State::Ok) {
        // Old neighbors would look current while lldpd cannot be asked
        snapshot.ports.clear();
    }
    m_interfaces.setPorts(std::move(snapshot.ports));
    if (m_interfaces.ports() != before) {
        Q_EMIT portsChanged();
    }

    if (m_loaded && m_state == LldpClient::State::Ok) {
        notifyChanges(before, m_interfaces.ports());
    }
    m_loaded = m_loaded || m_state == LldpClient::State::Ok;

    if (summary() != oldSummary) {
        Q_EMIT summaryChanged();
    }
    updateTray();
    updateWatcher();
}

void Backend::notifyChanges(const QList<LocalPort> &before, const QList<LocalPort> &after)
{
    QHash<QString, QString> previous;
    for (const LocalPort &port : before) {
        previous.insert(port.interfaceName, LldpWorkbench::summary(port));
    }
    for (const LocalPort &port : after) {
        const QString now = LldpWorkbench::summary(port);
        // A new interface (a dock plugged in) only counts once it hears someone
        if (now == previous.value(port.interfaceName)) {
            continue;
        }
        const QString text = now.isEmpty() ? i18nc("@info %1 is a network interface", "%1 no longer hears an LLDP neighbor.", port.interfaceName)
                                           : i18nc("@info %1 is a network interface, %2 a switch and port", "%1 is connected to %2", port.interfaceName, now);
        KNotification::event(u"neighborChanged"_s, i18nc("@title", "LLDP neighbor changed"), text, u"network-rj45-female"_s);
    }
}

void Backend::updateTray()
{
    QStringList lines;
    for (const LocalPort &port : m_interfaces.ports()) {
        if (!port.neighbors.isEmpty()) {
            lines << u"%1: %2"_s.arg(port.interfaceName, LldpWorkbench::summary(port));
        }
    }
    if (m_state != LldpClient::State::Ok) {
        lines = {m_error};
    } else if (lines.isEmpty()) {
        lines << i18nc("@info", "No LLDP neighbor");
    }
    for (QString &line : lines) {
        line = line.toHtmlEscaped();
    }
    m_tray->setToolTip(u"network-rj45-female"_s, i18nc("@title", "LLDP Workbench"), lines.join(u"<br>"_s));
    // Passive moves the icon into the tray's hidden section while there is nothing to show
    m_tray->setStatus(summary().isEmpty() ? KStatusNotifierItem::Passive : KStatusNotifierItem::Active);
}

void Backend::updateWatcher()
{
    // Without a watch the five-second refresh picks up new neighbors
    if (LldpWatcher::supported() && !m_demo && m_state == LldpClient::State::Ok && !m_watcher->isRunning()) {
        m_watcher->start();
    }
}

void Backend::copyInterface(const QString &name) const
{
    for (const LocalPort &port : m_interfaces.ports()) {
        if (port.interfaceName == name) {
            copyText(clipboardText(port));
        }
    }
}

void Backend::copyAll() const
{
    QStringList blocks;
    for (const LocalPort &port : m_interfaces.ports()) {
        blocks << clipboardText(port);
    }
    copyText(blocks.join(u"\n\n"_s));
}

void Backend::copyText(const QString &text) const
{
    QGuiApplication::clipboard()->setText(text);
}

}
