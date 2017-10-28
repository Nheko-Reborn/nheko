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

#include <QJsonObject>

#include "Deserializable.h"
#include "PowerLevelsEventContent.h"

using namespace matrix::events;

void
PowerLevelsEventContent::deserialize(const QJsonValue &data)
{
        if (!data.isObject())
                throw DeserializationException("PowerLevelsEventContent is not a JSON object");

        auto object = data.toObject();

        if (object.value("ban") != QJsonValue::Undefined)
                ban_ = object.value("ban").toInt();

        if (object.value("invite") != QJsonValue::Undefined)
                invite_ = object.value("invite").toInt();

        if (object.value("kick") != QJsonValue::Undefined)
                kick_ = object.value("kick").toInt();

        if (object.value("redact") != QJsonValue::Undefined)
                redact_ = object.value("redact").toInt();

        if (object.value("events_default") != QJsonValue::Undefined)
                events_default_ = object.value("events_default").toInt();

        if (object.value("state_default") != QJsonValue::Undefined)
                state_default_ = object.value("state_default").toInt();

        if (object.value("users_default") != QJsonValue::Undefined)
                users_default_ = object.value("users_default").toInt();

        if (object.value("users").isObject()) {
                auto users = object.value("users").toObject();

                for (auto it = users.constBegin(); it != users.constEnd(); ++it)
                        users_.insert(it.key(), it.value().toInt());
        }

        if (object.value("events").isObject()) {
                auto events = object.value("events").toObject();

                for (auto it = events.constBegin(); it != events.constEnd(); ++it)
                        events_.insert(it.key(), it.value().toInt());
        }
}

QJsonObject
PowerLevelsEventContent::serialize() const
{
        QJsonObject object;

        object["ban"]    = ban_;
        object["invite"] = invite_;
        object["kick"]   = kick_;
        object["redact"] = redact_;

        object["events_default"] = events_default_;
        object["users_default"]  = users_default_;
        object["state_default"]  = state_default_;

        QJsonObject users;
        QJsonObject events;

        for (auto it = users_.constBegin(); it != users_.constEnd(); ++it)
                users.insert(it.key(), it.value());

        for (auto it = events_.constBegin(); it != events_.constEnd(); ++it)
                events.insert(it.key(), it.value());

        object["users"]  = users;
        object["events"] = events;

        return object;
}

int
PowerLevelsEventContent::eventLevel(QString event_type) const
{
        if (events_.contains(event_type))
                return events_[event_type];

        return events_default_;
}

int
PowerLevelsEventContent::userLevel(QString userid) const
{
        if (users_.contains(userid))
                return users_[userid];

        return users_default_;
}
