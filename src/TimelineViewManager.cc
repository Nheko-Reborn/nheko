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

#include <QApplication>
#include <QDebug>
#include <QSettings>
#include <QStackedWidget>
#include <QWidget>

#include "TimelineView.h"
#include "TimelineViewManager.h"

TimelineViewManager::TimelineViewManager(QSharedPointer<MatrixClient> client, QWidget *parent)
    : QStackedWidget(parent)
    , client_(client)
{
	setStyleSheet("QWidget { background: #f8fbfe; color: #e8e8e8; border: none;}");

	connect(client_.data(),
		SIGNAL(messageSent(const QString &, const QString &, int)),
		this,
		SLOT(messageSent(const QString &, const QString &, int)));
}

TimelineViewManager::~TimelineViewManager()
{
}

void TimelineViewManager::messageSent(const QString &event_id, const QString &roomid, int txn_id)
{
	// We save the latest valid transaction ID for later use.
	QSettings settings;
	settings.setValue("client/transaction_id", txn_id + 1);

	auto view = views_[roomid];
	view->updatePendingMessage(txn_id, event_id);
}

void TimelineViewManager::sendTextMessage(const QString &msg)
{
	auto room_id = active_room_;
	auto view = views_[room_id];

	view->addUserTextMessage(msg, client_->transactionId());
	client_->sendTextMessage(room_id, msg);
}

void TimelineViewManager::clearAll()
{
	NICK_COLORS.clear();

	for (auto view : views_)
		removeWidget(view.data());

	views_.clear();
}

void TimelineViewManager::initialize(const Rooms &rooms)
{
	for (auto it = rooms.join().constBegin(); it != rooms.join().constEnd(); it++) {
		auto roomid = it.key();

		// Create a history view with the room events.
		TimelineView *view = new TimelineView(it.value().timeline(), client_, it.key());
		views_.insert(it.key(), QSharedPointer<TimelineView>(view));

		// Add the view in the widget stack.
		addWidget(view);
	}
}

void TimelineViewManager::sync(const Rooms &rooms)
{
	for (auto it = rooms.join().constBegin(); it != rooms.join().constEnd(); it++) {
		auto roomid = it.key();

		if (!views_.contains(roomid)) {
			qDebug() << "Ignoring event from unknown room" << roomid;
			continue;
		}

		auto view = views_.value(roomid);

		int msgs_added = view->addEvents(it.value().timeline());

		if (msgs_added > 0) {
			// TODO: When the app window gets active the current
			// unread count (if any) should be cleared.
			auto isAppActive = QApplication::activeWindow() != nullptr;

			if (roomid != active_room_ || !isAppActive)
				emit unreadMessages(roomid, msgs_added);
		}
	}
}

void TimelineViewManager::setHistoryView(const QString &room_id)
{
	if (!views_.contains(room_id)) {
		qDebug() << "Room ID from RoomList is not present in ViewManager" << room_id;
		return;
	}

	active_room_ = room_id;
	auto widget = views_.value(room_id);

	setCurrentWidget(widget.data());

	widget->scrollDown();
}

QMap<QString, QString> TimelineViewManager::NICK_COLORS;
QMap<QString, QString> TimelineViewManager::DISPLAY_NAMES;

QString TimelineViewManager::chooseRandomColor()
{
	std::random_device random_device;
	std::mt19937 engine{random_device()};
	std::uniform_real_distribution<float> dist(0, 1);

	float hue = dist(engine);
	float saturation = 0.9;
	float value = 0.7;

	int hue_i = hue * 6;

	float f = hue * 6 - hue_i;

	float p = value * (1 - saturation);
	float q = value * (1 - f * saturation);
	float t = value * (1 - (1 - f) * saturation);

	float r = 0;
	float g = 0;
	float b = 0;

	if (hue_i == 0) {
		r = value;
		g = t;
		b = p;
	} else if (hue_i == 1) {
		r = q;
		g = value;
		b = p;
	} else if (hue_i == 2) {
		r = p;
		g = value;
		b = t;
	} else if (hue_i == 3) {
		r = p;
		g = q;
		b = value;
	} else if (hue_i == 4) {
		r = t;
		g = p;
		b = value;
	} else if (hue_i == 5) {
		r = value;
		g = p;
		b = q;
	}

	int ri = r * 256;
	int gi = g * 256;
	int bi = b * 256;

	QColor color(ri, gi, bi);

	return color.name();
}

QString TimelineViewManager::getUserColor(const QString &userid)
{
	auto color = NICK_COLORS.value(userid);

	if (color.isEmpty()) {
		color = chooseRandomColor();
		NICK_COLORS.insert(userid, color);
	}

	return color;
}

QString TimelineViewManager::displayName(const QString &userid)
{
	if (DISPLAY_NAMES.contains(userid))
		return DISPLAY_NAMES.value(userid);

	return userid;
}
