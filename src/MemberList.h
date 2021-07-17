// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "CacheStructs.h"
#include <QAbstractListModel>

class MemberList : public QAbstractListModel
{
        Q_OBJECT

        Q_PROPERTY(QString roomName READ roomName NOTIFY roomNameChanged)
        Q_PROPERTY(size_t memberCount READ memberCount NOTIFY memberCountChanged)
        Q_PROPERTY(QString avatarUrl READ avatarUrl NOTIFY avatarUrlChanged)
        Q_PROPERTY(QString roomId READ roomId NOTIFY roomIdChanged)
        Q_PROPERTY(int numUsersLoaded READ numUsersLoaded NOTIFY numUsersLoadedChanged)

public:
        enum Roles
        {
                Mxid,
                DisplayName,
                AvatarUrl,
        };
        MemberList(const QString &room_id, QWidget *parent = nullptr);

        QHash<int, QByteArray> roleNames() const override;
        int rowCount(const QModelIndex &parent = QModelIndex()) const override
        {
                Q_UNUSED(parent)
                return static_cast<int>(m_memberList.size());
        }
        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

        QString roomName() const { return QString::fromStdString(info_.name); }
        size_t memberCount() const { return info_.member_count; }
        QString avatarUrl() const { return QString::fromStdString(info_.avatar_url); }
        QString roomId() const { return room_id_; }
        int numUsersLoaded() const { return numUsersLoaded_; }

signals:
        void roomNameChanged();
        void memberCountChanged();
        void avatarUrlChanged();
        void roomIdChanged();
        void numUsersLoadedChanged();

public slots:
        void addUsers(const std::vector<RoomMember> &users);

protected:
        bool canFetchMore(const QModelIndex &) const override;
        void fetchMore(const QModelIndex &) override;

private:
        QVector<QPair<RoomMember, QString>> m_memberList;
        QString room_id_;
        RoomInfo info_;
        int numUsersLoaded_{0};
};
