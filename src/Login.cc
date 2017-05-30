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
#include "Login.h"

LoginRequest::LoginRequest()
{
}

LoginRequest::LoginRequest(QString username, QString password)
    : user_(username)
    , password_(password)
{
}

QByteArray LoginRequest::serialize() noexcept
{
#if defined(Q_OS_MAC)
	QString initialDeviceName("nheko on Mac OS");
#elif defined(Q_OS_LINUX)
	QString initialDeviceName("nheko on Linux");
#elif defined(Q_OS_WIN)
	QString initialDeviceName("nheko on Windows");
#else
	QString initialDeviceName("nheko");
#endif

	QJsonObject body{
		{"type", "m.login.password"},
		{"user", user_},
		{"password", password_},
		{"initial_device_display_name", initialDeviceName},
	};

	return QJsonDocument(body).toJson(QJsonDocument::Compact);
}

void LoginResponse::deserialize(const QJsonDocument &data)
{
	if (!data.isObject())
		throw DeserializationException("Login response is not a JSON object");

	QJsonObject object = data.object();

	if (object.value("access_token") == QJsonValue::Undefined)
		throw DeserializationException("Login: missing access_token param");

	if (object.value("home_server") == QJsonValue::Undefined)
		throw DeserializationException("Login: missing home_server param");

	if (object.value("user_id") == QJsonValue::Undefined)
		throw DeserializationException("Login: missing user_id param");

	access_token_ = object.value("access_token").toString();
	home_server_ = object.value("home_server").toString();
	user_id_ = object.value("user_id").toString();
}
