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

#pragma once

#include <QApplication>
#include <QDebug>
#include <QLayout>
#include <QList>
#include <QQueue>
#include <QScrollArea>
#include <QSettings>
#include <QStyle>
#include <QStyleOption>

#include <mtx/events.hpp>
#include <mtx/responses/messages.hpp>

#include "MatrixClient.h"
#include "ScrollBar.h"
#include "TimelineItem.h"

class FloatingButton;
struct DescInfo;

// Contains info about a message shown in the history view
// but not yet confirmed by the homeserver through sync.
struct PendingMessage
{
        mtx::events::MessageType ty;
        int txn_id;
        QString body;
        QString filename;
        QString event_id;
        TimelineItem *widget;

        PendingMessage(mtx::events::MessageType ty,
                       int txn_id,
                       QString body,
                       QString filename,
                       QString event_id,
                       TimelineItem *widget)
          : ty(ty)
          , txn_id(txn_id)
          , body(body)
          , filename(filename)
          , event_id(event_id)
          , widget(widget)
        {}
};

// In which place new TimelineItems should be inserted.
enum class TimelineDirection
{
        Top,
        Bottom,
};

class TimelineView : public QWidget
{
        Q_OBJECT

public:
        TimelineView(const mtx::responses::Timeline &timeline,
                     QSharedPointer<MatrixClient> client,
                     const QString &room_id,
                     QWidget *parent = 0);
        TimelineView(QSharedPointer<MatrixClient> client,
                     const QString &room_id,
                     QWidget *parent = 0);

        // Add new events at the end of the timeline.
        int addEvents(const mtx::responses::Timeline &timeline);
        void addUserMessage(mtx::events::MessageType ty, const QString &msg);

        template<class Widget, mtx::events::MessageType MsgType>
        void addUserMessage(const QString &url, const QString &filename);
        void updatePendingMessage(int txn_id, QString event_id);
        void scrollDown();
        void addDateSeparator(QDateTime datetime, int position);

public slots:
        void sliderRangeChanged(int min, int max);
        void sliderMoved(int position);
        void fetchHistory();

        // Add old events at the top of the timeline.
        void addBackwardsEvents(const QString &room_id, const mtx::responses::Messages &msgs);

        // Whether or not the initial batch has been loaded.
        bool hasLoaded() { return scroll_layout_->count() > 1 || isTimelineFinished; }

        void handleFailedMessage(int txnid);

private slots:
        void sendNextPendingMessage();

signals:
        void updateLastTimelineMessage(const QString &user, const DescInfo &info);
        void clearUnreadMessageCount(const QString &room_id);

protected:
        void paintEvent(QPaintEvent *event) override;
        void showEvent(QShowEvent *event) override;
        bool event(QEvent *event) override;

private:
        void init();
        void addTimelineItem(TimelineItem *item, TimelineDirection direction);
        void updateLastSender(const QString &user_id, TimelineDirection direction);
        void notifyForLastEvent();
        void readLastEvent() const;
        bool isScrollbarActivated() { return scroll_area_->verticalScrollBar()->value() != 0; }
        QString getLastEventId() const;
        QString getEventSender(const mtx::events::collections::TimelineEvents &event) const;

        template<class Event, class Widget>
        TimelineItem *processMessageEvent(const Event &event, TimelineDirection direction);

        // TODO: Remove this eventually.
        template<class Event>
        TimelineItem *processMessageEvent(const Event &event, TimelineDirection direction);

        // For events with custom display widgets.
        template<class Event, class Widget>
        TimelineItem *createTimelineItem(const Event &event, bool withSender);

        // For events without custom display widgets.
        // TODO: All events should have custom widgets.
        template<class Event>
        TimelineItem *createTimelineItem(const Event &event, bool withSender);

        // Used to determine whether or not we should prefix a message with the
        // sender's name.
        bool isSenderRendered(const QString &user_id, TimelineDirection direction);

        bool isPendingMessage(const QString &txnid, const QString &sender, const QString &userid);
        void removePendingMessage(const QString &txnid);

        bool isDuplicate(const QString &event_id) { return eventIds_.contains(event_id); }

