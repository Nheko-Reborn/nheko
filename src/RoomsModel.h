// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "CacheStructs.h"

#include <QAbstractListModel>
#include <QString>

class RoomsModel final : public QAbstractListModel
{
public:
    enum Roles
    {
        AvatarUrl = Qt::UserRole,
        RoomAlias,
        RoomID,
        RoomName,
        IsTombstoned,
    };

    RoomsModel(bool showOnlyRoomWithAliases = false, QObject *parent = nullptr);
    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        (void)parent;
        return (int)rooms.size();
    }
    QVariant data(const QModelIndex &index, int role) const override;

private:
    std::vector<RoomNameAlias> rooms;
    bool showOnlyRoomWithAliases_;
};
