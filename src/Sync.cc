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

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include "Deserializable.h"
#include "Sync.h"

void SyncResponse::deserialize(const QJsonDocument &data)
{
	if (!data.isObject())
		throw DeserializationException("Sync response is not a JSON object");

	QJsonObject object = data.object();

	if (object.value("next_batch") == QJsonValue::Undefined)
		throw DeserializationException("Sync: missing next_batch parameter");

	if (object.value("rooms") == QJsonValue::Undefined)
		throw DeserializationException("Sync: missing rooms parameter");

	rooms_.deserialize(object.value("rooms"));
	next_batch_ = object.value("next_batch").toString();
}

void Rooms::deserialize(const QJsonValue &data)
{
	if (!data.isObject())
		throw DeserializationException("Rooms value is not a JSON object");

	QJsonObject object = data.toObject();

	if (!object.contains("join"))
		throw DeserializationException("rooms/join is missing");

	if (!object.contains("invite"))
		throw DeserializationException("rooms/invite is missing");

	if (!object.contains("leave"))
		throw DeserializationException("rooms/leave is missing");

	if (!object.value("join").isObject())
		throw DeserializationException("rooms/join must be a JSON object");

	if (!object.value("invite").isObject())
		throw DeserializationException("rooms/invite must be a JSON object");

	if (!object.value("leave").isObject())
		throw DeserializationException("rooms/leave must be a JSON object");

	auto join = object.value("join").toObject();

	for (auto it = join.constBegin(); it != join.constEnd(); it++) {
		JoinedRoom tmp_room;

		try {
			tmp_room.deserialize(it.value());
			join_.insert(it.key(), tmp_room);
		} catch (DeserializationException &e) {
			qWarning() << e.what();
			qWarning() << "Skipping malformed object for room" << it.key();
		}
	}
}

void JoinedRoom::deserialize(const QJsonValue &data)
{
	if (!data.isObject())
		throw DeserializationException("JoinedRoom is not a JSON object");

	QJsonObject object = data.toObject();

	if (!object.contains("state"))
		throw DeserializationException("join/state is missing");

	if (!object.contains("timeline"))
		throw DeserializationException("join/timeline is missing");

	if (!object.contains("account_data"))
		throw DeserializationException("join/account_data is missing");

	if (!object.contains("unread_notifications"))
		throw DeserializationException("join/unread_notifications is missing");

	if (!object.value("state").isObject())
		throw DeserializationException("join/state should be an object");

	QJsonObject state = object.value("state").toObject();

	if (!state.contains("events"))
		throw DeserializationException("join/state/events is missing");

	state_.deserialize(state.value("events"));
	timeline_.deserialize(object.value("timeline"));
}

void Event::deserialize(const QJsonValue &data)
{
	if (!data.isObject())
		throw DeserializationException("Event is not a JSON object");

	QJsonObject object = data.toObject();

	if (!object.contains("content"))
		throw DeserializationException("event/content is missing");

	if (!object.contains("unsigned"))
		throw DeserializationException("event/content is missing");

	if (!object.contains("sender"))
		throw DeserializationException("event/sender is missing");

	if (!object.contains("event_id"))
		throw DeserializationException("event/event_id is missing");

	// TODO: Make this optional
	/* if (!object.contains("state_key")) */
	/* 	throw DeserializationException("event/state_key is missing"); */

	if (!object.contains("type"))
		throw DeserializationException("event/type is missing");

	if (!object.contains("origin_server_ts"))
		throw DeserializationException("event/origin_server_ts is missing");

	content_ = object.value("content").toObject();
	unsigned_ = object.value("unsigned").toObject();

	sender_ = object.value("sender").toString();
	state_key_ = object.value("state_key").toString();
	type_ = object.value("type").toString();
	event_id_ = object.value("event_id").toString();

	origin_server_ts_ = object.value("origin_server_ts").toDouble();
}

void State::deserialize(const QJsonValue &data)
{
	if (!data.isArray())
		throw DeserializationException("State is not a JSON array");

	QJsonArray event_array = data.toArray();

	for (int i = 0; i < event_array.count(); i++) {
		Event event;

		try {
			event.deserialize(event_array.at(i));
			events_.push_back(event);
		} catch (DeserializationException &e) {
			qWarning() << e.what();
			qWarning() << "Skipping malformed state event";
		}
	}
}

void Timeline::deserialize(const QJsonValue &data)
{
	if (!data.isObject())
		throw DeserializationException("Timeline is not a JSON object");

	auto object = data.toObject();

	if (!object.contains("events"))
		throw DeserializationException("timeline/events is missing");

	if (!object.contains("prev_batch"))
		throw DeserializationException("timeline/prev_batch is missing");

	if (!object.contains("limited"))
		throw DeserializationException("timeline/limited is missing");

	prev_batch_ = object.value("prev_batch").toString();
	limited_ = object.value("limited").toBool();

	if (!object.value("events").isArray())
		throw DeserializationException("timeline/events is not a JSON array");

	auto timeline_events = object.value("events").toArray();

	for (int i = 0; i < timeline_events.count(); i++) {
		Event event;

		try {
			event.deserialize(timeline_events.at(i));
			events_.push_back(event);
		} catch (DeserializationException &e) {
			qWarning() << e.what();
			qWarning() << "Skipping malformed timeline event";
		}
	}
}
