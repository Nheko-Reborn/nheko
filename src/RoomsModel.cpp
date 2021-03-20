// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "RoomsModel.h"

#include <QUrl>

#include "Cache_p.h"
#include "CompletionModelRoles.h"

RoomsModel::RoomsModel(bool showOnlyRoomWithAliases, QObject *parent)
  : QAbstractListModel(parent)
  , showOnlyRoomWithAliases_(showOnlyRoomWithAliases)
{
        std::vector<std::string> rooms_ = cache::joinedRooms();
        roomInfos                       = cache::getRoomInfo(rooms_);
        if (!showOnlyRoomWithAliases_) {
                roomids.reserve(rooms_.size());
                roomAliases.reserve(rooms_.size());
        }

        for (const auto &r : rooms_) {
                auto roomAliasesList = cache::client()->getRoomAliases(r);

                if (showOnlyRoomWithAliases_) {
                        if (roomAliasesList && !roomAliasesList->alias.empty()) {
                                roomids.push_back(QString::fromStdString(r));
                                roomAliases.push_back(
                                  QString::fromStdString(roomAliasesList->alias));
                        }
                } else {
                        roomids.push_back(QString::fromStdString(r));
                        roomAliases.push_back(
                          roomAliasesList ? QString::fromStdString(roomAliasesList->alias) : "");
                }
        }
}

QHash<int, QByteArray>
RoomsModel::roleNames() const
{
        return {{CompletionModel::CompletionRole, "completionRole"},
                {CompletionModel::SearchRole, "searchRole"},
                {CompletionModel::SearchRole2, "searchRole2"},
                {Roles::RoomAlias, "roomAlias"},
                {Roles::AvatarUrl, "avatarUrl"},
                {Roles::RoomID, "roomid"},
                {Roles::RoomName, "roomName"}};
}

QVariant
RoomsModel::data(const QModelIndex &index, int role) const
{
        if (hasIndex(index.row(), index.column(), index.parent())) {
                switch (role) {
                case CompletionModel::CompletionRole: {
                        QString percentEncoding = QUrl::toPercentEncoding(roomAliases[index.row()]);
                        return QString("[%1](https://matrix.to/#/%2)")
                          .arg(roomAliases[index.row()], percentEncoding);
                }
                case CompletionModel::SearchRole:
                case Qt::DisplayRole:
                case Roles::RoomAlias:
                        return roomAliases[index.row()];
                case CompletionModel::SearchRole2:
                case Roles::RoomName:
                        return QString::fromStdString(roomInfos.at(roomids[index.row()]).name);
                case Roles::AvatarUrl:
                        return QString::fromStdString(
                          roomInfos.at(roomids[index.row()]).avatar_url);
                case Roles::RoomID:
                        return roomids[index.row()];
                }
        }
        return {};
}
