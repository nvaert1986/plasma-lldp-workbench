// SPDX-FileCopyrightText: 2026 plasma-lldp-workbench contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QMutex>
#include <QThread>

#include <atomic>

struct lldpctl_conn_t;

namespace LldpWorkbench
{
// lldpd pushes neighbor changes over a connection that blocks while it waits, so it gets a
// thread of its own. It only signals that something changed; the reading happens elsewhere.
class LldpWatcher : public QThread
{
    Q_OBJECT

public:
    using QThread::QThread;
    ~LldpWatcher() override;

    // Older lldpctl (1.0.18 in Debian 13 and Fedora 45) cannot interrupt a waiting watch, so
    // it is not started there
    static constexpr bool supported()
    {
#ifdef HAVE_LLDPCTL_WATCH_UNBLOCK
        return true;
#else
        return false;
#endif
    }

    void stop();

Q_SIGNALS:
    void neighborsChanged();

protected:
    void run() override;

private:
    QMutex m_lock;
    lldpctl_conn_t *m_connection = nullptr;
    std::atomic_bool m_stopping = false;
};
}
