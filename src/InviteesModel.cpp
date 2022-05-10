// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "InviteesModel.h"

#include "Cache.h"
#include "Logging.h"
#include "MatrixClient.h"
#include "mtx/responses/profile.hpp"

InviteesModel::InviteesModel(QObject *parent)
  : QAbstractListModel{parent}
{}

void
InviteesModel::addUser(QString mxid)
{
    for (const auto &invitee : qAsConst(invitees_))
        if (invitee->mxid_ == mxid)
            return;

    beginInsertRows(QModelIndex(), invitees_.count(), invitees_.count());

    auto invitee        = new Invitee{mxid, this};
    auto indexOfInvitee = invitees_.count();
    connect(invitee, &Invitee::userInfoLoaded, this, [this, indexOfInvitee]() {
        emit dataChanged(index(indexOfInvitee), index(indexOfInvitee));
    });

    invitees_.push_back(invitee);

    endInsertRows();
    emit countChanged();
}

void
InviteesModel::removeUser(QString mxid)
{
    for (int i = 0; i < invitees_.length(); ++i) {
        if (invitees_[i]->mxid_ == mxid) {
            beginRemoveRows(QModelIndex(), i, i);
            invitees_.removeAt(i);
            endRemoveRows();
            emit countChanged();
            break;
        }
    }
}

QHash<int, QByteArray>
InviteesModel::roleNames() const
{
    return {{Mxid, "mxid"}, {DisplayName, "displayName"}, {AvatarUrl, "avatarUrl"}};
}

QVariant
InviteesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= (int)invitees_.size() || index.row() < 0)
        return {};

    switch (role) {
    case Mxid:
        return invitees_[index.row()]->mxid_;
    case DisplayName:
        return invitees_[index.row()]->displayName_;
    case AvatarUrl:
        return invitees_[index.row()]->avatarUrl_;
    default:
        return {};
    }
}

QStringList
InviteesModel::mxids()
{
    QStringList mxidList;
    mxidList.reserve(invitees_.size());
    for (auto &invitee : qAsConst(invitees_))
        mxidList.push_back(invitee->mxid_);
    return mxidList;
}

Invitee::Invitee(QString mxid, QObject *parent)
  : QObject{parent}
  , mxid_{std::move(mxid)}
{
    http::client()->get_profile(
      mxid_.toStdString(), [this](const mtx::responses::Profile &res, mtx::http::RequestErr err) {
          if (err) {
              nhlog::net()->warn("failed to retrieve profile info");
              emit userInfoLoaded();
              return;
          }

          displayName_ = QString::fromStdString(res.display_name);
          avatarUrl_   = QString::fromStdString(res.avatar_url);

          emit userInfoLoaded();
      });
}
