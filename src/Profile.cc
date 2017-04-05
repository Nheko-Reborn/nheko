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
#include <QJsonValue>
#include <QUrl>

#include "Deserializable.h"
#include "Profile.h"

QUrl ProfileResponse::getAvatarUrl()
{
	return avatar_url_;
}

QString ProfileResponse::getDisplayName()
{
	return display_name_;
}

void ProfileResponse::deserialize(QJsonDocument data) throw(DeserializationException)
{
	if (!data.isObject())
		throw DeserializationException("Profile response is not a JSON object");

	QJsonObject object = data.object();

	if (object.value("avatar_url") == QJsonValue::Undefined)
		throw DeserializationException("Profile: missing avatar_url param");

	if (object.value("displayname") == QJsonValue::Undefined)
		throw DeserializationException("Profile: missing displayname param");

	avatar_url_ = QUrl(object.value("avatar_url").toString());
	display_name_ = object.value("displayname").toString();
}
