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

#include <QApplication>
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
namespace msgs   = matrix::events::messages;

static bool
isRedactedEvent(const QJsonObject &event)
{
        if (event.contains("redacted_because"))
                return true;

        if (event.contains("unsigned") &&
            event.value("unsigned").toObject().contains("redacted_because"))
                return true;

        return false;
}

TimelineView::TimelineView(const Timeline &timeline,
                           QSharedPointer<MatrixClient> client,
                           const QString &room_id,
                           QWidget *parent)
  : QWidget(parent)
  , room_id_{ room_id }
  , client_{ client }
{
        init();
        addEvents(timeline);
}

TimelineView::TimelineView(QSharedPointer<MatrixClient> client,
                           const QString &room_id,
                           QWidget *parent)
  : QWidget(parent)
  , room_id_{ room_id }
  , client_{ client }
{
        init();
        client_->messages(room_id_, "");
}

void
TimelineView::sliderRangeChanged(int min, int max)
{
        Q_UNUSED(min);

        if (!scroll_area_->verticalScrollBar()->isVisible()) {
                scroll_area_->verticalScrollBar()->setValue(max);
                return;
        }

        // If the scrollbar is close to the bottom and a new message
        // is added we move the scrollbar.
        if (max - scroll_area_->verticalScrollBar()->value() < SCROLL_BAR_GAP) {
                scroll_area_->verticalScrollBar()->setValue(max);
                return;
        }

        int currentHeight = scroll_widget_->size().height();
        int diff          = currentHeight - oldHeight_;
        int newPosition   = oldPosition_ + diff;

        // Keep the scroll bar to the bottom if it hasn't been activated yet.
        if (oldPosition_ == 0 && !scroll_area_->verticalScrollBar()->isVisible())
                newPosition = max;

        if (lastMessageDirection_ == TimelineDirection::Top)
                scroll_area_->verticalScrollBar()->setValue(newPosition);
}

void
TimelineView::fetchHistory()
{
        bool hasEnoughMessages = scroll_area_->verticalScrollBar()->isVisible();

        if (!hasEnoughMessages && !isTimelineFinished) {
                isPaginationInProgress_ = true;
                client_->messages(room_id_, prev_batch_token_);
                paginationTimer_->start(500);
                return;
        }

        paginationTimer_->stop();
}

void
TimelineView::scrollDown()
{
        int current = scroll_area_->verticalScrollBar()->value();
        int max     = scroll_area_->verticalScrollBar()->maximum();

        // The first time we enter the room move the scroll bar to the bottom.
        if (!isInitialized) {
                scroll_area_->verticalScrollBar()->setValue(max);
                isInitialized = true;
                return;
        }

        // If the gap is small enough move the scroll bar down. e.g when a new
        // message appears.
        if (max - current < SCROLL_BAR_GAP)
                scroll_area_->verticalScrollBar()->setValue(max);
}

void
TimelineView::sliderMoved(int position)
{
        if (!scroll_area_->verticalScrollBar()->isVisible())
                return;

        // The scrollbar is high enough so we can start retrieving old events.
        if (position < SCROLL_BAR_GAP) {
                if (isTimelineFinished)
                        return;

                // Prevent user from moving up when there is pagination in
                // progress.
                // TODO: Keep a map of the event ids to filter out duplicates.
                if (isPaginationInProgress_)
                        return;

                isPaginationInProgress_ = true;

                // FIXME: Maybe move this to TimelineViewManager to remove the
                // extra calls?
                client_->messages(room_id_, prev_batch_token_);
        }
}

void
TimelineView::addBackwardsEvents(const QString &room_id, const RoomMessages &msgs)
{
        if (room_id_ != room_id)
                return;

        if (msgs.chunk().count() == 0) {
                isTimelineFinished = true;
                return;
        }

        isTimelineFinished = false;
        QList<TimelineItem *> items;

        // Parse in reverse order to determine where we should not show sender's
        // name.
        auto ii = msgs.chunk().size();
        while (ii != 0) {
                --ii;

                TimelineItem *item =
                  parseMessageEvent(msgs.chunk().at(ii).toObject(), TimelineDirection::Top);

                if (item != nullptr)
                        items.push_back(item);
        }

        // Reverse again to render them.
        std::reverse(items.begin(), items.end());

        oldPosition_ = scroll_area_->verticalScrollBar()->value();
        oldHeight_   = scroll_widget_->size().height();

        for (const auto &item : items)
                addTimelineItem(item, TimelineDirection::Top);

        lastMessageDirection_ = TimelineDirection::Top;

        QApplication::processEvents();

        prev_batch_token_       = msgs.end();
        isPaginationInProgress_ = false;

        // Exclude the top stretch.
        if (!msgs.chunk().isEmpty() && scroll_layout_->count() > 1)
                notifyForLastEvent();

        // If this batch is the first being rendered (i.e the first and the last
        // events originate from this batch), set the last sender.
        if (lastSender_.isEmpty() && !items.isEmpty())
                lastSender_ = items.constFirst()->descriptionMessage().userid;
}

