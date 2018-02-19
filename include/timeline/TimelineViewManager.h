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

#include <QSharedPointer>
#include <QStackedWidget>

#include <mtx.hpp>

class QFile;

class MatrixClient;
class RoomInfoListItem;
class TimelineView;
struct DescInfo;

class TimelineViewManager : public QStackedWidget
{
        Q_OBJECT

public:
        TimelineViewManager(QSharedPointer<MatrixClient> client, QWidget *parent);

        // Initialize with timeline events.
        void initialize(const mtx::responses::Rooms &rooms);
        // Empty initialization.
        void initialize(const std::vector<QString> &rooms);

        void addRoom(const mtx::responses::JoinedRoom &room, const QString &room_id);
        void addRoom(const QString &room_id);

        void sync(const mtx::responses::Rooms &rooms);
        void clearAll() { views_.clear(); }

        // Check if all the timelines have been loaded.
        bool hasLoaded() const;

        static QString chooseRandomColor();
        static QString displayName(const QString &userid);

        static std::map<QString, QString> DISPLAY_NAMES;

signals:
        void clearRoomMessageCount(QString roomid);
        void updateRoomsLastMessage(const QString &user, const DescInfo &info);

public slots:
        void setHistoryView(const QString &room_id);
        void queueTextMessage(const QString &msg);
        void queueEmoteMessage(const QString &msg);
        void queueImageMessage(const QString &roomid,
                               const QString &filename,
                               const QString &url,
                               const QString &mime,
                               uint64_t dsize);
        void queueFileMessage(const QString &roomid,
                              const QString &filename,
                              const QString &url,
                              const QString &mime,
                              uint64_t dsize);
        void queueAudioMessage(const QString &roomid,
                               const QString &filename,
                               const QString &url,
                               const QString &mime,
                               uint64_t dsize);
        void queueVideoMessage(const QString &roomid,
                               const QString &filename,
                               const QString &url,
                               const QString &mime,
                               uint64_t dsize);

private slots:
        void messageSent(const QString &eventid, const QString &roomid, int txnid);
        void messageSendFailed(const QString &roomid, int txnid);

private:
        //! Check if the given room id is managed by a TimelineView.
        bool timelineViewExists(const QString &id) { return views_.find(id) != views_.end(); }

        QString active_room_;
        std::map<QString, QSharedPointer<TimelineView>> views_;
        QSharedPointer<MatrixClient> client_;
};
