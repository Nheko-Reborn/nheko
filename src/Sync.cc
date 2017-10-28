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

#include "Sync.h"

void
SyncResponse::deserialize(const QJsonDocument &data)
{
        if (!data.isObject())
                throw DeserializationException("Sync response is not a JSON object");

        QJsonObject object = data.object();

        if (!object.contains("next_batch"))
                throw DeserializationException("Sync: missing next_batch parameter");

        if (object.contains("rooms")) {
                if (!object.value("rooms").isObject()) {
                        throw DeserializationException("Sync: rooms is not a JSON object");
                }
                rooms_.deserialize(object.value("rooms"));
        }

        if (object.contains("presence")) {
                if (!object.value("presence").isObject()) {
                        throw DeserializationException("Sync: presence is not a JSON object");
                }
                // TODO: implement presence handling
        }

        if (object.contains("account_data")) {
                if (!object.value("account_data").isObject()) {
                        throw DeserializationException("Sync: account_data is not a JSON object");
                }
                // TODO: implement account_data handling
        }

        if (object.contains("to_device")) {
                if (!object.value("to_device").isObject()) {
                        throw DeserializationException("Sync: to_device is not a JSON object");
                }
                // TODO: implement to_device handling
        }

        // for device_lists updates (for e2e)
        if (object.contains("device_lists")) {
                if (!object.value("device_lists").isObject()) {
                        throw DeserializationException("Sync: device_lists is not a JSON object");
                }
                // TODO: implement device_lists handling
        }

        next_batch_ = object.value("next_batch").toString();
}

void
Rooms::deserialize(const QJsonValue &data)
{
        if (!data.isObject())
                throw DeserializationException("Rooms value is not a JSON object");

        QJsonObject object = data.toObject();

        if (object.contains("join")) {
                if (!object.value("join").isObject())
                        throw DeserializationException("rooms/join must be a JSON object");

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

        if (object.contains("invite")) {
                if (!object.value("invite").isObject()) {
                        throw DeserializationException("rooms/invite must be a JSON object");
                }
                // TODO: Implement invite handling
        }

        if (object.contains("leave")) {
                if (!object.value("leave").isObject()) {
                        throw DeserializationException("rooms/leave must be a JSON object");
                }
                auto leave = object.value("leave").toObject();

                for (auto it = leave.constBegin(); it != leave.constEnd(); it++) {
                        LeftRoom tmp_room;

                        try {
                                tmp_room.deserialize(it.value());
                                leave_.insert(it.key(), tmp_room);
                        } catch (DeserializationException &e) {
                                qWarning() << e.what();
                                qWarning() << "Skipping malformed object for room" << it.key();
                        }
                }
        }
}

void
JoinedRoom::deserialize(const QJsonValue &data)
{
        if (!data.isObject())
                throw DeserializationException("JoinedRoom is not a JSON object");

        QJsonObject object = data.toObject();

        if (object.contains("state")) {
                if (!object.value("state").isObject()) {
                        throw DeserializationException("join/state should be an object");
                }

                QJsonObject state = object.value("state").toObject();

                if (state.contains("events")) {
                        if (!state.value("events").isArray()) {
                                throw DeserializationException(
                                  "join/state/events should be an array");
                        }

                        state_.deserialize(state.value("events"));
                }
        }

        if (object.contains("timeline")) {
                if (!object.value("timeline").isObject())
                        throw DeserializationException("join/timeline should be an object");
                timeline_.deserialize(object.value("timeline"));
        }

        if (object.contains("ephemeral")) {
                if (!object.value("ephemeral").isObject())
                        throw DeserializationException("join/ephemeral should be an object");

                QJsonObject ephemeral = object.value("ephemeral").toObject();

                if (ephemeral.contains("events")) {
                        if (!ephemeral.value("events").isArray())
                                qWarning() << "join/ephemeral/events should be an array";

                        auto ephemeralEvents = ephemeral.value("events").toArray();

                        for (const auto e : ephemeralEvents) {
                                auto obj = e.toObject();

                                if (obj.contains("type") && obj.value("type") == "m.typing") {
                                        auto ids = obj.value("content")
                                                     .toObject()
                                                     .value("user_ids")
                                                     .toArray();

                                        for (const auto uid : ids)
                                                typingUserIDs_.push_back(uid.toString());
                                }
                        }
                }
        }

        if (object.contains("account_data")) {
                if (!object.value("account_data").isObject())
                        throw DeserializationException("join/account_data is not a JSON object");
                // TODO: Implement account_data handling
        }

        if (object.contains("unread_notifications")) {
                if (!object.value("unread_notifications").isObject()) {
                        throw DeserializationException(
                          "join/unread_notifications is not a JSON object");
                }

                QJsonObject unreadNotifications = object.value("unread_notifications").toObject();

                if (unreadNotifications.contains("highlight_count")) {
                        // TODO: Implement unread_notifications handling
                }
                if (unreadNotifications.contains("notification_count")) {
                        // TODO: Implement unread_notifications handling
                }
        }
}

void
LeftRoom::deserialize(const QJsonValue &data)
{
        if (!data.isObject())
                throw DeserializationException("LeftRoom is not a JSON object");

        QJsonObject object = data.toObject();

        if (!object.contains("state"))
                throw DeserializationException("leave/state is missing");

        if (!object.contains("timeline"))
                throw DeserializationException("leave/timeline is missing");

        if (!object.value("state").isObject())
                throw DeserializationException("leave/state should be an object");

        QJsonObject state = object.value("state").toObject();

        if (!state.contains("events"))
                throw DeserializationException("leave/state/events is missing");

        state_.deserialize(state.value("events"));
        timeline_.deserialize(object.value("timeline"));
}

void
Event::deserialize(const QJsonValue &data)
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

        content_  = object.value("content").toObject();
        unsigned_ = object.value("unsigned").toObject();

        sender_    = object.value("sender").toString();
        state_key_ = object.value("state_key").toString();
        type_      = object.value("type").toString();
        event_id_  = object.value("event_id").toString();

        origin_server_ts_ = object.value("origin_server_ts").toDouble();
}

void
State::deserialize(const QJsonValue &data)
{
        if (!data.isArray())
                throw DeserializationException("State is not a JSON array");

        events_ = data.toArray();
}

void
Timeline::deserialize(const QJsonValue &data)
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
        limited_    = object.value("limited").toBool();

        if (!object.value("events").isArray())
                throw DeserializationException("timeline/events is not a JSON array");

        events_ = object.value("events").toArray();
}
