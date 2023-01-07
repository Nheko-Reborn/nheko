// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-FileCopyrightText: 2023 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "RoomsModel.h"

#include <QUrl>

#include "Cache.h"
#include "Cache_p.h"
#include "CompletionModelRoles.h"
#include "UserSettingsPage.h"
#include "Utils.h"

RoomsModel::RoomsModel(bool showOnlyRoomWithAliases, QObject *parent)
  : QAbstractListModel(parent)
  , showOnlyRoomWithAliases_(showOnlyRoomWithAliases)
{
    rooms = cache::client()->roomNamesAndAliases();

    if (showOnlyRoomWithAliases_)
        utils::erase_if(rooms, [](auto &r) { return r.alias.empty(); });
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
            auto alias = QString::fromStdString(rooms[index.row()].alias);
            if (UserSettings::instance()->markdown()) {
                QString percentEncoding = QUrl::toPercentEncoding(alias);
                return QStringLiteral("[%1](https://matrix.to/#/%2)")
                  .arg(alias.replace("[", "\\[").replace("]", "\\]").toHtmlEscaped(),
                       percentEncoding);
            } else {
                return alias;
            }
        }
        case CompletionModel::SearchRole:
        case Qt::DisplayRole:
        case Roles::RoomAlias:
            return QString::fromStdString(rooms[index.row()].alias).toHtmlEscaped();
        case CompletionModel::SearchRole2:
        case Roles::RoomName:
            return QString::fromStdString(rooms[index.row()].name);
        case Roles::AvatarUrl:
            return QString::fromStdString(
              cache::client()->singleRoomInfo(rooms[index.row()].id).avatar_url);
        case Roles::RoomID:
            return QString::fromStdString(rooms[index.row()].id).toHtmlEscaped();
        }
    }
    return {};
}
