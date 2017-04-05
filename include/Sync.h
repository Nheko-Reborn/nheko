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

#ifndef SYNC_H
#define SYNC_H

#include <QJsonDocument>
#include <QMap>
#include <QString>

#include "Deserializable.h"

class Event : public Deserializable
{
public:
	QJsonObject content() const;
	QJsonObject unsigned_content() const;

	QString sender() const;
	QString state_key() const;
	QString type() const;
	QString eventId() const;

	uint64_t timestamp() const;

	void deserialize(QJsonValue data) throw(DeserializationException) override;

private:
	QJsonObject content_;
	QJsonObject unsigned_;

	QString sender_;
	QString state_key_;
	QString type_;
	QString event_id_;

	uint64_t origin_server_ts_;
};

class State : public Deserializable
{
public:
	void deserialize(QJsonValue data) throw(DeserializationException) override;
	QList<Event> events() const;

private:
	QList<Event> events_;
};

class Timeline : public Deserializable
{
public:
	QList<Event> events() const;
	QString previousBatch() const;
	bool limited() const;

	void deserialize(QJsonValue data) throw(DeserializationException) override;

private:
	QList<Event> events_;
	QString prev_batch_;
	bool limited_;
};

// TODO: Add support for ehpmeral, account_data, undread_notifications
class JoinedRoom : public Deserializable
{
public:
	State state() const;
	Timeline timeline() const;

	void deserialize(QJsonValue data) throw(DeserializationException) override;

private:
	State state_;
	Timeline timeline_;
	/* Ephemeral ephemeral_; */
	/* AccountData account_data_; */
	/* UnreadNotifications unread_notifications_; */
};

// TODO: Add support for invited and left rooms.
class Rooms : public Deserializable
{
public:
	QMap<QString, JoinedRoom> join() const;
	void deserialize(QJsonValue data) throw(DeserializationException) override;

private:
	QMap<QString, JoinedRoom> join_;
};

class SyncResponse : public Deserializable
{
public:
	void deserialize(QJsonDocument data) throw(DeserializationException) override;
	QString nextBatch() const;
	Rooms rooms() const;

private:
	QString next_batch_;
	Rooms rooms_;
};

#endif  // SYNC_H
