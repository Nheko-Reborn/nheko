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

#include "RoomInfo.h"

RoomInfo::RoomInfo()
    : name_("")
    , topic_("")
{
}

RoomInfo::RoomInfo(QString name, QString topic, QUrl avatar_url)
    : name_(name)
    , topic_(topic)
    , avatar_url_(avatar_url)
{
}

QString RoomInfo::id() const
{
	return id_;
}

QString RoomInfo::name() const
{
	return name_;
}

QString RoomInfo::topic() const
{
	return topic_;
}

QUrl RoomInfo::avatarUrl() const
{
	return avatar_url_;
}

void RoomInfo::setAvatarUrl(const QUrl &url)
{
	avatar_url_ = url;
}

void RoomInfo::setId(const QString &id)
{
	id_ = id;
}

void RoomInfo::setName(const QString &name)
{
	name_ = name;
}

void RoomInfo::setTopic(const QString &topic)
{
	topic_ = topic;
}
