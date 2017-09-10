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

#include "MessageEventContent.h"

using namespace matrix::events;

MessageEventType
matrix::events::extractMessageEventType(const QJsonObject &data)
{
        if (!data.contains("content"))
                return MessageEventType::Unknown;

        auto content = data.value("content").toObject();
        auto msgtype = content.value("msgtype").toString();

        if (msgtype == "m.audio")
                return MessageEventType::Audio;
        else if (msgtype == "m.emote")
                return MessageEventType::Emote;
        else if (msgtype == "m.file")
                return MessageEventType::File;
        else if (msgtype == "m.image")
                return MessageEventType::Image;
        else if (msgtype == "m.location")
                return MessageEventType::Location;
        else if (msgtype == "m.notice")
                return MessageEventType::Notice;
        else if (msgtype == "m.text")
                return MessageEventType::Text;
        else if (msgtype == "m.video")
                return MessageEventType::Video;
        else
                return MessageEventType::Unknown;
}

void
MessageEventContent::deserialize(const QJsonValue &data)
{
        if (!data.isObject())
                throw DeserializationException("MessageEventContent is not a JSON object");

        auto object = data.toObject();

        if (!object.contains("body"))
                throw DeserializationException("body key is missing");

        body_ = object.value("body").toString();
}

QJsonObject
MessageEventContent::serialize() const
{
        // TODO: Add for all the message contents.
        QJsonObject object;

        return object;
}
