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

#include "events/Event.h"

#include "AliasesEventContent.h"
#include "AvatarEventContent.h"
#include "CanonicalAliasEventContent.h"
#include "CreateEventContent.h"
#include "Deserializable.h"
#include "HistoryVisibilityEventContent.h"
#include "JoinRulesEventContent.h"
#include "MemberEventContent.h"
#include "NameEventContent.h"
#include "PowerLevelsEventContent.h"
#include "TopicEventContent.h"

matrix::events::EventType
matrix::events::extractEventType(const QJsonObject &object)
{
        if (!object.contains("type"))
                throw DeserializationException("Missing event type");

        auto type = object.value("type").toString();

        if (type == "m.room.aliases")
                return EventType::RoomAliases;
        else if (type == "m.room.avatar")
                return EventType::RoomAvatar;
        else if (type == "m.room.canonical_alias")
                return EventType::RoomCanonicalAlias;
        else if (type == "m.room.create")
                return EventType::RoomCreate;
        else if (type == "m.room.history_visibility")
                return EventType::RoomHistoryVisibility;
        else if (type == "m.room.join_rules")
                return EventType::RoomJoinRules;
        else if (type == "m.room.member")
                return EventType::RoomMember;
        else if (type == "m.room.message")
                return EventType::RoomMessage;
        else if (type == "m.room.name")
                return EventType::RoomName;
        else if (type == "m.room.power_levels")
                return EventType::RoomPowerLevels;
        else if (type == "m.room.topic")
                return EventType::RoomTopic;
        else
                return EventType::Unsupported;
}

bool
matrix::events::isStateEvent(EventType type)
{
        return type == EventType::RoomAliases || type == EventType::RoomAvatar ||
               type == EventType::RoomCanonicalAlias || type == EventType::RoomCreate ||
               type == EventType::RoomHistoryVisibility || type == EventType::RoomJoinRules ||
               type == EventType::RoomMember || type == EventType::RoomName ||
               type == EventType::RoomPowerLevels || type == EventType::RoomTopic;
}

bool
matrix::events::isMessageEvent(EventType type)
{
        return type == EventType::RoomMessage;
}

void
matrix::events::UnsignedData::deserialize(const QJsonValue &data)
{
        if (!data.isObject())
                throw DeserializationException("UnsignedData is not a JSON object");

        auto object = data.toObject();

        transaction_id_ = object.value("transaction_id").toString();
        age_            = object.value("age").toDouble();
}

QJsonObject
matrix::events::UnsignedData::serialize() const
{
        QJsonObject object;

        if (!transaction_id_.isEmpty())
                object["transaction_id"] = transaction_id_;

        if (age_ > 0)
                object["age"] = age_;

        return object;
}
