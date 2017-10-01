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
#include <QJsonDocument>
#include <QMap>
#include <QString>

#include "Deserializable.h"

class Event : public Deserializable
{
public:
        inline QJsonObject content() const;
        inline QJsonObject unsigned_content() const;

        inline QString sender() const;
        inline QString state_key() const;
        inline QString type() const;
        inline QString eventId() const;

        inline uint64_t timestamp() const;

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

inline QJsonObject
Event::content() const
{
        return content_;
}

inline QJsonObject
Event::unsigned_content() const
{
        return unsigned_;
}

inline QString
Event::sender() const
{
        return sender_;
}

inline QString
Event::state_key() const
{
        return state_key_;
}

inline QString
Event::type() const
{
        return type_;
}

inline QString
Event::eventId() const
{
        return event_id_;
}

inline uint64_t
Event::timestamp() const
{
        return origin_server_ts_;
}

class State : public Deserializable
{
public:
        void deserialize(const QJsonValue &data) override;
        inline QJsonArray events() const;

private:
        QJsonArray events_;
};

inline QJsonArray
State::events() const
{
        return events_;
}

class Timeline : public Deserializable
{
public:
        inline QJsonArray events() const;
        inline QString previousBatch() const;
        inline bool limited() const;

        void deserialize(const QJsonValue &data) override;

private:
        QJsonArray events_;
        QString prev_batch_;
        bool limited_;
};

inline QJsonArray
Timeline::events() const
{
        return events_;
}

inline QString
Timeline::previousBatch() const
{
        return prev_batch_;
}

inline bool
Timeline::limited() const
{
        return limited_;
}

// TODO: Add support for ehpmeral, account_data, undread_notifications
class JoinedRoom : public Deserializable
{
public:
        inline State state() const;
        inline Timeline timeline() const;

        void deserialize(const QJsonValue &data) override;

private:
        State state_;
        Timeline timeline_;
        /* Ephemeral ephemeral_; */
        /* AccountData account_data_; */
        /* UnreadNotifications unread_notifications_; */
};

inline State
JoinedRoom::state() const
{
        return state_;
}

inline Timeline
JoinedRoom::timeline() const
{
        return timeline_;
}

class LeftRoom : public Deserializable
{
public:
        inline State state() const;
        inline Timeline timeline() const;

        void deserialize(const QJsonValue &data) override;

private:
        State state_;
        Timeline timeline_;
};

inline State
LeftRoom::state() const
{
        return state_;
}

inline Timeline
LeftRoom::timeline() const
{
        return timeline_;
}

// TODO: Add support for invited and left rooms.
class Rooms : public Deserializable
{
public:
        inline QMap<QString, JoinedRoom> join() const;
        inline QMap<QString, LeftRoom> leave() const;
        void deserialize(const QJsonValue &data) override;

private:
        QMap<QString, JoinedRoom> join_;
        QMap<QString, LeftRoom> leave_;
};

inline QMap<QString, JoinedRoom>
Rooms::join() const
{
        return join_;
}

inline QMap<QString, LeftRoom>
Rooms::leave() const
{
        return leave_;
}

class SyncResponse : public Deserializable
{
public:
        void deserialize(const QJsonDocument &data) override;
        inline QString nextBatch() const;
        inline Rooms rooms() const;

private:
        QString next_batch_;
        Rooms rooms_;
};

inline Rooms
SyncResponse::rooms() const
{
        return rooms_;
}

inline QString
SyncResponse::nextBatch() const
{
        return next_batch_;
}
