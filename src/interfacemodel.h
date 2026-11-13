// SPDX-FileCopyrightText: 2026 plasma-lldp-workbench contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "neighbors.h"

#include <QAbstractListModel>
#include <QtQmlIntegration/qqmlintegration.h>

namespace LldpWorkbench
{
// One row per interface lldpd manages; interfaces that hear a neighbor come first
class InterfaceModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ANONYMOUS

public:
    enum Role {
        NameRole = Qt::UserRole + 1,
        StatusRole,
        StatusTextRole,
        SummaryRole,
        NeighborsRole,
        NeighborCountRole,
    };

    using QAbstractListModel::QAbstractListModel;

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // All ports, also the ones the view hides
    void setPorts(QList<LocalPort> ports);
    const QList<LocalPort> &ports() const;

    void setShowIdle(bool show);

private:
    void updateRows();

    QList<LocalPort> m_ports;
    QList<LocalPort> m_rows;
    bool m_showIdle = true;
};
}
