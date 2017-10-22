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

#include <QDebug>
#include <QJsonValue>

#include "Deserializable.h"

namespace matrix {
namespace events {
enum class EventType
{
        /// m.room.aliases
        RoomAliases,
        /// m.room.avatar
        RoomAvatar,
        /// m.room.canonical_alias
        RoomCanonicalAlias,
        /// m.room.create
        RoomCreate,
        /// m.room.history_visibility
        RoomHistoryVisibility,
        /// m.room.join_rules
        RoomJoinRules,
        /// m.room.member
        RoomMember,
        /// m.room.message
        RoomMessage,
        /// m.room.name
        RoomName,
        /// m.room.power_levels
        RoomPowerLevels,
        /// m.room.topic
        RoomTopic,
        // Unsupported event
        Unsupported,
};

EventType
extractEventType(const QJsonObject &data);

bool
isMessageEvent(EventType type);
bool
isStateEvent(EventType type);

template<class Content>
class Event
  : public Deserializable
  , public Serializable
{
public:
        Content content() const;
        EventType eventType() const;

        void deserialize(const QJsonValue &data) override;
        QJsonObject serialize() const override;

private:
        Content content_;
        EventType type_;
};

template<class Content>
inline Content
Event<Content>::content() const
{
        return content_;
}

template<class Content>
inline EventType
Event<Content>::eventType() const
{
        return type_;
}

template<class Content>
void
Event<Content>::deserialize(const QJsonValue &data)
{
        if (!data.isObject())
                throw DeserializationException("Event is not a JSON object");

        auto object = data.toObject();

        content_.deserialize(object.value("content"));
        type_ = extractEventType(object);
}

template<class Content>
QJsonObject
Event<Content>::serialize() const
{
        QJsonObject object;

        switch (type_) {
        case EventType::RoomAliases:
                object["type"] = "m.room.aliases";
                break;
        case EventType::RoomAvatar:
                object["type"] = "m.room.avatar";
                break;
        case EventType::RoomCanonicalAlias:
                object["type"] = "m.room.canonical_alias";
                break;
        case EventType::RoomCreate:
                object["type"] = "m.room.create";
                break;
        case EventType::RoomHistoryVisibility:
                object["type"] = "m.room.history_visibility";
                break;
        case EventType::RoomJoinRules:
                object["type"] = "m.room.join_rules";
                break;
        case EventType::RoomMember:
                object["type"] = "m.room.member";
                break;
        case EventType::RoomMessage:
                object["type"] = "m.room.message";
                break;
        case EventType::RoomName:
                object["type"] = "m.room.name";
                break;
        case EventType::RoomPowerLevels:
                object["type"] = "m.room.power_levels";
                break;
        case EventType::RoomTopic:
                object["type"] = "m.room.topic";
                break;
        case EventType::Unsupported:
                qWarning() << "Unsupported type to serialize";
                break;
        }

        object["content"] = content_.serialize();

        return object;
}
} // namespace events
} // namespace matrix
