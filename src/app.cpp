// SPDX-FileCopyrightText: 2026 plasma-lldp-workbench contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "app.h"

#include "backend.h"

#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>
#include <KStandardActions>

#include <QAction>

using namespace Qt::StringLiterals;

namespace LldpWorkbench
{
namespace
{
KConfigGroup viewState()
{
    return KSharedConfig::openStateConfig()->group(u"View"_s);
}
}

App::App(Backend *backend, QObject *parent)
    : AbstractKirigamiApplication(parent)
    , m_backend(backend)
{
    setupActions();
}

void App::setupActions()
{
    AbstractKirigamiApplication::setupActions();
    KirigamiActionCollection *collection = mainCollection();

    QAction *copyAll = collection->addAction(u"copy_all"_s, this, [this] {
        m_backend->copyAll();
        Q_EMIT copiedAll();
    });
    copyAll->setText(i18nc("@action", "&Copy All Neighbors"));
    copyAll->setIcon(QIcon::fromTheme(u"edit-copy"_s));
    KirigamiActionCollection::setDefaultShortcut(copyAll, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_C));

    QAction *close = collection->addAction(KStandardActions::Close, u"file_close"_s, this, &App::closeWindowRequested);
    close->setText(i18nc("@action", "C&lose Window"));

    QAction *refresh = collection->addAction(KStandardActions::Redisplay, u"view_redisplay"_s, m_backend, &Backend::refresh);
    refresh->setText(i18nc("@action", "&Refresh"));

    QAction *showIdle = collection->addAction(u"show_idle_interfaces"_s, new QAction(this));
    showIdle->setText(i18nc("@action", "Show Interfaces &Without Neighbors"));
    showIdle->setCheckable(true);
    showIdle->setChecked(viewState().readEntry("ShowIdleInterfaces", true));
    m_backend->interfaces()->setShowIdle(showIdle->isChecked());
    connect(showIdle, &QAction::toggled, this, [this](bool checked) {
        m_backend->interfaces()->setShowIdle(checked);
        KConfigGroup group = viewState();
        group.writeEntry("ShowIdleInterfaces", checked);
        group.sync();
    });

    QAction *configure = collection->addAction(KStandardActions::Preferences, u"options_configure"_s, this, &App::configureSendingRequested);
    configure->setText(i18nc("@action", "Configure &LLDP…"));

    QAction *autostart = collection->addAction(u"start_at_login"_s, new QAction(this));
    autostart->setText(i18nc("@action", "Start at &Login"));
    autostart->setCheckable(true);
    autostart->setChecked(m_backend->autostart());
    connect(autostart, &QAction::toggled, m_backend, &Backend::setAutostart);

    readSettings();
}
}
