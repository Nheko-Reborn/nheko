// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "UsersModel.h"

#include <QUrl>

#include "Cache.h"
#include "CompletionModelRoles.h"
#include "UserSettingsPage.h"

UsersModel::UsersModel(const std::string &roomId, QObject *parent)
  : QAbstractListModel(parent)
  , room_id(roomId)
{
        roomMembers_ = cache::roomMembers(roomId);
        for (const auto &m : roomMembers_) {
                displayNames.push_back(QString::fromStdString(cache::displayName(room_id, m)));
                userids.push_back(QString::fromStdString(m));
        }
}

QHash<int, QByteArray>
UsersModel::roleNames() const
{
        return {
          {CompletionModel::CompletionRole, "completionRole"},
          {CompletionModel::SearchRole, "searchRole"},
          {CompletionModel::SearchRole2, "searchRole2"},
          {Roles::DisplayName, "displayName"},
          {Roles::AvatarUrl, "avatarUrl"},
          {Roles::UserID, "userid"},
        };
}

QVariant
UsersModel::data(const QModelIndex &index, int role) const
{
        if (hasIndex(index.row(), index.column(), index.parent())) {
                switch (role) {
                case CompletionModel::CompletionRole:
                        if (UserSettings::instance()->markdown())
                                return QString("[%1](https://matrix.to/#/%2)")
                                  .arg(displayNames[index.row()])
                                  .arg(QString(QUrl::toPercentEncoding(userids[index.row()])));
                        else
                                return displayNames[index.row()];
                case CompletionModel::SearchRole:
                case Qt::DisplayRole:
                case Roles::DisplayName:
                        return displayNames[index.row()];
                case CompletionModel::SearchRole2:
                        return userids[index.row()];
                case Roles::AvatarUrl:
                        return cache::avatarUrl(QString::fromStdString(room_id),
                                                QString::fromStdString(roomMembers_[index.row()]));
                case Roles::UserID:
                        return userids[index.row()];
                }
        }
        return {};
}
