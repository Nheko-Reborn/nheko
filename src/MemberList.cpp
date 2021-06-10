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
  : QObject{parent}
  , room_id_{room_id}
{
        try {
                info_ = cache::singleRoomInfo(room_id_.toStdString());
        } catch (const lmdb::error &) {
                nhlog::db()->warn("failed to retrieve room info from cache: {}",
                                  room_id_.toStdString());
        }

        try {
                addUsers(cache::getMembers(room_id_.toStdString()));
        } catch (const lmdb::error &e) {
                nhlog::db()->critical("Failed to retrieve members from cache: {}", e.what());
        }
}

void
MemberList::loadMoreMembers()
{
        const size_t numMembers = m_model.rowCount() - 1;
        if (numMembers > 0)
                addUsers(cache::getMembers(room_id_.toStdString(), numMembers));
}

void
MemberList::addUsers(const std::vector<RoomMember> &members)
{
        m_model.beginInsertRows(QModelIndex{}, m_model.m_memberList.count(), m_model.m_memberList.count() + members.size() - 1);

        for (const auto &member : members)
                m_model.m_memberList.push_back(
                  {member,
                   ChatPage::instance()->timelineManager()->rooms()->currentRoom()->avatarUrl(
                     member.user_id)});

        m_model.endInsertRows();
}

QHash<int, QByteArray>
MemberListModel::roleNames() const
{
        return {{Mxid, "mxid"}, {DisplayName, "displayName"}, {AvatarUrl, "avatarUrl"}};
}

QVariant
MemberListModel::data(const QModelIndex &index, int role) const
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
