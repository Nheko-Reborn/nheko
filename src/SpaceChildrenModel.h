// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SPACECHILDRENMODEL_H
#define SPACECHILDRENMODEL_H

#include <QAbstractListModel>
#include <QSharedPointer>

#include "timeline/TimelineModel.h"

class SpaceChildrenModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(TimelineModel *space READ space CONSTANT)

public:
    enum Roles
    {
        Id,
        RoomName,
        AvatarUrl,
        Alias,
    };

    explicit SpaceChildrenModel(QSharedPointer<TimelineModel> space, QObject *parent = nullptr);

    TimelineModel *space() const { return m_space.data(); }

    int rowCount(const QModelIndex &parent) const override
    {
        Q_UNUSED(parent);
        return m_childIds.size();
    }
    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex &index, int role) const override;

private:
    QSharedPointer<TimelineModel> m_space;
    QStringList m_childIds;
    QList<RoomInfo> m_childInfos;
};

class NonSpaceChildrenModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(TimelineModel *space READ space CONSTANT)

public:
    enum Roles
    {
        Id,
        RoomName,
        AvatarUrl,
        Alias,
    };

    explicit NonSpaceChildrenModel(QSharedPointer<TimelineModel> space, QObject *parent = nullptr);

    TimelineModel *space() const { return m_space.data(); }

    int rowCount(const QModelIndex &parent) const override
    {
        Q_UNUSED(parent);
        return m_roomIds.size();
    }
    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex &index, int role) const override;

private:
    QSharedPointer<TimelineModel> m_space;
    QStringList m_roomIds;
    QList<RoomInfo> m_roomInfos;
};

#endif // SPACECHILDRENMODEL_H
