// SPDX-FileCopyrightText: 2026 plasma-lldp-workbench contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "daemonoptions.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QSaveFile>

using namespace Qt::StringLiterals;

namespace LldpWorkbench::Daemon
{
namespace
{
// lldpd's options that take a value (its getopt string), so "-M 3" and "-M3" both parse
const QString optionsWithValue = u"DpXmu46ICMPSLOH@t"_s;
const QString managed = u"Mikr"_s;

const QString confdPath = u"/etc/conf.d/lldpd"_s;
const QString dropInDirectory = u"/etc/systemd/system/lldpd.service.d"_s;
const QString dropInPath = dropInDirectory + u"/plasma-lldp-workbench.conf"_s;

struct Token {
    QChar option;
    QString value;
    // Anything that is not an option, such as a $VARIABLE from the unit file
    QString verbatim;
};

QList<Token> tokens(const QStringList &arguments)
{
    QList<Token> result;
    for (int i = 0; i < arguments.size(); ++i) {
        const QString &argument = arguments.at(i);
        if (argument.size() < 2 || !argument.startsWith(u'-') || argument.startsWith(u"--"_s)) {
            result << Token{{}, {}, argument};
            continue;
        }
        for (int j = 1; j < argument.size(); ++j) {
            const QChar option = argument.at(j);
            if (optionsWithValue.contains(option)) {
                const QString value = j + 1 < argument.size() ? argument.mid(j + 1) : arguments.value(++i);
                result << Token{option, value, {}};
                break;
            }
            result << Token{option, {}, {}};
        }
    }
    return result;
}

QString run(const QString &program, const QStringList &arguments)
{
    QProcess process;
    process.start(program, arguments);
    if (!process.waitForFinished(30000) || process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        const QString output = QString::fromUtf8(process.readAllStandardError()).trimmed();
        return output.isEmpty() ? u"%1 %2 failed"_s.arg(program, arguments.join(u' ')) : output;
    }
    return {};
}
}

Options parse(const QStringList &arguments)
{
    Options options;
    for (const Token &token : tokens(arguments)) {
        if (token.option == u'M') {
            options.medClass = qBound(0, token.value.toInt(), 4);
        } else if (token.option == u'i') {
            options.noInventory = true;
        } else if (token.option == u'k') {
            options.hideKernel = true;
        } else if (token.option == u'r') {
            options.receiveOnly = true;
        }
    }
    return options;
}

QStringList withOptions(const QStringList &arguments, const Options &options)
{
    QStringList result;
    for (const Token &token : tokens(arguments)) {
        if (!token.verbatim.isEmpty()) {
            result << token.verbatim;
        } else if (!managed.contains(token.option)) {
            result << u'-' + token.option;
            if (optionsWithValue.contains(token.option)) {
                result << token.value;
            }
        }
    }
    if (options.medClass > 0) {
        result << u"-M"_s << QString::number(options.medClass);
    }
    if (options.noInventory) {
        result << u"-i"_s;
    }
    if (options.hideKernel) {
        result << u"-k"_s;
    }
    if (options.receiveOnly) {
        result << u"-r"_s;
    }
    return result;
}

QStringList splitCommand(const QString &command)
{
    return QProcess::splitCommand(command);
}

QString joinCommand(const QStringList &arguments)
{
    QStringList parts;
    for (const QString &argument : arguments) {
        parts << (argument.contains(u' ') ? u'"' + argument + u'"' : argument);
    }
    return parts.join(u' ');
}

QStringList execStartArguments(const QString &systemctlValue)
{
    static const QRegularExpression argv(u"argv\\[\\]=(.*?) ;"_s);
    return splitCommand(argv.match(systemctlValue).captured(1));
}

QStringList confdOptions(const QString &content)
{
    static const QRegularExpression option(u"^\\s*LLDPD_OPTS=\"?(.*?)\"?\\s*$"_s, QRegularExpression::MultilineOption);
    return splitCommand(option.match(content).captured(1));
}

QString withConfdOptions(const QString &content, const QStringList &arguments)
{
    static const QRegularExpression option(u"^\\s*LLDPD_OPTS=.*$"_s, QRegularExpression::MultilineOption);
    const QString line = u"LLDPD_OPTS=\"%1\""_s.arg(joinCommand(arguments).replace(u'"', u'\''));
    if (option.match(content).hasMatch()) {
        QString result = content;
        return result.replace(option, line);
    }
    return content + (content.isEmpty() || content.endsWith(u'\n') ? QString() : u"\n"_s) + line + u'\n';
}

Service readService()
{
    Service service;
    if (QFileInfo(u"/run/systemd/system"_s).isDir()) {
        service.kind = Service::Systemd;
        QProcess process;
        process.start(u"systemctl"_s, {u"show"_s, u"lldpd.service"_s, u"--property=ExecStart"_s, u"--value"_s});
        process.waitForFinished(10000);
        const QStringList command = execStartArguments(QString::fromUtf8(process.readAllStandardOutput()));
        if (command.isEmpty()) {
            service.error = u"The lldpd service has no ExecStart line."_s;
            return service;
        }
        service.program = command.constFirst();
        service.arguments = command.mid(1);
        return service;
    }
    if (QFile::exists(confdPath)) {
        service.kind = Service::OpenRc;
        QFile file(confdPath);
        if (file.open(QIODevice::ReadOnly)) {
            service.arguments = confdOptions(QString::fromUtf8(file.readAll()));
        }
        service.program = u"lldpd"_s;
        return service;
    }
    service.error = u"Neither systemd nor OpenRC's /etc/conf.d/lldpd was found."_s;
    return service;
}

QString writeService(const Service &service, const QStringList &arguments)
{
    if (service.kind == Service::Systemd) {
        QDir().mkpath(dropInDirectory);
        QSaveFile file(dropInPath);
        if (!file.open(QIODevice::WriteOnly)) {
            return file.errorString();
        }
        file.write(u"# Written by plasma-lldp-workbench: lldpd's startup options\n"
                   "[Service]\n"
                   "ExecStart=\n"
                   "ExecStart=%1\n"_s.arg(joinCommand(QStringList{service.program} + arguments))
                       .toUtf8());
        if (!file.commit()) {
            return file.errorString();
        }
        file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup | QFileDevice::ReadOther);
        return run(u"systemctl"_s, {u"daemon-reload"_s});
    }
    if (service.kind == Service::OpenRc) {
        QFile current(confdPath);
        const QString content = current.open(QIODevice::ReadOnly) ? QString::fromUtf8(current.readAll()) : QString();
        current.close();
        QSaveFile file(confdPath);
        if (!file.open(QIODevice::WriteOnly)) {
            return file.errorString();
        }
        file.write(withConfdOptions(content, arguments).toUtf8());
        return file.commit() ? QString() : file.errorString();
    }
    return service.error;
}

QString restartService(const Service &service)
{
    switch (service.kind) {
    case Service::Systemd:
        return run(u"systemctl"_s, {u"restart"_s, u"lldpd.service"_s});
    case Service::OpenRc:
        return run(u"rc-service"_s, {u"lldpd"_s, u"restart"_s});
    case Service::Unknown:
        break;
    }
    return service.error;
}
}
