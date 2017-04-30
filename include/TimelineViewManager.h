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

#ifndef HISTORY_VIEW_MANAGER_H
#define HISTORY_VIEW_MANAGER_H

#include <QDebug>
#include <QSharedPointer>
#include <QStackedWidget>
#include <QWidget>

#include "MatrixClient.h"
#include "RoomInfo.h"
#include "Sync.h"
#include "TimelineView.h"

class TimelineViewManager : public QStackedWidget
{
	Q_OBJECT

public:
	TimelineViewManager(QSharedPointer<MatrixClient> client, QWidget *parent);
	~TimelineViewManager();

	void initialize(const Rooms &rooms);
	void sync(const Rooms &rooms);
	void clearAll();

	static QString chooseRandomColor();
	static QString getUserColor(const QString &userid);
	static QMap<QString, QString> NICK_COLORS;

signals:
	void unreadMessages(QString roomid, int count);

public slots:
	void setHistoryView(const RoomInfo &info);
	void sendTextMessage(const QString &msg);

private slots:
	void messageSent(const QString &eventid, const QString &roomid, int txnid);

private:
	RoomInfo active_room_;
	QMap<QString, TimelineView *> views_;
	QSharedPointer<MatrixClient> client_;
};

#endif
