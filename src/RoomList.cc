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

#include "ui_RoomList.h"

#include <QDebug>
#include <QJsonArray>
#include <QLabel>

#include "RoomInfoListItem.h"
#include "RoomList.h"
#include "Sync.h"

RoomList::RoomList(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::RoomList)
{
	ui->setupUi(this);
}

RoomList::~RoomList()
{
	delete ui;
}

RoomInfo RoomList::extractRoomInfo(const State &room_state)
{
	RoomInfo info;

	auto events = room_state.events();

	for (int i = 0; i < events.count(); i++) {
		if (events[i].type() == "m.room.name") {
			info.setName(events[i].content().value("name").toString());
		} else if (events[i].type() == "m.room.topic") {
			info.setTopic(events[i].content().value("topic").toString());
		} else if (events[i].type() == "m.room.avatar") {
			info.setAvatarUrl(QUrl(events[i].content().value("url").toString()));
		} else if (events[i].type() == "m.room.canonical_alias") {
			if (info.name().isEmpty())
				info.setName(events[i].content().value("alias").toString());
		}
	}

	return info;
}

void RoomList::setInitialRooms(const Rooms &rooms)
{
	available_rooms_.clear();

	for (auto it = rooms.join().constBegin(); it != rooms.join().constEnd(); it++) {
		RoomInfo info = RoomList::extractRoomInfo(it.value().state());
		info.setId(it.key());

		if (info.name().isEmpty())
			continue;

		if (!info.avatarUrl().isEmpty())
			emit fetchRoomAvatar(info.id(), info.avatarUrl());

		RoomInfoListItem *room_item = new RoomInfoListItem(info, ui->scrollArea);
		connect(room_item,
			SIGNAL(clicked(const RoomInfo &)),
			this,
			SLOT(highlightSelectedRoom(const RoomInfo &)));

		available_rooms_.insert(it.key(), room_item);

		ui->scrollVerticalLayout->addWidget(room_item);
	}

	// TODO: Move this into its own function.
	auto first_room = available_rooms_.first();
	first_room->setPressedState(true);
	emit roomChanged(first_room->info());

	ui->scrollVerticalLayout->addStretch(1);
}

void RoomList::highlightSelectedRoom(const RoomInfo &info)
{
	emit roomChanged(info);

	for (auto it = available_rooms_.constBegin(); it != available_rooms_.constEnd(); it++) {
		if (it.key() != info.id())
			it.value()->setPressedState(false);
	}
}

void RoomList::updateRoomAvatar(const QString &roomid, const QImage &avatar_image)
{
	if (!available_rooms_.contains(roomid)) {
		qDebug() << "Avatar update on non existent room" << roomid;
		return;
	}

	auto list_item = available_rooms_.value(roomid);
	list_item->setAvatar(avatar_image);
}

void RoomList::appendRoom(QString name)
{
	Q_UNUSED(name);
}
