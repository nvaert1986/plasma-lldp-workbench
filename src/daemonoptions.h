// SPDX-FileCopyrightText: 2026 plasma-lldp-workbench contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QStringList>

// lldpd's startup options that have no runtime command. Shared by the app, which shows them,
// and the root helper, which writes them into the service configuration.
namespace LldpWorkbench::Daemon
{
struct Options {
    // 0 means no LLDP-MED; 1 to 4 are the classes of -M
    int medClass = 0;
    bool noInventory = false;
    bool hideKernel = false;
    bool receiveOnly = false;

    bool operator==(const Options &) const = default;
};

Options parse(const QStringList &arguments);
// The same command line with the managed options replaced and every other one kept
QStringList withOptions(const QStringList &arguments, const Options &options);

QStringList splitCommand(const QString &command);
QString joinCommand(const QStringList &arguments);

struct Service {
    enum Kind {
        Systemd,
        OpenRc,
        Unknown,
    };
    Kind kind = Unknown;
    QString program;
    QStringList arguments;
    QString error;
};

// How lldpd is started on this system: systemd's ExecStart or OpenRC's /etc/conf.d/lldpd
Service readService();

// Root only. Each returns an error message, or an empty string.
QString writeService(const Service &service, const QStringList &arguments);
QString restartService(const Service &service);

// systemctl show prints ExecStart as "{ path=... ; argv[]=... ; ... }"
QStringList execStartArguments(const QString &systemctlValue);
// The LLDPD_OPTS line of /etc/conf.d/lldpd, replaced or added
QString withConfdOptions(const QString &content, const QStringList &arguments);
QStringList confdOptions(const QString &content);
}
