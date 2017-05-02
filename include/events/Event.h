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

#ifndef MATRIX_EVENT_H
#define MATRIX_EVENT_H

#include <QJsonValue>

#include "Deserializable.h"

enum EventType {
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
	/// m.room.name
	RoomName,
	/// m.room.power_levels
	RoomPowerLevels,
	/// m.room.topic
	RoomTopic,
	// Unsupported event
	Unsupported,
};

EventType extractEventType(const QJsonObject &data);

template <class Content>
class Event : public Deserializable
{
public:
	inline Content content() const;
	inline EventType eventType() const;

	void deserialize(const QJsonValue &data) override;

private:
	Content content_;
	EventType type_;
};

template <class Content>
inline Content Event<Content>::content() const
{
	return content_;
}

template <class Content>
inline EventType Event<Content>::eventType() const
{
	return type_;
}

template <class Content>
void Event<Content>::deserialize(const QJsonValue &data)
{
	if (!data.isObject())
		throw DeserializationException("Event is not a JSON object");

	auto object = data.toObject();

	content_.deserialize(object.value("content"));
}

#endif  // MATRIX_EVENT_H
