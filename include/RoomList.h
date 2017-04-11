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

#ifndef ROOMLIST_H
#define ROOMLIST_H

#include <QImage>
#include <QSharedPointer>
#include <QUrl>
#include <QWidget>

#include "MatrixClient.h"
#include "RoomInfo.h"
#include "RoomInfoListItem.h"
#include "Sync.h"

namespace Ui
{
class RoomList;
}

class RoomList : public QWidget
{
	Q_OBJECT

public:
	RoomList(QSharedPointer<MatrixClient> client, QWidget *parent = 0);
	~RoomList();

	void setInitialRooms(const Rooms &rooms);
	void clear();

	RoomInfo extractRoomInfo(const State &room_state);

signals:
	void roomChanged(const RoomInfo &info);

public slots:
	void updateRoomAvatar(const QString &roomid, const QPixmap &img);
	void highlightSelectedRoom(const RoomInfo &info);

private:
	Ui::RoomList *ui;

	QMap<QString, RoomInfoListItem *> rooms_;

	QSharedPointer<MatrixClient> client_;
};

#endif  // ROOMLIST_H
