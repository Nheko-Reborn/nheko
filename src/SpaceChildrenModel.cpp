// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "SpaceChildrenModel.h"

#include "Cache.h"
#include "Cache_p.h"

SpaceChildrenModel::SpaceChildrenModel(QSharedPointer<TimelineModel> space, QObject *parent)
  : QAbstractListModel{parent}
  , m_space{space}
{
    auto joinedRooms = cache::joinedRooms();
    for (const auto &child : cache::client()->getChildRoomIds(m_space->roomId().toStdString())) {
        m_childIds.push_back(QString::fromStdString(child));

        if (std::find(std::begin(joinedRooms), std::end(joinedRooms), child) !=
            std::end(joinedRooms))
            m_childInfos.push_back(cache::singleRoomInfo(child));
        else // TODO: replace with code to fetch a non-joined room
            m_childInfos.push_back(cache::singleRoomInfo(child));
    }
}

QHash<int, QByteArray>
SpaceChildrenModel::roleNames() const
{
    return {{Roles::Id, "id"},
            {Roles::RoomName, "roomName"},
            {Roles::AvatarUrl, "avatarUrl"},
            {Roles::Alias, "alias"}};
}

QVariant
SpaceChildrenModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= (int)m_childIds.size() || index.row() < 0)
        return {};

    switch (role) {
    case Roles::Id:
        return m_childIds[index.row()];
    case Roles::RoomName:
        return QString::fromStdString(m_childInfos[index.row()].name);
    case Roles::AvatarUrl:
        return QString::fromStdString(m_childInfos[index.row()].avatar_url);
    case Roles::Alias: {
        auto aliases = cache::client()->getRoomAliases(m_childIds[index.row()].toStdString());
        if (aliases.has_value())
            return QString::fromStdString(aliases->alias);
        else
            return {};
    }
    default:
        return {};
    }
}

NonSpaceChildrenModel::NonSpaceChildrenModel(QSharedPointer<TimelineModel> space, QObject *parent)
  : QAbstractListModel{parent}
  , m_space{space}
{
    auto children = cache::client()->getChildRoomIds(m_space->roomId().toStdString());
    for (const auto &room : cache::joinedRooms()) {
        if (room == space->roomId().toStdString() ||
            std::find(std::begin(children), std::end(children), room) == std::end(children)) {
            m_roomIds.push_back(QString::fromStdString(room));
            m_roomInfos.push_back(cache::singleRoomInfo(room));
        }
    }
}

QHash<int, QByteArray>
NonSpaceChildrenModel::roleNames() const
{
    return {{Roles::Id, "id"},
            {Roles::RoomName, "roomName"},
            {Roles::AvatarUrl, "avatarUrl"},
            {Roles::Alias, "alias"}};
}

QVariant
NonSpaceChildrenModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= (int)m_roomIds.size() || index.row() < 0)
        return {};

    switch (role) {
    case Roles::Id:
        return m_roomIds[index.row()];
    case Roles::RoomName:
        return QString::fromStdString(m_roomInfos[index.row()].name);
    case Roles::AvatarUrl:
        return QString::fromStdString(m_roomInfos[index.row()].avatar_url);
    case Roles::Alias: {
        auto aliases = cache::client()->getRoomAliases(m_roomIds[index.row()].toStdString());
        if (aliases.has_value())
            return QString::fromStdString(aliases->alias);
        else
            return {};
    }
    default:
        return {};
    }
}
