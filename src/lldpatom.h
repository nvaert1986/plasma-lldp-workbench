// SPDX-FileCopyrightText: 2026 plasma-lldp-workbench contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QStringList>

#include <lldpctl.h>

#include <memory>

// Owning wrappers and typed getters around lldpctl's reference-counted atoms
namespace LldpWorkbench::Atoms
{
struct ConnectionRelease {
    void operator()(lldpctl_conn_t *connection) const
    {
        lldpctl_release(connection);
    }
};
using Connection = std::unique_ptr<lldpctl_conn_t, ConnectionRelease>;

struct AtomRelease {
    void operator()(lldpctl_atom_t *atom) const
    {
        lldpctl_atom_dec_ref(atom);
    }
};
using Atom = std::unique_ptr<lldpctl_atom_t, AtomRelease>;

inline Connection connect()
{
    // No callbacks: lldpctl then uses its own blocking socket I/O
    return Connection(lldpctl_new(nullptr, nullptr, nullptr));
}

// lldpd shows unset patterns as "(none)" in some versions
inline QString text(lldpctl_atom_t *atom, lldpctl_key_t key)
{
    const char *value = lldpctl_atom_get_str(atom, key);
    const QString result = value ? QString::fromUtf8(value) : QString();
    return result == QLatin1String("(none)") ? QString() : result;
}

// lldpctl returns a negative error code for values that are not set
inline int number(lldpctl_atom_t *atom, lldpctl_key_t key)
{
    const long value = lldpctl_atom_get_int(atom, key);
    return value < 0 ? 0 : static_cast<int>(value);
}

// Bytes in lldpcli's notation: "00,12,0F"
inline QString bytes(lldpctl_atom_t *atom, lldpctl_key_t key)
{
    size_t length = 0;
    const uint8_t *data = lldpctl_atom_get_buffer(atom, key, &length);
    QStringList parts;
    for (size_t i = 0; data && i < length; ++i) {
        parts << QString::number(data[i], 16).rightJustified(2, QLatin1Char('0')).toUpper();
    }
    return parts.join(QLatin1Char(','));
}

inline QString lastError(lldpctl_conn_t *connection)
{
    return QString::fromUtf8(lldpctl_strerror(lldpctl_last_error(connection)));
}
}
