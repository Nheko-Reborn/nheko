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
#include <QRegularExpression>

#include "RoomInfoListItem.h"
#include "RoomList.h"
#include "Sync.h"

RoomList::RoomList(QSharedPointer<MatrixClient> client, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::RoomList)
    , client_(client)
{
	ui->setupUi(this);
	ui->scrollVerticalLayout->addStretch(1);

	setStyleSheet("border-top: none");

	connect(client_.data(),
		SIGNAL(roomAvatarRetrieved(const QString &, const QPixmap &)),
		this,
		SLOT(updateRoomAvatar(const QString &, const QPixmap &)));
}

RoomList::~RoomList()
{
	delete ui;
}

void RoomList::clear()
{
	for (const auto &room : rooms_)
		room->deleteLater();

	rooms_.clear();
}

RoomInfo RoomList::extractRoomInfo(const State &room_state)
{
	RoomInfo info;

	auto events = room_state.events();

	for (const auto &event : events) {
		if (event.type() == "m.room.name") {
			info.setName(event.content().value("name").toString());
		} else if (event.type() == "m.room.topic") {
			info.setTopic(event.content().value("topic").toString());
		} else if (event.type() == "m.room.avatar") {
			info.setAvatarUrl(QUrl(event.content().value("url").toString()));
		} else if (event.type() == "m.room.canonical_alias") {
			if (info.name().isEmpty())
				info.setName(event.content().value("alias").toString());
		}
	}

	// Sanitize info for print.
	info.setTopic(info.topic().simplified());
	info.setName(info.name().simplified());

	return info;
}

void RoomList::setInitialRooms(const Rooms &rooms)
{
	rooms_.clear();

	for (auto it = rooms.join().constBegin(); it != rooms.join().constEnd(); it++) {
		RoomInfo info = RoomList::extractRoomInfo(it.value().state());
		info.setId(it.key());

		if (info.name().isEmpty())
			continue;

		if (!info.avatarUrl().isEmpty())
			client_->fetchRoomAvatar(info.id(), info.avatarUrl());

		RoomInfoListItem *room_item = new RoomInfoListItem(info, ui->scrollArea);
		connect(room_item,
			SIGNAL(clicked(const RoomInfo &)),
			this,
			SLOT(highlightSelectedRoom(const RoomInfo &)));

		rooms_.insert(it.key(), room_item);

		int pos = ui->scrollVerticalLayout->count() - 1;
		ui->scrollVerticalLayout->insertWidget(pos, room_item);
	}

	// TODO: Move this into its own function.
	auto first_room = rooms_.first();
	first_room->setPressedState(true);
	emit roomChanged(first_room->info());
}

void RoomList::highlightSelectedRoom(const RoomInfo &info)
{
	emit roomChanged(info);

	for (auto it = rooms_.constBegin(); it != rooms_.constEnd(); it++) {
		if (it.key() != info.id())
			it.value()->setPressedState(false);
	}
}

void RoomList::updateRoomAvatar(const QString &roomid, const QPixmap &img)
{
	if (!rooms_.contains(roomid)) {
		qDebug() << "Avatar update on non existent room" << roomid;
		return;
	}

	auto list_item = rooms_.value(roomid);
	list_item->setAvatar(img.toImage());
}
