// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include <QSortFilterProxyModel>

#include <mtx/events/power_levels.hpp>

#include "CacheStructs.h"

class MemberListBackend : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(QString roomName READ roomName NOTIFY roomNameChanged)
    Q_PROPERTY(int memberCount READ memberCount NOTIFY memberCountChanged)
    Q_PROPERTY(QString avatarUrl READ avatarUrl NOTIFY avatarUrlChanged)
    Q_PROPERTY(QString roomId READ roomId NOTIFY roomIdChanged)
    Q_PROPERTY(int numUsersLoaded READ numUsersLoaded NOTIFY numUsersLoadedChanged)
    Q_PROPERTY(bool loadingMoreMembers READ loadingMoreMembers NOTIFY loadingMoreMembersChanged)

public:
    enum Roles
    {
        Mxid,
        DisplayName,
        AvatarUrl,
        Trustlevel,
        Powerlevel,
    };

    MemberListBackend(const QString &room_id, QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        Q_UNUSED(parent)
        return static_cast<int>(m_memberList.size());
    }
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    QString roomName() const { return QString::fromStdString(info_.name); }
    int memberCount() const { return info_.member_count; }
    QString avatarUrl() const { return QString::fromStdString(info_.avatar_url); }
    QString roomId() const { return room_id_; }
    int numUsersLoaded() const { return numUsersLoaded_; }
    bool loadingMoreMembers() const { return loadingMoreMembers_; }

signals:
    void roomNameChanged();
    void memberCountChanged();
    void avatarUrlChanged();
    void roomIdChanged();
    void numUsersLoadedChanged();
    void loadingMoreMembersChanged();

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
    bool loadingMoreMembers_{false};

    mtx::events::state::PowerLevels powerLevels_;

    friend class MemberList;
};

class MemberList : public QSortFilterProxyModel
{
    Q_OBJECT

    Q_PROPERTY(QString roomName READ roomName NOTIFY roomNameChanged)
    Q_PROPERTY(int memberCount READ memberCount NOTIFY memberCountChanged)
    Q_PROPERTY(QString avatarUrl READ avatarUrl NOTIFY avatarUrlChanged)
    Q_PROPERTY(QString roomId READ roomId NOTIFY roomIdChanged)
    Q_PROPERTY(int numUsersLoaded READ numUsersLoaded NOTIFY numUsersLoadedChanged)
    Q_PROPERTY(bool loadingMoreMembers READ loadingMoreMembers NOTIFY loadingMoreMembersChanged)

public:
    enum MemberSortRoles
    {
        Mxid        = MemberListBackend::Roles::Mxid,
        DisplayName = MemberListBackend::Roles::DisplayName,
        Powerlevel  = MemberListBackend::Roles::Powerlevel,
    };
    Q_ENUM(MemberSortRoles)

    MemberList(const QString &room_id, QObject *parent = nullptr);

    QString roomName() const { return m_model.roomName(); }
    int memberCount() const { return m_model.memberCount(); }
    QString avatarUrl() const { return m_model.avatarUrl(); }
    QString roomId() const { return m_model.roomId(); }
    int numUsersLoaded() const { return m_model.numUsersLoaded(); }
    bool loadingMoreMembers() const { return m_model.loadingMoreMembers(); }

signals:
    void roomNameChanged();
    void memberCountChanged();
    void avatarUrlChanged();
    void roomIdChanged();
    void numUsersLoadedChanged();
    void loadingMoreMembersChanged();

public slots:
    void setFilterString(const QString &text);
    void sortBy(const MemberSortRoles role);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

private:
    QString filterString;
    MemberListBackend m_model;
};