        void handleNewUserMessage(PendingMessage msg);

        // Return nullptr if the event couldn't be parsed.
        TimelineItem *parseMessageEvent(const mtx::events::collections::TimelineEvents &event,
                                        TimelineDirection direction);

        QVBoxLayout *top_layout_;
        QVBoxLayout *scroll_layout_;

        QScrollArea *scroll_area_;
        ScrollBar *scrollbar_;
        QWidget *scroll_widget_;

        QString lastSender_;
        QString firstSender_;
        QString room_id_;
        QString prev_batch_token_;
        QString local_user_;

        bool isPaginationInProgress_ = false;

        // Keeps track whether or not the user has visited the view.
        bool isInitialized      = false;
        bool isTimelineFinished = false;
        bool isInitialSync      = true;

        const int SCROLL_BAR_GAP = 200;

        QTimer *paginationTimer_;

        int scroll_height_       = 0;
        int previous_max_height_ = 0;

        int oldPosition_;
        int oldHeight_;

        FloatingButton *scrollDownBtn_;

        TimelineDirection lastMessageDirection_;

        // The events currently rendered. Used for duplicate detection.
        QMap<QString, bool> eventIds_;
        QQueue<PendingMessage> pending_msgs_;
        QList<PendingMessage> pending_sent_msgs_;
        QSharedPointer<MatrixClient> client_;
};

template<class Widget, mtx::events::MessageType MsgType>
void
TimelineView::addUserMessage(const QString &url, const QString &filename)
{
        QSettings settings;
        auto user_id     = settings.value("auth/user_id").toString();
        auto with_sender = lastSender_ != user_id;

        auto widget = new Widget(client_, url, filename, this);

        TimelineItem *view_item = new TimelineItem(widget, user_id, with_sender, scroll_widget_);
        scroll_layout_->addWidget(view_item);

        lastMessageDirection_ = TimelineDirection::Bottom;

        QApplication::processEvents();

        lastSender_ = user_id;

        int txn_id = client_->incrementTransactionId();

        PendingMessage message(MsgType, txn_id, url, filename, "", view_item);
        handleNewUserMessage(message);
}

template<class Event>
TimelineItem *
TimelineView::createTimelineItem(const Event &event, bool withSender)
{
        TimelineItem *item = new TimelineItem(event, withSender, scroll_widget_);
        return item;
}

template<class Event, class Widget>
TimelineItem *
TimelineView::createTimelineItem(const Event &event, bool withSender)
{
        auto eventWidget = new Widget(client_, event);
        auto item        = new TimelineItem(eventWidget, event, withSender, scroll_widget_);

        return item;
}

template<class Event>
TimelineItem *
TimelineView::processMessageEvent(const Event &event, TimelineDirection direction)
{
        const auto event_id = QString::fromStdString(event.event_id);
        const auto sender   = QString::fromStdString(event.sender);

        if (isDuplicate(event_id))
                return nullptr;

        eventIds_[event_id] = true;

        const QString txnid = QString::fromStdString(event.unsigned_data.transaction_id);
        if (!txnid.isEmpty() && isPendingMessage(txnid, sender, local_user_)) {
                removePendingMessage(txnid);
                return nullptr;
        }

        auto with_sender = isSenderRendered(sender, direction);

        updateLastSender(sender, direction);

        return createTimelineItem<Event>(event, with_sender);
}

template<class Event, class Widget>
TimelineItem *
TimelineView::processMessageEvent(const Event &event, TimelineDirection direction)
{
        const auto event_id = QString::fromStdString(event.event_id);
        const auto sender   = QString::fromStdString(event.sender);

        if (isDuplicate(event_id))
                return nullptr;

        eventIds_[event_id] = true;

        const QString txnid = QString::fromStdString(event.unsigned_data.transaction_id);
        if (!txnid.isEmpty() && isPendingMessage(txnid, sender, local_user_)) {
                removePendingMessage(txnid);
                return nullptr;
        }

        auto with_sender = isSenderRendered(sender, direction);

        updateLastSender(sender, direction);

        return createTimelineItem<Event, Widget>(event, with_sender);
}
