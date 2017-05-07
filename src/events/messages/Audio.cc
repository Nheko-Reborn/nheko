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

#include "Audio.h"

using namespace matrix::events::messages;

void Audio::deserialize(const QJsonObject &object)
{
	if (!object.contains("url"))
		throw DeserializationException("url key is missing");

	url_ = object.value("url").toString();

	if (object.value("msgtype") != "m.audio")
		throw DeserializationException("invalid msgtype for audio");

	if (object.contains("info")) {
		auto info = object.value("info").toObject();

		info_.duration = info.value("duration").toInt();
		info_.mimetype = info.value("mimetype").toString();
		info_.size = info.value("size").toInt();
	}
}
