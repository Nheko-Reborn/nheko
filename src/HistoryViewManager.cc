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

#include <random>

#include <QDebug>
#include <QSettings>
#include <QStackedWidget>
#include <QWidget>

#include "HistoryView.h"
#include "HistoryViewManager.h"

HistoryViewManager::HistoryViewManager(QSharedPointer<MatrixClient> client, QWidget *parent)
    : QStackedWidget(parent)
    , client_(client)
{
	setStyleSheet(
		"QWidget {background: #171919; color: #ebebeb;}"
		"QScrollBar:vertical { background-color: #171919; width: 10px; border-radius: 20px; margin: 0px 2px 0 2px; }"
		"QScrollBar::handle:vertical { border-radius : 50px; background-color : #1c3133; }"
		"QScrollBar::add-line:vertical { border: none; background: none; }"
		"QScrollBar::sub-line:vertical { border: none; background: none; }");

	connect(client_.data(),
		SIGNAL(messageSent(const QString &, const QString &, int)),
		this,
		SLOT(messageSent(const QString &, const QString &, int)));
}

HistoryViewManager::~HistoryViewManager()
{
}

void HistoryViewManager::messageSent(const QString &event_id, const QString &roomid, int txn_id)
{
	// We save the latest valid transaction ID for later use.
	QSettings settings;
	settings.setValue("client/transaction_id", txn_id + 1);

	auto view = views_[roomid];
	view->updatePendingMessage(txn_id, event_id);
}

void HistoryViewManager::sendTextMessage(const QString &msg)
{
	auto room = active_room_;
	auto view = views_[room.id()];

	view->addUserTextMessage(msg, client_->transactionId());
	client_->sendTextMessage(room.id(), msg);
}

void HistoryViewManager::clearAll()
{
	NICK_COLORS.clear();

	for (const auto &view : views_) {
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

	active_room_ = info;
	auto widget = views_.value(info.id());

	setCurrentWidget(widget);
}

QMap<QString, QString> HistoryViewManager::NICK_COLORS;

const QList<QString> HistoryViewManager::COLORS({"#FFF46E",
						 "#A58BFF",
						 "#50C9BA",
						 "#9EE6CF",
						 "#FFDD67",
						 "#2980B9",
						 "#FC993C",
						 "#2772DB",
						 "#CB8589",
						 "#DDE8B9",
						 "#55A44E",
						 "#A9EEE6",
						 "#53B759",
						 "#9E3997",
						 "#5D89D5",
						 "#BB86B7",
						 "#50a0cf",
						 "#3C989F",
						 "#5A4592",
						 "#235e5b",
						 "#d58247",
						 "#e0a729",
						 "#a2b636",
						 "#4BBE2E"});

QString HistoryViewManager::chooseRandomColor()
{
	std::random_device random_device;
	std::mt19937 engine{random_device()};
	std::uniform_int_distribution<int> dist(0, HistoryViewManager::COLORS.size() - 1);

	return HistoryViewManager::COLORS[dist(engine)];
}

QString HistoryViewManager::getUserColor(const QString &userid)
{
	auto color = NICK_COLORS.value(userid);

	if (color.isEmpty()) {
		color = chooseRandomColor();
		NICK_COLORS.insert(userid, color);
	}

	return color;
}
