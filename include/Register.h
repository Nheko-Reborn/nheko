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

#ifndef REGISTER_H
#define REGISTER_H

#include <QJsonDocument>

#include "Deserializable.h"

class RegisterRequest
{
public:
	RegisterRequest();
	RegisterRequest(const QString &username, const QString &password);

	QByteArray serialize();

	inline void setPassword(QString password);
	inline void setUser(QString username);

private:
	QString user_;
	QString password_;
};

inline void RegisterRequest::setPassword(QString password)
{
	password_ = password;
}

inline void RegisterRequest::setUser(QString username)
{
	user_ = username;
}

class RegisterResponse : public Deserializable
{
public:
	void deserialize(const QJsonDocument &data) throw(DeserializationException) override;

	inline QString getAccessToken();
	inline QString getHomeServer();
	inline QString getUserId();

private:
	QString access_token_;
	QString home_server_;
	QString user_id_;
};

inline QString RegisterResponse::getAccessToken()
{
	return access_token_;
}

inline QString RegisterResponse::getHomeServer()
{
	return home_server_;
}

inline QString RegisterResponse::getUserId()
{
	return user_id_;
}

#endif  // REGISTER_H
