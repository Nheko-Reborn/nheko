// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "CacheStructs.h"

#include <QAbstractListModel>
#include <QString>

class RoomlistModel;

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
        IsSpace,
        RoomParent,
    };

    RoomsModel(RoomlistModel &roomListModel, bool showOnlyRoomWithAliases = false, QObject *parent = nullptr);
    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        (void)parent;
        return (int)rooms.size();
    }
    QVariant data(const QModelIndex &index, int role) const override;

private:
    RoomlistModel &roomListModel_;
    std::vector<RoomNameAlias> rooms;
    bool showOnlyRoomWithAliases_;
};
