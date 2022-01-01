// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "Cache.h"

#include <QAbstractListModel>
#include <QString>

class RoomsModel : public QAbstractListModel
{
public:
    enum Roles
    {
        AvatarUrl = Qt::UserRole,
        RoomAlias,
        RoomID,
        RoomName,
    };

    RoomsModel(bool showOnlyRoomWithAliases = false, QObject *parent = nullptr);
    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        (void)parent;
        return (int)roomids.size();
    }
    QVariant data(const QModelIndex &index, int role) const override;

private:
    std::vector<QString> roomids;
    std::vector<QString> roomAliases;
    std::map<QString, RoomInfo> roomInfos;
    bool showOnlyRoomWithAliases_;
};
