// SPDX-FileCopyrightText: 2026 plasma-lldp-workbench contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "app.h"
#include "backend.h"
#include "settingseditor.h"

#include <KAboutData>
#include <KDBusService>
#include <KLocalizedQmlContext>
#include <KLocalizedString>
#include <KWindowSystem>

#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTimer>

using namespace LldpWorkbench;
using namespace Qt::StringLiterals;

int main(int argc, char **argv)
{
    // QApplication rather than QGuiApplication: the tray icon's menu is a widget
    QApplication app(argc, argv);
    // Closing the window keeps the tray icon; Quit in its menu ends the app
    QApplication::setQuitOnLastWindowClosed(false);
    KLocalizedString::setApplicationDomain("plasma-lldp-workbench");
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
        QQuickStyle::setStyle(u"org.kde.desktop"_s);
    }

    KAboutData about(u"plasma-lldp-workbench"_s,
                     i18nc("@title", "LLDP Workbench"),
                     QStringLiteral(LLDPWORKBENCH_VERSION),
                     i18nc("@info", "Shows the switch, port and VLAN this computer is connected to, and controls what it sends over LLDP"),
                     KAboutLicense::GPL_V3);
    about.setOrganizationDomain("nvaert1986.github.io");
    about.setDesktopFileName(u"io.github.nvaert1986.lldpworkbench"_s);
    KAboutData::setApplicationData(about);
    QApplication::setWindowIcon(QIcon::fromTheme(u"network-rj45-female"_s));

    QCommandLineParser parser;
    about.setupCommandLine(&parser);
    parser.addOption({u"hidden"_s, i18nc("@info:shell", "Start in the system tray without opening the window")});
    parser.addOption({u"demo"_s, u"Show invented neighbors instead of asking lldpd (testing)"_s});
    parser.addOption({u"page"_s, u"Open the settings at this page, such as profiles or interfaces (testing)"_s, u"name"_s});
    parser.addOption({u"screenshot"_s, u"Save a screenshot of the window and quit (testing)"_s, u"file"_s});
    parser.process(app);
    about.processCommandLine(&parser);

    // A second start (menu, autostart) shows the running instance instead
    std::unique_ptr<KDBusService> unique;
    if (!parser.isSet(u"screenshot"_s)) {
        unique = std::make_unique<KDBusService>(KDBusService::Unique);
    }

    Backend backend(parser.isSet(u"demo"_s));
    App application(&backend);
    SettingsEditor editor;

    QQmlApplicationEngine engine;
    KLocalization::setupLocalizedContext(&engine);
    engine.rootContext()->setContextProperty(u"backend"_s, &backend);
    engine.rootContext()->setContextProperty(u"app"_s, &application);
    engine.rootContext()->setContextProperty(u"editor"_s, &editor);
    engine.rootContext()->setContextProperty(u"startHidden"_s, parser.isSet(u"hidden"_s));
    engine.rootContext()->setContextProperty(u"startPage"_s, parser.value(u"page"_s));
    engine.loadFromModule("io.github.nvaert1986.lldpworkbench", "Main");
    if (engine.rootObjects().isEmpty()) {
        return 1;
    }
    auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst());
    backend.setWindow(window);

    if (unique) {
        QObject::connect(unique.get(), &KDBusService::activateRequested, window, [window] {
            window->show();
            // The Wayland activation token arrives with the request; without it KWin would
            // only flash the task bar entry
            KWindowSystem::updateStartupId(window);
            KWindowSystem::activateWindow(window);
        });
    }

    if (parser.isSet(u"screenshot"_s)) {
        const QString file = parser.value(u"screenshot"_s);
        // The settings window too, as file-2.png, when a page was asked for
        QTimer::singleShot(2500, window, [window, file] {
            window->grabWindow().save(file);
            int number = 2;
            for (QWindow *other : QGuiApplication::topLevelWindows()) {
                auto quick = qobject_cast<QQuickWindow *>(other);
                if (quick && quick != window && quick->isVisible()) {
                    quick->grabWindow().save(QString(file).replace(u".png"_s, u"-%1.png"_s.arg(number++)));
                }
            }
            QCoreApplication::quit();
        });
    }
    return app.exec();
}
