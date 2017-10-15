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

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include "Deserializable.h"
#include "Register.h"

RegisterRequest::RegisterRequest(const QString &username, const QString &password)
  : user_(username)
  , password_(password)
{}

QByteArray
RegisterRequest::serialize() noexcept
{
        QJsonObject body{ { "username", user_ }, { "password", password_ } };

        return QJsonDocument(body).toJson(QJsonDocument::Compact);
}

void
RegisterResponse::deserialize(const QJsonDocument &data)
{
        if (!data.isObject())
                throw DeserializationException("Response is not a JSON object");

        QJsonObject object = data.object();

        if (!object.contains("access_token"))
                throw DeserializationException("Missing access_token param");

        if (!object.contains("home_server"))
                throw DeserializationException("Missing home_server param");

        if (!object.contains("user_id"))
                throw DeserializationException("Missing user_id param");

        access_token_ = object.value("access_token").toString();
        home_server_  = object.value("home_server").toString();
        user_id_      = object.value("user_id").toString();
}
