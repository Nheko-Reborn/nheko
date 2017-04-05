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

#ifndef ROOM_INFO_H
#define ROOM_INFO_H

#include <QList>
#include <QString>
#include <QUrl>

class RoomInfo
{
public:
	RoomInfo();
	RoomInfo(QString name, QString topic = "", QUrl avatar_url = QUrl(""));

	QString id() const;
	QString name() const;
	QString topic() const;
	QUrl avatarUrl() const;

	void setAvatarUrl(const QUrl &url);
	void setId(const QString &id);
	void setName(const QString &name);
	void setTopic(const QString &name);

private:
	QString id_;
	QString name_;
	QString topic_;
	QUrl avatar_url_;
	QList<QString> aliases_;
};

#endif  // ROOM_INFO_H
