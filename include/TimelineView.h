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

#include <QHBoxLayout>
#include <QList>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>

#include "ScrollBar.h"
#include "Sync.h"
#include "TimelineItem.h"

#include "Emote.h"
#include "Image.h"
#include "MessageEvent.h"
#include "Notice.h"
#include "RoomInfoListItem.h"
#include "Text.h"

namespace msgs   = matrix::events::messages;
namespace events = matrix::events;

// Contains info about a message shown in the history view
// but not yet confirmed by the homeserver through sync.
struct PendingMessage
{
        int txn_id;
        QString body;
        QString event_id;
        TimelineItem *widget;

        PendingMessage(int txn_id, QString body, QString event_id, TimelineItem *widget)
          : txn_id(txn_id)
          , body(body)
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
        TimelineView(const Timeline &timeline,
                     QSharedPointer<MatrixClient> client,
                     const QString &room_id,
                     QWidget *parent = 0);
        TimelineView(QSharedPointer<MatrixClient> client,
                     const QString &room_id,
                     QWidget *parent = 0);

        TimelineItem *createTimelineItem(const events::MessageEvent<msgs::Image> &e,
                                         bool with_sender);
        TimelineItem *createTimelineItem(const events::MessageEvent<msgs::Notice> &e,
                                         bool with_sender);
        TimelineItem *createTimelineItem(const events::MessageEvent<msgs::Text> &e,
                                         bool with_sender);
        TimelineItem *createTimelineItem(const events::MessageEvent<msgs::Emote> &e,
                                         bool with_sender);

        // Add new events at the end of the timeline.
        int addEvents(const Timeline &timeline);
        void addUserMessage(matrix::events::MessageEventType ty, const QString &msg, int txn_id);
        void addUserMessage(const QString &url, const QString &filename, int txn_id);
        void updatePendingMessage(int txn_id, QString event_id);
        void scrollDown();

public slots:
        void sliderRangeChanged(int min, int max);
        void sliderMoved(int position);
        void fetchHistory();

        // Add old events at the top of the timeline.
        void addBackwardsEvents(const QString &room_id, const RoomMessages &msgs);

        // Whether or not the initial batch has been loaded.
        bool hasLoaded();

signals:
        void updateLastTimelineMessage(const QString &user, const DescInfo &info);

private:
        void init();
        void addTimelineItem(TimelineItem *item, TimelineDirection direction);
        void updateLastSender(const QString &user_id, TimelineDirection direction);
        void notifyForLastEvent();

        // Used to determine whether or not we should prefix a message with the
        // sender's name.
        bool isSenderRendered(const QString &user_id, TimelineDirection direction);

        bool isPendingMessage(const QString &eventid,
                              const QString &body,
                              const QString &sender,
                              const QString &userid);
        void removePendingMessage(const QString &eventid, const QString &body);

        inline bool isDuplicate(const QString &event_id);

        // Return nullptr if the event couldn't be parsed.
        TimelineItem *parseMessageEvent(const QJsonObject &event, TimelineDirection direction);

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

        const int SCROLL_BAR_GAP = 400;

        QTimer *paginationTimer_;

        int scroll_height_       = 0;
        int previous_max_height_ = 0;

        int oldPosition_;
        int oldHeight_;

        // The events currently rendered. Used for duplicate detection.
        QMap<QString, bool> eventIds_;
        QList<PendingMessage> pending_msgs_;
        QSharedPointer<MatrixClient> client_;
};

inline bool
TimelineView::isDuplicate(const QString &event_id)
{
        return eventIds_.contains(event_id);
}

inline bool
TimelineView::hasLoaded()
{
        return scroll_layout_->count() > 1 || isTimelineFinished;
}
