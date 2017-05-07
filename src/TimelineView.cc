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
#include <QJsonArray>
#include <QScrollBar>
#include <QSettings>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>

#include "Event.h"
#include "MessageEvent.h"
#include "MessageEventContent.h"

#include "ImageItem.h"
#include "TimelineItem.h"
#include "TimelineView.h"
#include "TimelineViewManager.h"

namespace events = matrix::events;
namespace msgs = matrix::events::messages;

TimelineView::TimelineView(const QJsonArray &events, QSharedPointer<MatrixClient> client, QWidget *parent)
    : QWidget(parent)
    , client_{client}
{
	init();
	addEvents(events);
}

TimelineView::TimelineView(QSharedPointer<MatrixClient> client, QWidget *parent)
    : QWidget(parent)
    , client_{client}
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

int TimelineView::addEvents(const QJsonArray &events)
{
	QSettings settings;
	auto local_user = settings.value("auth/user_id").toString();

	int message_count = 0;
	events::EventType ty;

	for (const auto &event : events) {
		ty = events::extractEventType(event.toObject());

		if (ty == events::RoomMessage) {
			events::MessageEventType msg_type = events::extractMessageEventType(event.toObject());

			if (msg_type == events::MessageEventType::Text) {
				events::MessageEvent<msgs::Text> text;

				try {
					text.deserialize(event.toObject());
				} catch (const DeserializationException &e) {
					qWarning() << e.what() << event;
					continue;
				}

				if (isPendingMessage(text, local_user)) {
					removePendingMessage(text);
					continue;
				}

				auto with_sender = last_sender_ != text.sender();
				auto color = TimelineViewManager::getUserColor(text.sender());

				addHistoryItem(text, color, with_sender);
				last_sender_ = text.sender();

				message_count += 1;
			} else if (msg_type == events::MessageEventType::Notice) {
				events::MessageEvent<msgs::Notice> notice;

				try {
					notice.deserialize(event.toObject());
				} catch (const DeserializationException &e) {
					qWarning() << e.what() << event;
					continue;
				}

				auto with_sender = last_sender_ != notice.sender();
				auto color = TimelineViewManager::getUserColor(notice.sender());

				addHistoryItem(notice, color, with_sender);
				last_sender_ = notice.sender();

				message_count += 1;
			} else if (msg_type == events::MessageEventType::Image) {
				events::MessageEvent<msgs::Image> img;

				try {
					img.deserialize(event.toObject());
				} catch (const DeserializationException &e) {
					qWarning() << e.what() << event;
					continue;
				}

				auto with_sender = last_sender_ != img.sender();
				auto color = TimelineViewManager::getUserColor(img.sender());

				addHistoryItem(img, color, with_sender);

				last_sender_ = img.sender();
				message_count += 1;
			} else if (msg_type == events::MessageEventType::Unknown) {
				qWarning() << "Unknown message type" << event.toObject();
				continue;
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

void TimelineView::addHistoryItem(const events::MessageEvent<msgs::Image> &event, const QString &color, bool with_sender)
{
	auto image = new ImageItem(client_, event);

	if (with_sender) {
		auto item = new TimelineItem(image, event, color, scroll_widget_);
		scroll_layout_->addWidget(item);
	} else {
		auto item = new TimelineItem(image, event, scroll_widget_);
		scroll_layout_->addWidget(item);
	}
}

void TimelineView::addHistoryItem(const events::MessageEvent<msgs::Notice> &event, const QString &color, bool with_sender)
{
	TimelineItem *item = new TimelineItem(event, with_sender, color, scroll_widget_);
	scroll_layout_->addWidget(item);
}

void TimelineView::addHistoryItem(const events::MessageEvent<msgs::Text> &event, const QString &color, bool with_sender)
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

bool TimelineView::isPendingMessage(const events::MessageEvent<msgs::Text> &e, const QString &local_userid)
{
	if (e.sender() != local_userid)
		return false;

	for (const auto &msg : pending_msgs_) {
		if (msg.event_id == e.eventId() || msg.body == e.content().body())
			return true;
	}

	return false;
}

void TimelineView::removePendingMessage(const events::MessageEvent<msgs::Text> &e)
{
	for (auto it = pending_msgs_.begin(); it != pending_msgs_.end(); it++) {
		int index = std::distance(pending_msgs_.begin(), it);

		if (it->event_id == e.eventId() || it->body == e.content().body()) {
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
