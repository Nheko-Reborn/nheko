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

#ifndef ROOM_MESSAGES_H
#define ROOM_MESSAGES_H

#include <QJsonArray>
#include <QJsonDocument>

#include "Deserializable.h"

class RoomMessages : public Deserializable
{
public:
	void deserialize(const QJsonDocument &data) override;

	inline QString start() const;
	inline QString end() const;
	inline QJsonArray chunk() const;

private:
	QString start_;
	QString end_;
	QJsonArray chunk_;
};

inline QString RoomMessages::start() const
{
	return start_;
}

inline QString RoomMessages::end() const
{
	return end_;
}

inline QJsonArray RoomMessages::chunk() const
{
	return chunk_;
}

#endif  // ROOM_MESSAGES_H
