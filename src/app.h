// SPDX-FileCopyrightText: 2026 plasma-lldp-workbench contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <AbstractKirigamiApplication>

namespace LldpWorkbench
{
class Backend;

// The menu bar, toolbar and command bar actions, with configurable shortcuts. Actions that
// only touch the data run here; the window handles the ones that open or close something.
class App : public AbstractKirigamiApplication
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by the application")

public:
    explicit App(Backend *backend, QObject *parent = nullptr);

Q_SIGNALS:
    void copiedAll();
    void closeWindowRequested();
    void configureSendingRequested();

protected:
    void setupActions() override;

private:
    Backend *m_backend;
};
}
