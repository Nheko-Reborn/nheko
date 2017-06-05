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

TimelineView::TimelineView(const Timeline &timeline,
			   QSharedPointer<MatrixClient> client,
			   const QString &room_id,
			   QWidget *parent)
    : QWidget(parent)
    , room_id_{room_id}
    , client_{client}
{
	QSettings settings;
	local_user_ = settings.value("auth/user_id").toString();

	init();
	addEvents(timeline);
}

void TimelineView::sliderRangeChanged(int min, int max)
{
	Q_UNUSED(min);

	if (!scroll_area_->verticalScrollBar()->isVisible())
		return;

	if (max - scroll_area_->verticalScrollBar()->value() < SCROLL_BAR_GAP)
		scroll_area_->verticalScrollBar()->setValue(max);

	if (isPaginationScrollPending_) {
		isPaginationScrollPending_ = false;

		int currentHeight = scroll_widget_->size().height();
		int diff = currentHeight - oldHeight_;

		scroll_area_->verticalScrollBar()->setValue(oldPosition_ + diff);
	}
}

void TimelineView::scrollDown()
{
	int current = scroll_area_->verticalScrollBar()->value();
	int max = scroll_area_->verticalScrollBar()->maximum();

	// The first time we enter the room move the scroll bar to the bottom.
	if (!isInitialized) {
		scroll_area_->verticalScrollBar()->setValue(max);
		isInitialized = true;
		return;
	}

	// If the gap is small enough move the scroll bar down. e.g when a new message appears.
	if (max - current < SCROLL_BAR_GAP)
		scroll_area_->verticalScrollBar()->setValue(max);
}

void TimelineView::sliderMoved(int position)
{
	if (!scroll_area_->verticalScrollBar()->isVisible())
		return;

	// The scrollbar is high enough so we can start retrieving old events.
	if (position < SCROLL_BAR_GAP) {
		if (isTimelineFinished)
			return;

		// Prevent user from moving up when there is pagination in progress.
		// TODO: Keep a map of the event ids to filter out duplicates.
		if (isPaginationInProgress_)
			return;

		isPaginationInProgress_ = true;

		// FIXME: Maybe move this to TimelineViewManager to remove the extra calls?
		client_->messages(room_id_, prev_batch_token_);
	}
}

void TimelineView::addBackwardsEvents(const QString &room_id, const RoomMessages &msgs)
{
	if (room_id_ != room_id)
		return;

	if (msgs.chunk().count() == 0) {
		isTimelineFinished = true;
		return;
	}

	isTimelineFinished = false;
	last_sender_backwards_.clear();
	QList<TimelineItem *> items;

	// Parse in reverse order to determine where we should not show sender's name.
	auto it = msgs.chunk().constEnd();
	while (it != msgs.chunk().constBegin()) {
		--it;

		TimelineItem *item = parseMessageEvent((*it).toObject(), TimelineDirection::Top);

		if (item != nullptr)
			items.push_back(item);
	}

	// Reverse again to render them.
	std::reverse(items.begin(), items.end());

	oldPosition_ = scroll_area_->verticalScrollBar()->value();
	oldHeight_ = scroll_widget_->size().height();

	for (const auto &item : items)
		addTimelineItem(item, TimelineDirection::Top);

	prev_batch_token_ = msgs.end();
	isPaginationInProgress_ = false;
	isPaginationScrollPending_ = true;
}

