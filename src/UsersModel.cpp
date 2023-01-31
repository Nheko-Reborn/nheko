// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-FileCopyrightText: 2023 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "UsersModel.h"

#include <QUrl>

#include "Cache.h"
#include "Cache_p.h"
#include "CompletionModelRoles.h"
#include "UserSettingsPage.h"

UsersModel::UsersModel(const std::string &roomId, QObject *parent)
  : QAbstractListModel(parent)
  , room_id(roomId)
{
    // obviously, "friends" isn't a room, but I felt this was the least invasive way
    if (roomId == "friends") {
        auto e = cache::client()->getAccountData(mtx::events::EventType::Direct);
        if (e) {
            if (auto event =
                  std::get_if<mtx::events::AccountDataEvent<mtx::events::account_data::Direct>>(
                    &e.value())) {
                for (const auto &[userId, roomIds] : event->content.user_to_rooms) {
                    displayNames.push_back(
                      QString::fromStdString(cache::displayName(roomIds[0], userId)));
                    userids.push_back(QString::fromStdString(userId));
                    avatarUrls.push_back(cache::avatarUrl(QString::fromStdString(roomIds[0]),
                                                          QString::fromStdString(userId)));
                }
            }
        }
    } else {
        const auto start_at = std::chrono::steady_clock::now();
        for (const auto &m : cache::getMembers(roomId, 0, -1)) {
            displayNames.push_back(m.display_name);
            userids.push_back(m.user_id);
            avatarUrls.push_back(m.avatar_url);
        }
        const auto end_at     = std::chrono::steady_clock::now();
        const auto build_time = std::chrono::duration<double, std::milli>(end_at - start_at);
        nhlog::ui()->debug("UsersModel: build data: {} ms", build_time.count());
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
                return QStringLiteral("[%1](https://matrix.to/#/%2)")
                  .arg(QString(displayNames[index.row()])
                         .replace("[", "\\[")
                         .replace("]", "\\]")
                         .toHtmlEscaped(),
                       QString(QUrl::toPercentEncoding(userids[index.row()])));
            else
                return displayNames[index.row()];
        case CompletionModel::SearchRole:
            return displayNames[index.row()];
        case Qt::DisplayRole:
        case Roles::DisplayName:
            return displayNames[index.row()].toHtmlEscaped();
        case CompletionModel::SearchRole2:
            return userids[index.row()];
        case Roles::AvatarUrl:
            return avatarUrls[index.row()];
        case Roles::UserID:
            return userids[index.row()].toHtmlEscaped();
        }
    }
    return {};
}
