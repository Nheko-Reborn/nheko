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

#include <QPushButton>
#include <QScrollArea>
#include <QSharedPointer>
#include <QVBoxLayout>
#include <QWidget>

#include <mtx.hpp>

class LeaveRoomDialog;
class OverlayModal;
class RoomInfoListItem;
class Sync;
class UserSettings;
struct DescInfo;
struct RoomInfo;

class RoomList : public QWidget
{
        Q_OBJECT

public:
        RoomList(QSharedPointer<UserSettings> userSettings, QWidget *parent = 0);

        void initialize(const QMap<QString, RoomInfo> &info);
        void sync(const std::map<QString, RoomInfo> &info);

        void clear() { rooms_.clear(); };
        void updateAvatar(const QString &room_id, const QString &url);

        void addRoom(const QString &room_id, const RoomInfo &info);
        void addInvitedRoom(const QString &room_id, const RoomInfo &info);
        void removeRoom(const QString &room_id, bool reset);
        void setFilterRooms(bool filterRooms);
        void setRoomFilter(std::vector<QString> room_ids);
        void updateRoom(const QString &room_id, const RoomInfo &info);
        void cleanupInvites(const std::map<QString, bool> &invites);

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

protected:
        void paintEvent(QPaintEvent *event) override;
        void leaveEvent(QEvent *event) override;

private slots:
        void sortRoomsByLastMessage();

private:
        //! Return the first non-null room.
        std::pair<QString, QSharedPointer<RoomInfoListItem>> firstRoom() const;
        void calculateUnreadMessageCount();
        bool roomExists(const QString &room_id) { return rooms_.find(room_id) != rooms_.end(); }
        bool filterItemExists(const QString &id)
        {
                return std::find(roomFilter_.begin(), roomFilter_.end(), id) != roomFilter_.end();
        }

        QVBoxLayout *topLayout_;
        QVBoxLayout *contentsLayout_;
        QScrollArea *scrollArea_;
        QWidget *scrollAreaContents_;

        QPushButton *joinRoomButton_;

        OverlayModal *joinRoomModal_;

        std::map<QString, QSharedPointer<RoomInfoListItem>> rooms_;
        QString selectedRoom_;

        //! Which rooms to include in the room list.
        std::vector<QString> roomFilter_;

        QSharedPointer<UserSettings> userSettings_;

        bool isSortPending_ = false;
};
