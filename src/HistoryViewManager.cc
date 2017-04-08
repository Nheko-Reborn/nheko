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

#include <QDebug>
#include <QStackedWidget>
#include <QWidget>

#include "HistoryView.h"
#include "HistoryViewManager.h"

HistoryViewManager::HistoryViewManager(QWidget *parent)
    : QStackedWidget(parent)
{
	setStyleSheet(
		"QWidget {background: #171919; color: #ebebeb;}"
		"QScrollBar:vertical { background-color: #171919; width: 10px; border-radius: 20px; margin: 0px 2px 0 2px; }"
		"QScrollBar::handle:vertical { border-radius : 50px; background-color : #1c3133; }"
		"QScrollBar::add-line:vertical { border: none; background: none; }"
		"QScrollBar::sub-line:vertical { border: none; background: none; }");
}

HistoryViewManager::~HistoryViewManager()
{
}

void HistoryViewManager::clearAll()
{
	for (const auto &view: views_) {
		view->clear();
		removeWidget(view);
		view->deleteLater();
	}

	views_.clear();
}

void HistoryViewManager::initialize(const Rooms &rooms)
{
	for (auto it = rooms.join().constBegin(); it != rooms.join().constEnd(); it++) {
		auto roomid = it.key();
		auto events = it.value().timeline().events();

		// Create a history view with the room events.
		HistoryView *view = new HistoryView(events);
		views_.insert(it.key(), view);

		// Add the view in the widget stack.
		addWidget(view);
	}
}

void HistoryViewManager::sync(const Rooms &rooms)
{
	for (auto it = rooms.join().constBegin(); it != rooms.join().constEnd(); it++) {
		auto roomid = it.key();

		if (!views_.contains(roomid)) {
			qDebug() << "Ignoring event from unknown room";
			continue;
		}

		auto view = views_.value(roomid);
		auto events = it.value().timeline().events();

		view->addEvents(events);
	}
}

void HistoryViewManager::setHistoryView(const RoomInfo &info)
{
	if (!views_.contains(info.id())) {
		qDebug() << "Room List id is not present in view manager";
		qDebug() << info.name();
		return;
	}

	auto widget = views_.value(info.id());

	setCurrentWidget(widget);
}
