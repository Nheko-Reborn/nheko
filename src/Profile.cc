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

#include "Deserializable.h"
#include "Profile.h"

void
ProfileResponse::deserialize(const QJsonDocument &data)
{
        if (!data.isObject())
                throw DeserializationException("Response is not a JSON object");

        QJsonObject object = data.object();

        if (object.contains("avatar_url"))
                avatar_url_ = QUrl(object.value("avatar_url").toString());

        if (object.contains("displayname"))
                display_name_ = object.value("displayname").toString();
}
