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

#include <exception>

#include <QJsonDocument>
#include <QJsonObject>

class DeserializationException : public std::exception
{
public:
        explicit DeserializationException(const std::string &msg);
        virtual const char *what() const noexcept;

private:
        std::string msg_;
};

// JSON response structs need to implement the interface.
class Deserializable
{
public:
        virtual void deserialize(const QJsonValue &) {}
        virtual void deserialize(const QJsonObject &) {}
        virtual void deserialize(const QJsonDocument &) {}
        virtual ~Deserializable() {}
};

class Serializable
{
public:
        virtual QJsonObject serialize() const = 0;
        virtual ~Serializable() {}
};
