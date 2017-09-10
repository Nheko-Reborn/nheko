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

#include "HistoryVisibilityEventContent.h"

using namespace matrix::events;

void
HistoryVisibilityEventContent::deserialize(const QJsonValue &data)
{
        if (!data.isObject())
                throw DeserializationException(
                  "HistoryVisibilityEventContent is not a JSON object");

        auto object = data.toObject();

        if (object.value("history_visibility") == QJsonValue::Undefined)
                throw DeserializationException("history_visibility key is missing");

        auto value = object.value("history_visibility").toString();

        if (value == "invited")
                history_visibility_ = HistoryVisibility::Invited;
        else if (value == "joined")
                history_visibility_ = HistoryVisibility::Joined;
        else if (value == "shared")
                history_visibility_ = HistoryVisibility::Shared;
        else if (value == "world_readable")
                history_visibility_ = HistoryVisibility::WorldReadable;
        else
                throw DeserializationException(
                  QString("Unknown history_visibility value: %1").arg(value).toUtf8().constData());
}

QJsonObject
HistoryVisibilityEventContent::serialize() const
{
        QJsonObject object;

        if (history_visibility_ == HistoryVisibility::Invited)
                object["history_visibility"] = "invited";
        else if (history_visibility_ == HistoryVisibility::Joined)
                object["history_visibility"] = "joined";
        else if (history_visibility_ == HistoryVisibility::Shared)
                object["history_visibility"] = "shared";
        else if (history_visibility_ == HistoryVisibility::WorldReadable)
                object["history_visibility"] = "world_readable";

        return object;
}
