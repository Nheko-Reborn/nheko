// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QAbstractSlider>
#include <QLabel>
#include <QListWidgetItem>
#include <QPainter>
#include <QPushButton>
#include <QScrollBar>
#include <QShortcut>
#include <QStyleOption>
#include <QVBoxLayout>

#include "MemberList.h"

#include "Cache.h"
#include "ChatPage.h"
#include "Config.h"
#include "Logging.h"
#include "Utils.h"
#include "timeline/TimelineViewManager.h"
#include "ui/Avatar.h"

MemberList::MemberList(const QString &room_id, QWidget *parent)
  : QAbstractListModel{parent}
  , room_id_{room_id}
{
        try {
                info_ = cache::singleRoomInfo(room_id_.toStdString());
        } catch (const lmdb::error &) {
                nhlog::db()->warn("failed to retrieve room info from cache: {}",
                                  room_id_.toStdString());
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
        beginInsertRows(
          QModelIndex{}, m_memberList.count(), m_memberList.count() + members.size() - 1);

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
        return {{Mxid, "mxid"}, {DisplayName, "displayName"}, {AvatarUrl, "avatarUrl"}};
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
        auto members = cache::getMembers(room_id_.toStdString(), rowCount());
        addUsers(members);
        numUsersLoaded_ += members.size();
        emit numUsersLoadedChanged();
}
