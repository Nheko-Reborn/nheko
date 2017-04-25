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
#include <QScrollBar>
#include <QSettings>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>

#include "TimelineItem.h"
#include "TimelineView.h"
#include "TimelineViewManager.h"

TimelineView::TimelineView(const QList<Event> &events, QWidget *parent)
    : QWidget(parent)
{
	init();
	addEvents(events);
}

TimelineView::TimelineView(QWidget *parent)
    : QWidget(parent)
{
	init();
}

void TimelineView::clear()
{
	for (const auto msg : scroll_layout_->children())
		msg->deleteLater();
}

void TimelineView::sliderRangeChanged(int min, int max)
{
	Q_UNUSED(min);
	scroll_area_->verticalScrollBar()->setValue(max);
}

int TimelineView::addEvents(const QList<Event> &events)
{
	QSettings settings;
	auto local_user = settings.value("auth/user_id").toString();

	int message_count = 0;

	for (const auto &event : events) {
		if (event.type() == "m.room.message") {
			auto msg_type = event.content().value("msgtype").toString();

			if (isPendingMessage(event, local_user)) {
				removePendingMessage(event);
				continue;
			}

			if (msg_type == "m.text" || msg_type == "m.notice") {
				auto with_sender = last_sender_ != event.sender();
				auto color = TimelineViewManager::getUserColor(event.sender());

				addHistoryItem(event, color, with_sender);
				last_sender_ = event.sender();

				message_count += 1;
			}
		}
	}

	return message_count;
}

void TimelineView::init()
{
	top_layout_ = new QVBoxLayout(this);
	top_layout_->setSpacing(0);
	top_layout_->setMargin(0);

	scroll_area_ = new QScrollArea(this);
	scroll_area_->setWidgetResizable(true);
	scroll_area_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	scroll_widget_ = new QWidget();

	scroll_layout_ = new QVBoxLayout();
	scroll_layout_->addStretch(1);
	scroll_layout_->setSpacing(0);

	scroll_widget_->setLayout(scroll_layout_);

	scroll_area_->setWidget(scroll_widget_);

	top_layout_->addWidget(scroll_area_);

	setLayout(top_layout_);

	connect(scroll_area_->verticalScrollBar(),
		SIGNAL(rangeChanged(int, int)),
		this,
		SLOT(sliderRangeChanged(int, int)));
}

void TimelineView::addHistoryItem(const Event &event, const QString &color, bool with_sender)
{
	TimelineItem *item = new TimelineItem(event, with_sender, color, scroll_widget_);
	scroll_layout_->addWidget(item);
}

void TimelineView::updatePendingMessage(int txn_id, QString event_id)
{
	for (auto &msg : pending_msgs_) {
		if (msg.txn_id == txn_id) {
			msg.event_id = event_id;
			break;
		}
	}
}

bool TimelineView::isPendingMessage(const Event &event, const QString &userid)
{
	if (event.sender() != userid || event.type() != "m.room.message")
		return false;

	auto msgtype = event.content().value("msgtype").toString();
	auto body = event.content().value("body").toString();

	// FIXME: should contain more checks later on for other types of messages.
	if (msgtype != "m.text")
		return false;

	for (const auto &msg : pending_msgs_) {
		if (msg.event_id == event.eventId() || msg.body == body)
			return true;
	}

	return false;
}

void TimelineView::removePendingMessage(const Event &event)
{
	auto body = event.content().value("body").toString();

	for (auto it = pending_msgs_.begin(); it != pending_msgs_.end(); it++) {
		int index = std::distance(pending_msgs_.begin(), it);

		if (it->event_id == event.eventId() || it->body == body) {
			pending_msgs_.removeAt(index);
			break;
		}
	}
}

void TimelineView::addUserTextMessage(const QString &body, int txn_id)
{
	QSettings settings;
	auto user_id = settings.value("auth/user_id").toString();

	auto with_sender = last_sender_ != user_id;
	auto color = TimelineViewManager::getUserColor(user_id);

	TimelineItem *view_item;

	if (with_sender)
		view_item = new TimelineItem(user_id, color, body, scroll_widget_);
	else
		view_item = new TimelineItem(body, scroll_widget_);

	scroll_layout_->addWidget(view_item);

	last_sender_ = user_id;

	PendingMessage message(txn_id, body, "", view_item);

	pending_msgs_.push_back(message);
}

TimelineView::~TimelineView()
{
}
