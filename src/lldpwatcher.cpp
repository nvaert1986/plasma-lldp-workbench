// SPDX-FileCopyrightText: 2026 plasma-lldp-workbench contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "lldpwatcher.h"

#include <lldpctl.h>

namespace LldpWorkbench
{
namespace
{
void changed(lldpctl_change_t, lldpctl_atom_t *, lldpctl_atom_t *, void *data)
{
    Q_EMIT static_cast<LldpWatcher *>(data)->neighborsChanged();
}
}

LldpWatcher::~LldpWatcher()
{
    stop();
}

void LldpWatcher::stop()
{
    m_stopping = true;
    // An unblock that lands just before the thread starts waiting is lost, so it repeats
    for (int attempt = 0; attempt < 50 && isRunning(); ++attempt) {
        {
            QMutexLocker locker(&m_lock);
#ifdef HAVE_LLDPCTL_WATCH_UNBLOCK
            if (m_connection) {
                lldpctl_watch_sync_unblock(m_connection);
            }
#endif
        }
        wait(100);
    }
    wait();
}

void LldpWatcher::run()
{
    m_stopping = false;
    lldpctl_conn_t *connection = lldpctl_new(nullptr, nullptr, nullptr);
    if (!connection) {
        return;
    }
    {
        QMutexLocker locker(&m_lock);
        m_connection = connection;
    }
    // Ends when lldpd goes away or stop() unblocks the wait; the owner restarts it later
    if (lldpctl_watch_callback2(connection, changed, this) == 0) {
        while (!m_stopping && lldpctl_watch(connection) == 0) {
        }
    }
    {
        QMutexLocker locker(&m_lock);
        m_connection = nullptr;
    }
    lldpctl_release(connection);
}
}
