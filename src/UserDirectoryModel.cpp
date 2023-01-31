// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-FileCopyrightText: 2023 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "UserDirectoryModel.h"

#include <QSharedPointer>

#include <mtx/responses/users.hpp>

#include "Cache.h"
#include "Logging.h"
#include "MatrixClient.h"

UserDirectoryModel::UserDirectoryModel(QObject *parent)
  : QAbstractListModel{parent}
{
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
    userSearchString_ = f.toStdString();
    nhlog::ui()->debug("Received user directory query: {}", userSearchString_);
    beginResetModel();
    results_.clear();
    if (userSearchString_ == "")
        nhlog::ui()->debug("Rejecting empty search string");
    else
        canFetchMore_ = true;
    endResetModel();
}

void
UserDirectoryModel::fetchMore(const QModelIndex &)
{
    if (!canFetchMore_)
        return;

    nhlog::net()->debug("Fetching users from mtxclient...");
    std::string searchTerm = userSearchString_;
    searchingUsers_        = true;
    emit searchingUsersChanged();
    auto job = QSharedPointer<FetchUsersFromDirectoryJob>::create();
    connect(job.data(),
            &FetchUsersFromDirectoryJob::fetchedSearchResults,
            this,
            &UserDirectoryModel::displaySearchResults);
    http::client()->search_user_directory(
      searchTerm,
      [job, searchTerm](const mtx::responses::Users &res, mtx::http::RequestErr err) {
          if (err) {
              nhlog::net()->error("Failed to retrieve users from mtxclient - {} - {} - {}",
                                  mtx::errors::to_string(err->matrix_error.errcode),
                                  err->matrix_error.error,
                                  err->parse_error);
          } else {
              emit job->fetchedSearchResults(res.results, searchTerm);
          }
      },
      50);
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
UserDirectoryModel::displaySearchResults(std::vector<mtx::responses::User> results,
                                         const std::string &searchTerm)
{
    if (searchTerm != this->userSearchString_)
        return;
    searchingUsers_ = false;
    emit searchingUsersChanged();
    if (results.empty()) {
        nhlog::net()->debug("mtxclient helper thread yielded no results!");
        return;
    }
    beginInsertRows(QModelIndex(), 0, static_cast<int>(results.size()) - 1);
    results_ = results;
    endInsertRows();
    canFetchMore_ = false;
}
