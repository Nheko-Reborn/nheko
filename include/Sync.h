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

#include <QJsonArray>
#include <QMap>

#include "Deserializable.h"

class Event : public Deserializable
{
public:
        QJsonObject content() const { return content_; };
        QJsonObject unsigned_content() const { return unsigned_; };

        QString sender() const { return sender_; };
        QString state_key() const { return state_key_; };
        QString type() const { return type_; };
        QString eventId() const { return event_id_; };

        uint64_t timestamp() const { return origin_server_ts_; };

        void deserialize(const QJsonValue &data) override;

private:
        QJsonObject content_;
        QJsonObject unsigned_;

        QString sender_;
        QString state_key_;
        QString type_;
        QString event_id_;

        uint64_t origin_server_ts_;
};

class State : public Deserializable
{
public:
        void deserialize(const QJsonValue &data) override;
        QJsonArray events() const { return events_; };

private:
        QJsonArray events_;
};

class Timeline : public Deserializable
{
public:
        QJsonArray events() const { return events_; };
        QString previousBatch() const { return prev_batch_; };
        bool limited() const { return limited_; };

        void deserialize(const QJsonValue &data) override;

private:
        QJsonArray events_;
        QString prev_batch_;
        bool limited_;
};

// TODO: Add support for account_data, undread_notifications
class JoinedRoom : public Deserializable
{
public:
        State state() const { return state_; };
        Timeline timeline() const { return timeline_; };
        QList<QString> typingUserIDs() const { return typingUserIDs_; };

        void deserialize(const QJsonValue &data) override;

private:
        State state_;
        Timeline timeline_;
        QList<QString> typingUserIDs_;
        /* AccountData account_data_; */
        /* UnreadNotifications unread_notifications_; */
};

class LeftRoom : public Deserializable
{
public:
        State state() const { return state_; };
        Timeline timeline() const { return timeline_; };

        void deserialize(const QJsonValue &data) override;

private:
        State state_;
        Timeline timeline_;
};

// TODO: Add support for invited and left rooms.
class Rooms : public Deserializable
{
public:
        QMap<QString, JoinedRoom> join() const { return join_; };
        QMap<QString, LeftRoom> leave() const { return leave_; };
        void deserialize(const QJsonValue &data) override;

private:
        QMap<QString, JoinedRoom> join_;
        QMap<QString, LeftRoom> leave_;
};

class SyncResponse : public Deserializable
{
public:
        void deserialize(const QJsonDocument &data) override;
        QString nextBatch() const { return next_batch_; };
        Rooms rooms() const { return rooms_; };

private:
        QString next_batch_;
        Rooms rooms_;
};
