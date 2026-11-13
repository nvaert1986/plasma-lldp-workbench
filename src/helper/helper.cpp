// SPDX-FileCopyrightText: 2026 plasma-lldp-workbench contributors
// SPDX-License-Identifier: GPL-3.0-or-later

// Runs as root through KAuth and polkit. It only writes lldpd's own configuration: the file
// of lldpcli commands in /etc/lldpd.d and lldpd's startup options, then restarts lldpd.

#include "../daemonoptions.h"

#include <KAuth/ActionReply>
#include <KAuth/HelperSupport>

#include <QDir>
#include <QFile>
#include <QSaveFile>

using namespace Qt::StringLiterals;

namespace LldpWorkbench
{
namespace
{
const QString configDirectory = u"/etc/lldpd.d"_s;
const QString configPath = configDirectory + u"/plasma-lldp-workbench.conf"_s;

KAuth::ActionReply failure(const QString &message)
{
    KAuth::ActionReply reply = KAuth::ActionReply::HelperErrorReply();
    reply.setErrorDescription(message);
    return reply;
}

// lldpd runs this file through lldpcli as root, so only configuration commands may go in
QString checkConfig(const QString &text)
{
    if (text.size() > 65536) {
        return u"The configuration is too long."_s;
    }
    for (const QString &line : text.split(u'\n')) {
        const QString trimmed = line.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith(u'#')) {
            continue;
        }
        if (!trimmed.startsWith(u"configure "_s) && !trimmed.startsWith(u"unconfigure "_s)) {
            return u"Not a configuration command: %1"_s.arg(trimmed);
        }
        for (const QChar c : trimmed) {
            if (c.category() == QChar::Other_Control) {
                return u"Control characters are not allowed."_s;
            }
        }
    }
    return {};
}

QString writeConfig(const QString &text)
{
    QDir().mkpath(configDirectory);
    QSaveFile file(configPath);
    if (!file.open(QIODevice::WriteOnly)) {
        return file.errorString();
    }
    file.write(text.toUtf8());
    if (!file.commit()) {
        return file.errorString();
    }
    QFile::setPermissions(configPath, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup | QFileDevice::ReadOther);
    return {};
}
}

class Helper : public QObject
{
    Q_OBJECT

public Q_SLOTS:
    KAuth::ActionReply apply(const QVariantMap &args)
    {
        if (args.value(u"writeConfig"_s).toBool()) {
            const QString text = args.value(u"config"_s).toString();
            if (const QString error = checkConfig(text); !error.isEmpty()) {
                return failure(error);
            }
            if (const QString error = writeConfig(text); !error.isEmpty()) {
                return failure(error);
            }
        }

        const Daemon::Service service = Daemon::readService();
        const bool daemon = args.value(u"daemon"_s).toBool();
        if (daemon) {
            Daemon::Options options;
            options.medClass = qBound(0, args.value(u"medClass"_s).toInt(), 4);
            options.noInventory = args.value(u"noInventory"_s).toBool();
            options.hideKernel = args.value(u"hideKernel"_s).toBool();
            options.receiveOnly = args.value(u"receiveOnly"_s).toBool();
            if (const QString error = Daemon::writeService(service, Daemon::withOptions(service.arguments, options)); !error.isEmpty()) {
                return failure(error);
            }
        }
        if (daemon || args.value(u"restart"_s).toBool()) {
            if (const QString error = Daemon::restartService(service); !error.isEmpty()) {
                return failure(error);
            }
        }
        return KAuth::ActionReply::SuccessReply();
    }
};
}

KAUTH_HELPER_MAIN("io.github.nvaert1986.lldpworkbench", LldpWorkbench::Helper)

#include "helper.moc"