TimelineItem *
TimelineView::parseMessageEvent(const QJsonObject &event, TimelineDirection direction)
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

                        if (isDuplicate(text.eventId()))
                                return nullptr;

                        eventIds_[text.eventId()] = true;

                        if (isPendingMessage(
                              text.eventId(), text.content().body(), text.sender(), local_user_)) {
                                removePendingMessage(text.eventId(), text.content().body());
                                return nullptr;
                        }

                        auto with_sender = isSenderRendered(text.sender(), direction);

                        updateLastSender(text.sender(), direction);

                        return createTimelineItem(text, with_sender);
                } else if (msg_type == events::MessageEventType::Notice) {
                        events::MessageEvent<msgs::Notice> notice;

                        try {
                                notice.deserialize(event);
                        } catch (const DeserializationException &e) {
                                qWarning() << e.what() << event;
                                return nullptr;
                        }

                        if (isDuplicate(notice.eventId()))
                                return nullptr;

                        eventIds_[notice.eventId()] = true;

                        auto with_sender = isSenderRendered(notice.sender(), direction);

                        updateLastSender(notice.sender(), direction);

                        return createTimelineItem(notice, with_sender);
                } else if (msg_type == events::MessageEventType::Image) {
                        events::MessageEvent<msgs::Image> img;

                        try {
                                img.deserialize(event);
                        } catch (const DeserializationException &e) {
                                qWarning() << e.what() << event;
                                return nullptr;
                        }

                        if (isDuplicate(img.eventId()))
                                return nullptr;

                        eventIds_[img.eventId()] = true;

                        if (isPendingMessage(
                              img.eventId(), img.msgContent().url(), img.sender(), local_user_)) {
                                removePendingMessage(img.eventId(), img.msgContent().url());
                                return nullptr;
                        }

                        auto with_sender = isSenderRendered(img.sender(), direction);

                        updateLastSender(img.sender(), direction);

                        return createTimelineItem(img, with_sender);
                } else if (msg_type == events::MessageEventType::Emote) {
                        events::MessageEvent<msgs::Emote> emote;

                        try {
                                emote.deserialize(event);
                        } catch (const DeserializationException &e) {
                                qWarning() << e.what() << event;
                                return nullptr;
                        }

                        if (isDuplicate(emote.eventId()))
                                return nullptr;

                        eventIds_[emote.eventId()] = true;

                        if (isPendingMessage(emote.eventId(),
                                             emote.content().body(),
                                             emote.sender(),
                                             local_user_)) {
                                removePendingMessage(emote.eventId(), emote.content().body());
                                return nullptr;
                        }

                        auto with_sender = isSenderRendered(emote.sender(), direction);

                        updateLastSender(emote.sender(), direction);

                        return createTimelineItem(emote, with_sender);
                } else if (msg_type == events::MessageEventType::Unknown) {
                        // TODO Handle redacted messages.
                        // Silenced for now.
                        if (!isRedactedEvent(event))
                                qWarning() << "Unknown message type" << event;

                        return nullptr;
                }
        }

        return nullptr;
}

int
TimelineView::addEvents(const Timeline &timeline)
{
        int message_count = 0;

        QSettings settings;
        QString localUser = settings.value("auth/user_id").toString();

        for (const auto &event : timeline.events()) {
                TimelineItem *item = parseMessageEvent(event.toObject(), TimelineDirection::Bottom);

                if (item != nullptr) {
                        addTimelineItem(item, TimelineDirection::Bottom);

                        if (localUser != event.toObject().value("sender").toString())
                                message_count += 1;
                }
        }

        lastMessageDirection_ = TimelineDirection::Bottom;

        QApplication::processEvents();

        if (isInitialSync) {
                prev_batch_token_ = timeline.previousBatch();
                isInitialSync     = false;

                client_->messages(room_id_, prev_batch_token_);
        }

        // Exclude the top stretch.
        if (!timeline.events().isEmpty() && scroll_layout_->count() > 1)
                notifyForLastEvent();

        return message_count;
}

void
TimelineView::init()
{
        QSettings settings;
        local_user_ = settings.value("auth/user_id").toString();

        top_layout_ = new QVBoxLayout(this);
        top_layout_->setSpacing(0);
        top_layout_->setMargin(0);

        scroll_area_ = new QScrollArea(this);
        scroll_area_->setWidgetResizable(true);
        scroll_area_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        scrollbar_ = new ScrollBar(scroll_area_);
        scroll_area_->setVerticalScrollBar(scrollbar_);

        scroll_widget_ = new QWidget(this);

        scroll_layout_ = new QVBoxLayout(scroll_widget_);
        scroll_layout_->addStretch(1);
        scroll_layout_->setSpacing(0);

        scroll_area_->setWidget(scroll_widget_);

        top_layout_->addWidget(scroll_area_);

        setLayout(top_layout_);

        paginationTimer_ = new QTimer(this);
        connect(paginationTimer_, &QTimer::timeout, this, &TimelineView::fetchHistory);

        connect(client_.data(),
                &MatrixClient::messagesRetrieved,
                this,
                &TimelineView::addBackwardsEvents);

        connect(scroll_area_->verticalScrollBar(),
                SIGNAL(valueChanged(int)),
                this,
                SLOT(sliderMoved(int)));
        connect(scroll_area_->verticalScrollBar(),
                SIGNAL(rangeChanged(int, int)),
                this,
                SLOT(sliderRangeChanged(int, int)));
}

