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

#ifndef MATRIX_ROOM_EVENT_H
#define MATRIX_ROOM_EVENT_H

#include <QJsonValue>
#include <QString>

#include "Event.h"

template <class Content>
class RoomEvent : public Event<Content>
{
public:
	inline QString eventId() const;
	inline QString roomId() const;
	inline QString sender() const;
	inline uint64_t timestamp() const;

	void deserialize(const QJsonValue &data) override;

private:
	QString event_id_;
	QString room_id_;
	QString sender_;

	uint64_t origin_server_ts_;
};

template <class Content>
inline QString RoomEvent<Content>::eventId() const
{
	return event_id_;
}

template <class Content>
inline QString RoomEvent<Content>::roomId() const
{
	return room_id_;
}

template <class Content>
inline QString RoomEvent<Content>::sender() const
{
	return sender_;
}

template <class Content>
inline uint64_t RoomEvent<Content>::timestamp() const
{
	return origin_server_ts_;
}

template <class Content>
void RoomEvent<Content>::deserialize(const QJsonValue &data)
{
	Event<Content>::deserialize(data);

	auto object = data.toObject();

	if (!object.contains("event_id"))
		throw DeserializationException("event_id key is missing");

	if (!object.contains("origin_server_ts"))
		throw DeserializationException("origin_server_ts key is missing");

	if (!object.contains("room_id"))
		throw DeserializationException("room_id key is missing");

	if (!object.contains("sender"))
		throw DeserializationException("sender key is missing");

	event_id_ = object.value("event_id").toString();
	room_id_ = object.value("room_id").toString();
	sender_ = object.value("sender").toString();
	origin_server_ts_ = object.value("origin_server_ts").toDouble();
}

#endif  // MATRIX_ROOM_EVENT_H
