// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "RoomsModel.h"

#include <QUrl>

#include "Cache_p.h"
#include "CompletionModelRoles.h"
#include "UserSettingsPage.h"

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
        auto roomAliasesList =
          cache::client()->getStateEvent<mtx::events::state::CanonicalAlias>(r);

        if (showOnlyRoomWithAliases_) {
            if (roomAliasesList && !roomAliasesList->content.alias.empty()) {
                roomids.push_back(QString::fromStdString(r));
                roomAliases.push_back(QString::fromStdString(roomAliasesList->content.alias));
            }
        } else {
            roomids.push_back(QString::fromStdString(r));
            roomAliases.push_back(roomAliasesList
                                    ? QString::fromStdString(roomAliasesList->content.alias)
                                    : QLatin1String(""));
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
            if (UserSettings::instance()->markdown()) {
                QString percentEncoding = QUrl::toPercentEncoding(roomAliases[index.row()]);
                return QStringLiteral("[%1](https://matrix.to/#/%2)")
                  .arg(QString(roomAliases[index.row()])
                         .replace("[", "\\[")
                         .replace("]", "\\]")
                         .toHtmlEscaped(),
                       percentEncoding);
            } else {
                return roomAliases[index.row()];
            }
        }
        case CompletionModel::SearchRole:
        case Qt::DisplayRole:
        case Roles::RoomAlias:
            return roomAliases[index.row()].toHtmlEscaped();
        case CompletionModel::SearchRole2:
        case Roles::RoomName:
            return QString::fromStdString(roomInfos.at(roomids[index.row()]).name).toHtmlEscaped();
        case Roles::AvatarUrl:
            return QString::fromStdString(roomInfos.at(roomids[index.row()]).avatar_url);
        case Roles::RoomID:
            return roomids[index.row()].toHtmlEscaped();
        }
    }
    return {};
}