void
TimelineView::updateLastSender(const QString &user_id, TimelineDirection direction)
{
        if (direction == TimelineDirection::Bottom)
                lastSender_ = user_id;
        else
                firstSender_ = user_id;
}

bool
TimelineView::isSenderRendered(const QString &user_id, TimelineDirection direction)
{
        if (direction == TimelineDirection::Bottom)
                return lastSender_ != user_id;
        else
                return firstSender_ != user_id;
}

TimelineItem *
TimelineView::createTimelineItem(const events::MessageEvent<msgs::Image> &event, bool with_sender)
{
        auto image = new ImageItem(client_, event);
        auto item  = new TimelineItem(image, event, with_sender, scroll_widget_);

        return item;
}

TimelineItem *
TimelineView::createTimelineItem(const events::MessageEvent<msgs::Notice> &event, bool with_sender)
{
        TimelineItem *item = new TimelineItem(event, with_sender, scroll_widget_);
        return item;
}

TimelineItem *
TimelineView::createTimelineItem(const events::MessageEvent<msgs::Text> &event, bool with_sender)
{
        TimelineItem *item = new TimelineItem(event, with_sender, scroll_widget_);
        return item;
}

TimelineItem *
TimelineView::createTimelineItem(const events::MessageEvent<msgs::Emote> &event, bool with_sender)
{
        TimelineItem *item = new TimelineItem(event, with_sender, scroll_widget_);
        return item;
}

void
TimelineView::addTimelineItem(TimelineItem *item, TimelineDirection direction)
{
        if (direction == TimelineDirection::Bottom)
                scroll_layout_->addWidget(item);
        else
                scroll_layout_->insertWidget(1, item);
}

void
TimelineView::updatePendingMessage(int txn_id, QString event_id)
{
        for (auto &msg : pending_msgs_) {
                if (msg.txn_id == txn_id) {
                        msg.event_id = event_id;
                        break;
                }
        }
}

void
TimelineView::addUserMessage(matrix::events::MessageEventType ty, const QString &body, int txn_id)
{
        QSettings settings;
        auto user_id     = settings.value("auth/user_id").toString();
        auto with_sender = lastSender_ != user_id;

        TimelineItem *view_item = new TimelineItem(ty, user_id, body, with_sender, scroll_widget_);
        scroll_layout_->addWidget(view_item);

        lastMessageDirection_ = TimelineDirection::Bottom;

        QApplication::processEvents();

        lastSender_ = user_id;

        PendingMessage message(txn_id, body, "", view_item);
        pending_msgs_.push_back(message);
}

void
TimelineView::addUserMessage(const QString &url, const QString &filename, int txn_id)
{
        QSettings settings;
        auto user_id     = settings.value("auth/user_id").toString();
        auto with_sender = lastSender_ != user_id;

        auto image = new ImageItem(client_, url, filename, this);

        TimelineItem *view_item = new TimelineItem(image, user_id, with_sender, scroll_widget_);
        scroll_layout_->addWidget(view_item);

        lastMessageDirection_ = TimelineDirection::Bottom;

        QApplication::processEvents();

        lastSender_ = user_id;

        PendingMessage message(txn_id, url, "", view_item);
        pending_msgs_.push_back(message);
}

void
TimelineView::notifyForLastEvent()
{
        auto lastItem          = scroll_layout_->itemAt(scroll_layout_->count() - 1);
        auto *lastTimelineItem = qobject_cast<TimelineItem *>(lastItem->widget());

        if (lastTimelineItem)
                emit updateLastTimelineMessage(room_id_, lastTimelineItem->descriptionMessage());
        else
                qWarning() << "Cast to TimelineView failed" << room_id_;
}

bool
TimelineView::isPendingMessage(const QString &eventid,
                               const QString &body,
                               const QString &sender,
                               const QString &local_userid)
{
        if (sender != local_userid)
                return false;

        for (const auto &msg : pending_msgs_) {
                if (msg.event_id == eventid || msg.body == body)
                        return true;
        }

        return false;
}

void
TimelineView::removePendingMessage(const QString &eventid, const QString &body)
{
        for (auto it = pending_msgs_.begin(); it != pending_msgs_.end(); it++) {
                int index = std::distance(pending_msgs_.begin(), it);

                if (it->event_id == eventid || it->body == body) {
                        pending_msgs_.removeAt(index);
                        break;
                }
        }
}
