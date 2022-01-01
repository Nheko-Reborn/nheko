// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "MemberList.h"

#include "Cache.h"
#include "ChatPage.h"
#include "Config.h"
#include "Logging.h"
#include "Utils.h"
#include "timeline/TimelineViewManager.h"

MemberList::MemberList(const QString &room_id, QObject *parent)
  : QAbstractListModel{parent}
  , room_id_{room_id}
{
    try {
        info_ = cache::singleRoomInfo(room_id_.toStdString());
    } catch (const lmdb::error &) {
        nhlog::db()->warn("failed to retrieve room info from cache: {}", room_id_.toStdString());
    }

    try {
        auto members = cache::getMembers(room_id_.toStdString());
        addUsers(members);
        numUsersLoaded_ = members.size();
    } catch (const lmdb::error &e) {
        nhlog::db()->critical("Failed to retrieve members from cache: {}", e.what());
    }
}

void
MemberList::addUsers(const std::vector<RoomMember> &members)
{
    beginInsertRows(QModelIndex{}, m_memberList.count(), m_memberList.count() + members.size() - 1);

    for (const auto &member : members)
        m_memberList.push_back(
          {member,
           ChatPage::instance()->timelineManager()->rooms()->currentRoom()->avatarUrl(
             member.user_id)});

    endInsertRows();
}

QHash<int, QByteArray>
MemberList::roleNames() const
{
    return {
      {Mxid, "mxid"},
      {DisplayName, "displayName"},
      {AvatarUrl, "avatarUrl"},
      {Trustlevel, "trustlevel"},
    };
}

QVariant
MemberList::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= (int)m_memberList.size() || index.row() < 0)
        return {};

    switch (role) {
    case Mxid:
        return m_memberList[index.row()].first.user_id;
    case DisplayName:
        return m_memberList[index.row()].first.display_name;
    case AvatarUrl:
        return m_memberList[index.row()].second;
    case Trustlevel: {
        auto stat =
          cache::verificationStatus(m_memberList[index.row()].first.user_id.toStdString());

        if (!stat)
            return crypto::Unverified;
        if (stat->unverified_device_count)
            return crypto::Unverified;
        else
            return stat->user_verified;
    }
    default:
        return {};
    }
}

bool
MemberList::canFetchMore(const QModelIndex &) const
{
    const size_t numMembers = rowCount();
    if (numMembers > 1 && numMembers < info_.member_count)
        return true;
    else
        return false;
}

void
MemberList::fetchMore(const QModelIndex &)
{
    loadingMoreMembers_ = true;
    emit loadingMoreMembersChanged();

    auto members = cache::getMembers(room_id_.toStdString(), rowCount());
    addUsers(members);
    numUsersLoaded_ += members.size();
    emit numUsersLoadedChanged();

    loadingMoreMembers_ = false;
    emit loadingMoreMembersChanged();
}
