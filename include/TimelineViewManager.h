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
#include <QSharedPointer>
#include <QStackedWidget>

class JoinedRoom;
class MatrixClient;
class RoomInfoListItem;
class Rooms;
class TimelineView;
struct DescInfo;

class TimelineViewManager : public QStackedWidget
{
        Q_OBJECT

public:
        TimelineViewManager(QSharedPointer<MatrixClient> client, QWidget *parent);
        ~TimelineViewManager();

        // Initialize with timeline events.
        void initialize(const Rooms &rooms);
        // Empty initialization.
        void initialize(const QList<QString> &rooms);

        void addRoom(const JoinedRoom &room, const QString &room_id);
        void addRoom(const QString &room_id);

        void sync(const Rooms &rooms);
        void clearAll();

        // Check if all the timelines have been loaded.
        bool hasLoaded() const;

        static QString chooseRandomColor();
        static QString displayName(const QString &userid);

        static QMap<QString, QString> DISPLAY_NAMES;

signals:
        void unreadMessages(QString roomid, int count);
        void updateRoomsLastMessage(const QString &user, const DescInfo &info);

public slots:
        void setHistoryView(const QString &room_id);
        void sendTextMessage(const QString &msg);
        void sendEmoteMessage(const QString &msg);
        void sendImageMessage(const QString &roomid, const QString &filename, const QString &url);

private slots:
        void messageSent(const QString &eventid, const QString &roomid, int txnid);

private:
        QString active_room_;
        QMap<QString, QSharedPointer<TimelineView>> views_;
        QSharedPointer<MatrixClient> client_;
};
