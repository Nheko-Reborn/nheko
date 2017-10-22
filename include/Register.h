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

#pragma once

#include <QJsonDocument>

#include "Deserializable.h"

class RegisterRequest
{
public:
        RegisterRequest();
        RegisterRequest(const QString &username, const QString &password);

        QByteArray serialize() noexcept;

        void setPassword(QString password) { password_ = password; };
        void setUser(QString username) { user_ = username; };

private:
        QString user_;
        QString password_;
};

class RegisterResponse : public Deserializable
{
public:
        void deserialize(const QJsonDocument &data) override;

        QString getAccessToken() { return access_token_; };
        QString getHomeServer() { return home_server_; };
        QString getUserId() { return user_id_; };

private:
        QString access_token_;
        QString home_server_;
        QString user_id_;
};
