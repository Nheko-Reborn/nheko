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

MemberListBackend::MemberListBackend(const QString &room_id, QObject *parent)
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

    // HACK: until https://bugreports.qt.io/browse/QTBUG-102169 is fixed, we'll just load all of the
    // members to make searches work properly. If QTBUG-102169 has been fixed, please delete this
    // code.
    while (canFetchMore({}))
        fetchMore({});
}

void
MemberListBackend::addUsers(const std::vector<RoomMember> &members)
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
MemberListBackend::roleNames() const
{
    return {
      {Mxid, "mxid"},
      {DisplayName, "displayName"},
      {AvatarUrl, "avatarUrl"},
      {Trustlevel, "trustlevel"},
    };
}

QVariant
MemberListBackend::data(const QModelIndex &index, int role) const
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
MemberListBackend::canFetchMore(const QModelIndex &) const
{
    const size_t numMembers = rowCount();
    if (numMembers > 1 && numMembers < info_.member_count)
        return true;
    else
        return false;
}

void
MemberListBackend::fetchMore(const QModelIndex &)
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

MemberList::MemberList(const QString &room_id, QObject *parent)
  : QSortFilterProxyModel{parent}
  , m_model{room_id, this}
{
    connect(&m_model, &MemberListBackend::roomNameChanged, this, &MemberList::roomNameChanged);
    connect(
      &m_model, &MemberListBackend::memberCountChanged, this, &MemberList::memberCountChanged);
    connect(&m_model, &MemberListBackend::avatarUrlChanged, this, &MemberList::avatarUrlChanged);
    connect(&m_model, &MemberListBackend::roomIdChanged, this, &MemberList::roomIdChanged);
    connect(&m_model,
            &MemberListBackend::numUsersLoadedChanged,
            this,
            &MemberList::numUsersLoadedChanged);
    connect(&m_model,
            &MemberListBackend::loadingMoreMembersChanged,
            this,
            &MemberList::loadingMoreMembersChanged);

    setSourceModel(&m_model);
    setSortRole(MemberListBackend::Mxid);
    sort(0, Qt::AscendingOrder);
    setDynamicSortFilter(true);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
}

void
MemberList::setFilterString(const QString &text)
{
    setFilterRegExp(QRegExp::escape(text));
}

bool
MemberList::filterAcceptsRow(int source_row, const QModelIndex &) const
{
    return m_model.m_memberList[source_row].first.user_id.contains(filterRegExp()) ||
           m_model.m_memberList[source_row].first.display_name.contains(filterRegExp());
}
