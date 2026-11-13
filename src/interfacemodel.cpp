// SPDX-FileCopyrightText: 2026 plasma-lldp-workbench contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "interfacemodel.h"

#include <algorithm>

using namespace Qt::StringLiterals;

namespace LldpWorkbench
{
int InterfaceModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_rows.size();
}

QVariant InterfaceModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid)) {
        return {};
    }
    const LocalPort &port = m_rows.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
    case NameRole:
        return port.interfaceName;
    case StatusRole:
        return port.status;
    case StatusTextRole:
        return portStatusText(port.status);
    case SummaryRole:
        return summary(port);
    case NeighborsRole: {
        QVariantList neighbors;
        for (const Neighbor &neighbor : port.neighbors) {
            neighbors << toVariant(neighbor);
        }
        return neighbors;
    }
    case NeighborCountRole:
        return port.neighbors.size();
    }
    return {};
}

QHash<int, QByteArray> InterfaceModel::roleNames() const
{
    return {
        {NameRole, "name"},
        {StatusRole, "status"},
        {StatusTextRole, "statusText"},
        {SummaryRole, "summary"},
        {NeighborsRole, "neighbors"},
        {NeighborCountRole, "neighborCount"},
    };
}

void InterfaceModel::setPorts(QList<LocalPort> ports)
{
    std::stable_sort(ports.begin(), ports.end(), [](const LocalPort &a, const LocalPort &b) {
        if (a.neighbors.isEmpty() != b.neighbors.isEmpty()) {
            return !a.neighbors.isEmpty();
        }
        return a.interfaceName < b.interfaceName;
    });
    m_ports = std::move(ports);
    updateRows();
}

void InterfaceModel::setShowIdle(bool show)
{
    m_showIdle = show;
    updateRows();
}

void InterfaceModel::updateRows()
{
    QList<LocalPort> rows;
    for (const LocalPort &port : std::as_const(m_ports)) {
        if (m_showIdle || !port.neighbors.isEmpty()) {
            rows << port;
        }
    }
    // lldpd reports every refresh of a neighbor; only a real difference resets the view
    if (rows == m_rows) {
        return;
    }
    beginResetModel();
    m_rows = std::move(rows);
    endResetModel();
}

const QList<LocalPort> &InterfaceModel::ports() const
{
    return m_ports;
}
}
