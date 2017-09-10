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

#include "Location.h"

using namespace matrix::events::messages;

void
Location::deserialize(const QJsonObject &object)
{
        if (!object.contains("geo_uri"))
                throw DeserializationException("messages::Location geo_uri key is missing");

        if (object.value("msgtype") != "m.location")
                throw DeserializationException("invalid msgtype for location");

        geo_uri_ = object.value("geo_uri").toString();

        if (object.contains("info")) {
                auto location_info = object.value("info").toObject();

                info_.thumbnail_url = location_info.value("thumbnail_url").toString();

                if (location_info.contains("thumbnail_info")) {
                        auto thumbinfo = location_info.value("thumbnail_info").toObject();

                        info_.thumbnail_info.h        = thumbinfo.value("h").toInt();
                        info_.thumbnail_info.w        = thumbinfo.value("w").toInt();
                        info_.thumbnail_info.size     = thumbinfo.value("size").toInt();
                        info_.thumbnail_info.mimetype = thumbinfo.value("mimetype").toString();
                }
        }
}
