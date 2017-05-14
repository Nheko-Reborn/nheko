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

	setStyleSheet(
		"QWidget { border: none; }"
		"QScrollBar:vertical { width: 4px; margin: 2px 0; }");

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
	rooms_.clear();
}

void RoomList::updateUnreadMessageCount(const QString &roomid, int count)
{
	if (!rooms_.contains(roomid)) {
		qWarning() << "UpdateUnreadMessageCount: Unknown roomid";
		return;
	}

	rooms_[roomid]->updateUnreadMessageCount(count);

	calculateUnreadMessageCount();
}

void RoomList::calculateUnreadMessageCount()
{
	int total_unread_msgs = 0;

	for (const auto &room : rooms_)
		total_unread_msgs += room->unreadMessageCount();

	emit totalUnreadMessageCountUpdated(total_unread_msgs);
}

void RoomList::setInitialRooms(const QMap<QString, RoomState> &states)
{
	rooms_.clear();

	for (auto it = states.constBegin(); it != states.constEnd(); it++) {
		auto room_id = it.key();
		auto state = it.value();

		if (!state.avatar.content().url().toString().isEmpty())
			client_->fetchRoomAvatar(room_id, state.avatar.content().url());

		RoomInfoListItem *room_item = new RoomInfoListItem(state, room_id, ui->scrollArea);
		connect(room_item, &RoomInfoListItem::clicked, this, &RoomList::highlightSelectedRoom);

		rooms_.insert(room_id, QSharedPointer<RoomInfoListItem>(room_item));

		int pos = ui->scrollVerticalLayout->count() - 1;
		ui->scrollVerticalLayout->insertWidget(pos, room_item);
	}

	if (rooms_.isEmpty())
		return;

	auto first_room = rooms_.first();
	first_room->setPressedState(true);

	emit roomChanged(rooms_.firstKey());
}

void RoomList::sync(const QMap<QString, RoomState> &states)
{
	for (auto it = states.constBegin(); it != states.constEnd(); it++) {
		auto room_id = it.key();
		auto state = it.value();

		// TODO: Add the new room to the list.
		if (!rooms_.contains(room_id))
			continue;

		auto room = rooms_[room_id];

		auto current_avatar = room->state().avatar.content().url();
		auto new_avatar = state.avatar.content().url();

		if (current_avatar != new_avatar && !new_avatar.toString().isEmpty())
			client_->fetchRoomAvatar(room_id, new_avatar);

		room->setState(state);
	}
}

void RoomList::highlightSelectedRoom(const QString &room_id)
{
	emit roomChanged(room_id);

	if (!rooms_.contains(room_id)) {
		qDebug() << "RoomList: clicked unknown roomid";
		return;
	}

	// TODO: Send a read receipt for the last event.
	auto room = rooms_[room_id];
	room->clearUnreadMessageCount();

	calculateUnreadMessageCount();

	for (auto it = rooms_.constBegin(); it != rooms_.constEnd(); it++) {
		if (it.key() != room_id)
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
