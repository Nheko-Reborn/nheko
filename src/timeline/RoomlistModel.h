// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QSharedPointer>
#include <QString>

#include <mtx/responses/sync.hpp>

class TimelineModel;
class TimelineViewManager;

class RoomlistModel : public QAbstractListModel
{
        Q_OBJECT
public:
        enum Roles
        {
                AvatarUrl = Qt::UserRole,
                RoomName,
                RoomId,
                LastMessage,
                Timestamp,
                HasUnreadMessages,
                HasLoudNotification,
                NotificationCount,
        };

        RoomlistModel(TimelineViewManager *parent = nullptr);
        QHash<int, QByteArray> roleNames() const override;
        int rowCount(const QModelIndex &parent = QModelIndex()) const override
        {
                (void)parent;
                return (int)roomids.size();
        }
        QVariant data(const QModelIndex &index, int role) const override;
        QSharedPointer<TimelineModel> getRoomById(QString id) const
        {
                if (models.contains(id))
                        return models.value(id);
                else
                        return {};
        }

public slots:
        void initializeRooms(const std::vector<QString> &roomids);
        void sync(const mtx::responses::Rooms &rooms);
        void clear();
        int roomidToIndex(QString roomid)
        {
                for (int i = 0; i < (int)roomids.size(); i++) {
                        if (roomids[i] == roomid)
                                return i;
                }

                return -1;
        }

private slots:
        void updateReadStatus(const std::map<QString, bool> roomReadStatus_);

signals:
        void totalUnreadMessageCountUpdated(int unreadMessages);

private:
        void addRoom(const QString &room_id, bool suppressInsertNotification = false);

        TimelineViewManager *manager = nullptr;
        std::vector<QString> roomids;
        QHash<QString, QSharedPointer<TimelineModel>> models;
        std::map<QString, bool> roomReadStatus;
};
