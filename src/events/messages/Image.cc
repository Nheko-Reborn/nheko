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

#include "Image.h"

using namespace matrix::events::messages;

void
Image::deserialize(const QJsonObject &object)
{
	if (!object.contains("url"))
		throw DeserializationException("messages::Image url key is missing");

	url_ = object.value("url").toString();

	if (object.value("msgtype") != "m.image")
		throw DeserializationException("invalid msgtype for image");

	if (object.contains("info")) {
		auto imginfo = object.value("info").toObject();

		info_.w = imginfo.value("w").toInt();
		info_.h = imginfo.value("h").toInt();
		info_.size = imginfo.value("size").toInt();

		info_.mimetype = imginfo.value("mimetype").toString();
		info_.thumbnail_url = imginfo.value("thumbnail_url").toString();

		if (imginfo.contains("thumbnail_info")) {
			auto thumbinfo = imginfo.value("thumbnail_info").toObject();

			info_.thumbnail_info.h = thumbinfo.value("h").toInt();
			info_.thumbnail_info.w = thumbinfo.value("w").toInt();
			info_.thumbnail_info.size = thumbinfo.value("size").toInt();
			info_.thumbnail_info.mimetype = thumbinfo.value("mimetype").toString();
		}
	}
}
