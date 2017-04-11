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
#include <QStackedWidget>
#include <QWidget>

#include "HistoryView.h"
#include "RoomInfo.h"
#include "Sync.h"

class HistoryViewManager : public QStackedWidget
{
	Q_OBJECT

public:
	HistoryViewManager(QWidget *parent);
	~HistoryViewManager();

	void initialize(const Rooms &rooms);
	void sync(const Rooms &rooms);
	void clearAll();

	static QString chooseRandomColor();
	static QMap<QString, QString> NICK_COLORS;
	static const QList<QString> COLORS;

public slots:
	void setHistoryView(const RoomInfo &info);

private:
	QMap<QString, HistoryView *> views_;
};

#endif
