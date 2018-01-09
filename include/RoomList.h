/*
 * nheko Copyright (C) 2017  Konstantinos Sideris <siderisk@auth.gr>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <QMap>
#include <QPushButton>
#include <QScrollArea>
#include <QSharedPointer>
#include <QVBoxLayout>
#include <QWidget>

#include <mtx.hpp>

#include "dialogs/LeaveRoom.h"

class LeaveRoomDialog;
class MatrixClient;
class Cache;
class OverlayModal;
class RoomInfoListItem;
class RoomSettings;
class RoomState;
class Sync;
class UserSettings;
struct DescInfo;

class RoomList : public QWidget
{
        Q_OBJECT

public:
        RoomList(QSharedPointer<MatrixClient> client,
                 QSharedPointer<UserSettings> userSettings,
                 QWidget *parent = 0);
        ~RoomList();

        void setCache(QSharedPointer<Cache> cache) { cache_ = cache; }
        void setInitialRooms(const QMap<QString, QSharedPointer<RoomSettings>> &settings,
                             const QMap<QString, RoomState> &states);
        void sync(const QMap<QString, RoomState> &states,
                  QMap<QString, QSharedPointer<RoomSettings>> &settings);
        void syncInvites(const std::map<std::string, mtx::responses::InvitedRoom> &rooms);

        void clear();
        void updateAvatar(const QString &room_id, const QString &url);

        void addRoom(const QMap<QString, QSharedPointer<RoomSettings>> &settings,
                     const RoomState &state,
                     const QString &room_id);
        void addInvitedRoom(const QString &room_id, const mtx::responses::InvitedRoom &room);
        void removeRoom(const QString &room_id, bool reset);
        void setFilterRooms(bool filterRooms);
        void setRoomFilter(QList<QString> room_ids);

signals:
        void roomChanged(const QString &room_id);
        void totalUnreadMessageCountUpdated(int count);
        void acceptInvite(const QString &room_id);
        void declineInvite(const QString &room_id);
        void roomAvatarChanged(const QString &room_id, const QPixmap &img);

public slots:
        void updateRoomAvatar(const QString &roomid, const QPixmap &img);
        void highlightSelectedRoom(const QString &room_id);
        void updateUnreadMessageCount(const QString &roomid, int count);
        void updateRoomDescription(const QString &roomid, const DescInfo &info);
        void closeJoinRoomDialog(bool isJoining, QString roomAlias);
        void openLeaveRoomDialog(const QString &room_id);
        void closeLeaveRoomDialog(bool leaving, const QString &room_id);
        void clearRoomMessageCount(const QString &room_id);

protected:
        void paintEvent(QPaintEvent *event) override;
        void leaveEvent(QEvent *event) override;

private slots:
        void sortRoomsByLastMessage();

private:
        void calculateUnreadMessageCount();

        QVBoxLayout *topLayout_;
        QVBoxLayout *contentsLayout_;
        QScrollArea *scrollArea_;
        QWidget *scrollAreaContents_;

        QPushButton *joinRoomButton_;

        OverlayModal *joinRoomModal_;

        QSharedPointer<OverlayModal> leaveRoomModal_;
        QSharedPointer<dialogs::LeaveRoom> leaveRoomDialog_;

        QMap<QString, QSharedPointer<RoomInfoListItem>> rooms_;
        QString selectedRoom_;

        bool filterRooms_          = false;
        QList<QString> roomFilter_ = QList<QString>(); // which rooms to include in the room list

        QSharedPointer<MatrixClient> client_;
        QSharedPointer<Cache> cache_;
        QSharedPointer<UserSettings> userSettings_;

        bool isSortPending_ = false;
};
