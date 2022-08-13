// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "MemberList.h"

#include "Cache.h"
#include "Cache_p.h"
#include "ChatPage.h"
#include "Config.h"
#include "Logging.h"
#include "Utils.h"
#include "timeline/TimelineViewManager.h"

MemberListBackend::MemberListBackend(const QString &room_id, QObject *parent)
  : QAbstractListModel{parent}
  , room_id_{room_id}
  , powerLevels_{cache::client()
                   ->getStateEvent<mtx::events::state::PowerLevels>(room_id_.toStdString())
                   .value_or(mtx::events::StateEvent<mtx::events::state::PowerLevels>{})
                   .content}
{
    try {
        info_ = cache::singleRoomInfo(room_id_.toStdString());
    } catch (const lmdb::error &) {
        nhlog::db()->warn("failed to retrieve room info from cache: {}", room_id_.toStdString());
    }

    try {
        // HACK: due to QTBUG-1020169, we'll load a big chunk to speed things up
        auto members = cache::getMembers(room_id_.toStdString(), 0, -1);
        addUsers(members);
        numUsersLoaded_ = members.size();
    } catch (const lmdb::error &e) {
        nhlog::db()->critical("Failed to retrieve members from cache: {}", e.what());
    }
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
      {Powerlevel, "powerlevel"},
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
    case Powerlevel:
        return static_cast<qlonglong>(
          powerLevels_.user_level(m_memberList[index.row()].first.user_id.toStdString()));
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
    setSortRole(MemberSortRoles::Mxid);
    sort(0, Qt::AscendingOrder);
    setDynamicSortFilter(true);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
}

void
MemberList::setFilterString(const QString &text)
{
    filterString = text;
    setFilterFixedString(text);
}

void
MemberList::sortBy(const MemberSortRoles role)
{
    setSortRole(role);
    // Unfortunately, Qt doesn't provide a "setSortOrder" function.
    sort(0, role == MemberSortRoles::Powerlevel ? Qt::DescendingOrder : Qt::AscendingOrder);
}

bool
MemberList::filterAcceptsRow(int source_row, const QModelIndex &) const
{
    return m_model.m_memberList[source_row].first.user_id.contains(filterString,
                                                                   Qt::CaseInsensitive) ||
           m_model.m_memberList[source_row].first.display_name.contains(filterString,
                                                                        Qt::CaseInsensitive);
}