TimelineItem *TimelineView::parseMessageEvent(const QJsonObject &event, TimelineDirection direction)
{
	events::EventType ty = events::extractEventType(event);

	if (ty == events::EventType::RoomMessage) {
		events::MessageEventType msg_type = events::extractMessageEventType(event);

		if (msg_type == events::MessageEventType::Text) {
			events::MessageEvent<msgs::Text> text;

			try {
				text.deserialize(event);
			} catch (const DeserializationException &e) {
				qWarning() << e.what() << event;
				return nullptr;
			}

			if (isPendingMessage(text, local_user_)) {
				removePendingMessage(text);
				return nullptr;
			}

			auto with_sender = isSenderRendered(text.sender(), direction);
			updateLastSender(text.sender(), direction);

			auto color = TimelineViewManager::getUserColor(text.sender());
			last_sender_ = text.sender();

			return createTimelineItem(text, color, with_sender);
		} else if (msg_type == events::MessageEventType::Notice) {
			events::MessageEvent<msgs::Notice> notice;

			try {
				notice.deserialize(event);
			} catch (const DeserializationException &e) {
				qWarning() << e.what() << event;
				return nullptr;
			}

			auto with_sender = isSenderRendered(notice.sender(), direction);
			updateLastSender(notice.sender(), direction);

			auto color = TimelineViewManager::getUserColor(notice.sender());
			last_sender_ = notice.sender();

			return createTimelineItem(notice, color, with_sender);
		} else if (msg_type == events::MessageEventType::Image) {
			events::MessageEvent<msgs::Image> img;

			try {
				img.deserialize(event);
			} catch (const DeserializationException &e) {
				qWarning() << e.what() << event;
				return nullptr;
			}

			auto with_sender = isSenderRendered(img.sender(), direction);
			updateLastSender(img.sender(), direction);

			auto color = TimelineViewManager::getUserColor(img.sender());
			last_sender_ = img.sender();

			return createTimelineItem(img, color, with_sender);
		} else if (msg_type == events::MessageEventType::Unknown) {
			qWarning() << "Unknown message type" << event;
			return nullptr;
		}
	}

	return nullptr;
}

int TimelineView::addEvents(const Timeline &timeline)
{
	int message_count = 0;

	QSettings settings;
	QString localUser = settings.value("auth/user_id").toString();

	for (const auto &event : timeline.events()) {
		TimelineItem *item = parseMessageEvent(event.toObject(), TimelineDirection::Bottom);
		auto sender = event.toObject().value("sender").toString();

		if (item != nullptr) {
			addTimelineItem(item, TimelineDirection::Bottom);

			if (sender != localUser)
				message_count += 1;
		}
	}

	if (isInitialSync) {
		prev_batch_token_ = timeline.previousBatch();
		isInitialSync = false;

		client_->messages(room_id_, prev_batch_token_);
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

	scrollbar_ = new ScrollBar(scroll_area_);
	scroll_area_->setVerticalScrollBar(scrollbar_);

	scroll_widget_ = new QWidget();

	scroll_layout_ = new QVBoxLayout();
	scroll_layout_->addStretch(1);
	scroll_layout_->setSpacing(0);

	scroll_widget_->setLayout(scroll_layout_);

	scroll_area_->setWidget(scroll_widget_);

	top_layout_->addWidget(scroll_area_);

	setLayout(top_layout_);

	connect(client_.data(), &MatrixClient::messagesRetrieved, this, &TimelineView::addBackwardsEvents);

	connect(scroll_area_->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(sliderMoved(int)));
	connect(scroll_area_->verticalScrollBar(), SIGNAL(rangeChanged(int, int)), this, SLOT(sliderRangeChanged(int, int)));
}

void TimelineView::updateLastSender(const QString &user_id, TimelineDirection direction)
{
	if (direction == TimelineDirection::Bottom)
		last_sender_ = user_id;
	else
		last_sender_backwards_ = user_id;
}

bool TimelineView::isSenderRendered(const QString &user_id, TimelineDirection direction)
{
	if (direction == TimelineDirection::Bottom)
		return last_sender_ != user_id;
	else
		return last_sender_backwards_ != user_id;
}

TimelineItem *TimelineView::createTimelineItem(const events::MessageEvent<msgs::Image> &event, const QString &color, bool with_sender)
{
	auto image = new ImageItem(client_, event);

	if (with_sender) {
		auto item = new TimelineItem(image, event, color, scroll_widget_);
		return item;
	}

	auto item = new TimelineItem(image, event, scroll_widget_);
	return item;
}

TimelineItem *TimelineView::createTimelineItem(const events::MessageEvent<msgs::Notice> &event, const QString &color, bool with_sender)
{
	TimelineItem *item = new TimelineItem(event, with_sender, color, scroll_widget_);
	return item;
}

TimelineItem *TimelineView::createTimelineItem(const events::MessageEvent<msgs::Text> &event, const QString &color, bool with_sender)
{
	TimelineItem *item = new TimelineItem(event, with_sender, color, scroll_widget_);
	return item;
}

void TimelineView::addTimelineItem(TimelineItem *item, TimelineDirection direction)
{
	if (direction == TimelineDirection::Bottom)
		scroll_layout_->addWidget(item);
	else
		scroll_layout_->insertWidget(0, item);
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
