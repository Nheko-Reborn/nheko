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

#include "File.h"

using namespace matrix::events::messages;

void
File::deserialize(const QJsonObject &object)
{
        if (!object.contains("url"))
                throw DeserializationException("messages::File url key is missing");

        if (object.value("msgtype") != "m.file")
                throw DeserializationException("invalid msgtype for file");

        url_      = object.value("url").toString();
        filename_ = object.value("filename").toString();

        if (object.contains("info")) {
                auto file_info = object.value("info").toObject();

                info_.size          = file_info.value("size").toInt();
                info_.mimetype      = file_info.value("mimetype").toString();
                info_.thumbnail_url = file_info.value("thumbnail_url").toString();

                if (file_info.contains("thumbnail_info")) {
                        auto thumbinfo = file_info.value("thumbnail_info").toObject();

                        info_.thumbnail_info.h        = thumbinfo.value("h").toInt();
                        info_.thumbnail_info.w        = thumbinfo.value("w").toInt();
                        info_.thumbnail_info.size     = thumbinfo.value("size").toInt();
                        info_.thumbnail_info.mimetype = thumbinfo.value("mimetype").toString();
                }
        }
}
