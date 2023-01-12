// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-FileCopyrightText: 2023 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "UserDirectoryModel.h"

#include "Cache.h"
#include "Logging.h"
#include "MatrixClient.h"
#include "mtx/responses/users.hpp"

UserDirectoryModel::UserDirectoryModel(QObject *parent)
  : QAbstractListModel{parent}
{
    connect(this,
            &UserDirectoryModel::fetchedSearchResults,
            this,
            &UserDirectoryModel::displaySearchResults,
            Qt::QueuedConnection);
}

QHash<int, QByteArray>
UserDirectoryModel::roleNames() const
{
    return {
      {Roles::DisplayName, "displayName"},
      {Roles::Mxid, "userid"},
      {Roles::AvatarUrl, "avatarUrl"},
    };
}

void
UserDirectoryModel::setSearchString(const QString &f)
{
    if (f == "") {
        nhlog::ui()->debug("Rejecting empty search string");
        return;
    }
    userSearchString_ = f.toStdString();
    nhlog::ui()->debug("Received user directory query: {}", userSearchString_);
    beginResetModel();
    results_.clear();
    canFetchMore_ = true;
    endResetModel();
}

void
UserDirectoryModel::fetchMore(const QModelIndex &)
{
    if (!canFetchMore_)
        return;

    nhlog::net()->debug("Fetching users from mtxclient...");
    searchingUsers_ = true;
    emit searchingUsersChanged();
    http::client()->search_user_directory(
      userSearchString_,
      [this](const mtx::responses::Users &res, mtx::http::RequestErr err) {
          searchingUsers_ = false;
          emit searchingUsersChanged();

          if (err) {
              nhlog::net()->error("Failed to retrieve users from mtxclient - {} - {} - {}",
                                  mtx::errors::to_string(err->matrix_error.errcode),
                                  err->matrix_error.error,
                                  err->parse_error);
          } else {
              emit fetchedSearchResults(res.results);
          }
      },
      -1);
}

QVariant
UserDirectoryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= (int)results_.size() || index.row() < 0)
        return {};
    switch (role) {
    case Roles::DisplayName:
        return QString::fromStdString(results_[index.row()].display_name);
    case Roles::Mxid:
        return QString::fromStdString(results_[index.row()].user_id);
    case Roles::AvatarUrl:
        return QString::fromStdString(results_[index.row()].avatar_url);
    }
    return {};
}

void
UserDirectoryModel::displaySearchResults(std::vector<mtx::responses::User> results)
{
    if (results.empty()) {
        nhlog::net()->error("mtxclient helper thread yielded no results!");
        return;
    }
    beginInsertRows(QModelIndex(), 0, static_cast<int>(results_.size()) - 1);
    results_ = results;
    endInsertRows();
    canFetchMore_ = false;
}
