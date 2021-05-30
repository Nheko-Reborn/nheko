// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "CacheStructs.h"
#include <QFrame>
#include <QListWidget>

class Avatar;
class QPushButton;
class QHBoxLayout;
class QLabel;
class QVBoxLayout;

class MemberListModel : public QAbstractListModel
{
        Q_OBJECT

public:
        enum Roles
        {
                Mxid,
                DisplayName,
                AvatarUrl,
        };

        QHash<int, QByteArray> roleNames() const override;
        int rowCount(const QModelIndex &parent = QModelIndex()) const override
        {
                Q_UNUSED(parent)
                return static_cast<int>(m_memberList.size());
        }
        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

private:
        // TODO: probably should move the avatar url into RoomMember at some point
        QVector<QPair<RoomMember, QString>> m_memberList;

        friend class MemberList;
};

template<class T>
class QSharedPointer;

class MemberList : public QObject
{
        Q_OBJECT

        Q_PROPERTY(QString roomName READ roomName NOTIFY roomNameChanged)
        Q_PROPERTY(MemberListModel *model READ model NOTIFY modelChanged)
        Q_PROPERTY(size_t memberCount READ memberCount NOTIFY memberCountChanged)
        Q_PROPERTY(QString avatarUrl READ avatarUrl NOTIFY avatarUrlChanged)
        Q_PROPERTY(QString roomId READ roomId NOTIFY roomIdChanged)

public:
        MemberList(const QString &room_id, QWidget *parent = nullptr);

        Q_INVOKABLE void loadMoreMembers();

        QString roomName() const { return QString::fromStdString(info_.name); }
        MemberListModel *model() { return &this->m_model; }
        size_t memberCount() const { return info_.member_count; }
        QString avatarUrl() const { return QString::fromStdString(info_.avatar_url); }
        QString roomId() const { return room_id_; }

signals:
        void roomNameChanged();
        void modelChanged();
        void memberCountChanged();
        void avatarUrlChanged();
        void roomIdChanged();

public slots:
        void addUsers(const std::vector<RoomMember> &users);

private:
        MemberListModel m_model;
        QString room_id_;
        RoomInfo info_;
};
