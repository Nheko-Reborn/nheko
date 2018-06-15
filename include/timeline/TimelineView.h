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
#include <QLayout>
#include <QList>
#include <QQueue>
#include <QScrollArea>
#include <QStyle>
#include <QStyleOption>
#include <QTimer>

#include <mtx/events.hpp>
#include <mtx/responses/messages.hpp>

#include "MatrixClient.h"
#include "ScrollBar.h"
#include "TimelineItem.h"

class StateKeeper
{
public:
        StateKeeper(std::function<void()> &&fn)
          : fn_(std::move(fn))
        {}

        ~StateKeeper() { fn_(); }

private:
        std::function<void()> fn_;
};

class FloatingButton;
struct DescInfo;

// Contains info about a message shown in the history view
// but not yet confirmed by the homeserver through sync.
struct PendingMessage
{
        mtx::events::MessageType ty;
        std::string txn_id;
        QString body;
        QString filename;
        QString mime;
        uint64_t media_size;
        QString event_id;
        TimelineItem *widget;
        bool is_encrypted = false;
};

template<class MessageT>
MessageT
toRoomMessage(const PendingMessage &) = delete;

template<>
mtx::events::msg::Audio
toRoomMessage<mtx::events::msg::Audio>(const PendingMessage &m);

template<>
mtx::events::msg::Emote
toRoomMessage<mtx::events::msg::Emote>(const PendingMessage &m);

template<>
mtx::events::msg::File
toRoomMessage<mtx::events::msg::File>(const PendingMessage &);

template<>
mtx::events::msg::Image
toRoomMessage<mtx::events::msg::Image>(const PendingMessage &m);

template<>
mtx::events::msg::Text
toRoomMessage<mtx::events::msg::Text>(const PendingMessage &);

template<>
mtx::events::msg::Video
toRoomMessage<mtx::events::msg::Video>(const PendingMessage &m);

// In which place new TimelineItems should be inserted.
enum class TimelineDirection
{
        Top,
        Bottom,
};

class DateSeparator : public QWidget
{
        Q_OBJECT

        Q_PROPERTY(QColor textColor WRITE setTextColor READ textColor)
        Q_PROPERTY(QColor boxColor WRITE setBoxColor READ boxColor)

public:
        DateSeparator(QDateTime datetime, QWidget *parent = nullptr);

        void setTextColor(QColor color) { textColor_ = color; }
        void setBoxColor(QColor color) { boxColor_ = color; }

        QColor textColor() const { return textColor_; }
        QColor boxColor() const { return boxColor_; }

protected:
        void paintEvent(QPaintEvent *event) override;

private:
        static constexpr int VPadding = 6;
        static constexpr int HPadding = 12;
        static constexpr int HMargin  = 20;

        int width_;
        int height_;

        QString msg_;
        QFont font_;

        QColor textColor_ = QColor("black");
        QColor boxColor_  = QColor("white");
};

class TimelineView : public QWidget
{
        Q_OBJECT

public:
        TimelineView(const mtx::responses::Timeline &timeline,
                     const QString &room_id,
                     QWidget *parent = 0);
        TimelineView(const QString &room_id, QWidget *parent = 0);

        // Add new events at the end of the timeline.
        void addEvents(const mtx::responses::Timeline &timeline);
        void addUserMessage(mtx::events::MessageType ty, const QString &msg);

        template<class Widget, mtx::events::MessageType MsgType>
        void addUserMessage(const QString &url,
                            const QString &filename,
                            const QString &mime,
                            uint64_t size);
        void updatePendingMessage(const std::string &txn_id, const QString &event_id);
        void scrollDown();
        QLabel *createDateSeparator(QDateTime datetime);

        //! Remove an item from the timeline with the given Event ID.
        void removeEvent(const QString &event_id);

public slots:
        void sliderRangeChanged(int min, int max);
        void sliderMoved(int position);
        void fetchHistory();

        // Add old events at the top of the timeline.
        void addBackwardsEvents(const mtx::responses::Messages &msgs);

        // Whether or not the initial batch has been loaded.
        bool hasLoaded() { return scroll_layout_->count() > 1 || isTimelineFinished; }

        void handleFailedMessage(const std::string &txn_id);

private slots:
        void sendNextPendingMessage();

signals:
        void updateLastTimelineMessage(const QString &user, const DescInfo &info);
        void messagesRetrieved(const mtx::responses::Messages &res);
        void messageFailed(const std::string &txn_id);
        void messageSent(const std::string &txn_id, const QString &event_id);

protected:
        void paintEvent(QPaintEvent *event) override;
        void showEvent(QShowEvent *event) override;
        bool event(QEvent *event) override;

private:
        using TimelineEvent = mtx::events::collections::TimelineEvents;

        QWidget *relativeWidget(TimelineItem *item, int dt) const;

        TimelineEvent parseEncryptedEvent(
          const mtx::events::EncryptedEvent<mtx::events::msg::Encrypted> &e);

        void handleClaimedKeys(std::shared_ptr<StateKeeper> keeper,
                               const std::string &room_key,
                               const DevicePublicKeys &pks,
                               const std::string &user_id,
                               const std::string &device_id,
                               const mtx::responses::ClaimKeys &res,
                               mtx::http::RequestErr err);

        //! Callback for all message sending.
        void sendRoomMessageHandler(const std::string &txn_id,
                                    const mtx::responses::EventId &res,
                                    mtx::http::RequestErr err);
        void prepareEncryptedMessage(const PendingMessage &msg);

        //! Call the /messages endpoint to fill the timeline.
        void getMessages();
        //! HACK: Fixing layout flickering when adding to the bottom
        //! of the timeline.
        void pushTimelineItem(TimelineItem *item)
        {
                item->hide();
                scroll_layout_->addWidget(item);
                QTimer::singleShot(0, this, [item]() { item->show(); });
        };

        //! Decides whether or not to show or hide the scroll down button.
        void toggleScrollDownButton();
        void init();
        void addTimelineItem(TimelineItem *item,
                             TimelineDirection direction = TimelineDirection::Bottom);
        void updateLastSender(const QString &user_id, TimelineDirection direction);
        void notifyForLastEvent();
        void notifyForLastEvent(const TimelineEvent &event);
        //! Keep track of the sender and the timestamp of the current message.
        void saveLastMessageInfo(const QString &sender, const QDateTime &datetime)
        {
                lastSender_       = sender;
                lastMsgTimestamp_ = datetime;
        }
        void saveFirstMessageInfo(const QString &sender, const QDateTime &datetime)
        {
                firstSender_       = sender;
                firstMsgTimestamp_ = datetime;
        }
        //! Keep track of the sender and the timestamp of the current message.
        void saveMessageInfo(const QString &sender,
                             uint64_t origin_server_ts,
                             TimelineDirection direction);

        TimelineEvent findFirstViewableEvent(const std::vector<TimelineEvent> &events);
        TimelineEvent findLastViewableEvent(const std::vector<TimelineEvent> &events);

        //! Mark the last event as read.
        void readLastEvent() const;
        //! Whether or not the scrollbar is visible (non-zero height).
        bool isScrollbarActivated() { return scroll_area_->verticalScrollBar()->value() != 0; }
        //! Retrieve the event id of the last item.
        QString getLastEventId() const;

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
        bool isSenderRendered(const QString &user_id,
                              uint64_t origin_server_ts,
                              TimelineDirection direction);

        bool isPendingMessage(const std::string &txn_id,
                              const QString &sender,
                              const QString &userid);
        void removePendingMessage(const std::string &txn_id);

        bool isDuplicate(const QString &event_id) { return eventIds_.contains(event_id); }

        void handleNewUserMessage(PendingMessage msg);
        bool isDateDifference(const QDateTime &first,
                              const QDateTime &second = QDateTime::currentDateTime()) const;

        // Return nullptr if the event couldn't be parsed.
        TimelineItem *parseMessageEvent(const mtx::events::collections::TimelineEvents &event,
                                        TimelineDirection direction);

        QVBoxLayout *top_layout_;
        QVBoxLayout *scroll_layout_;

        QScrollArea *scroll_area_;
        ScrollBar *scrollbar_;
        QWidget *scroll_widget_;

        QString firstSender_;
        QDateTime firstMsgTimestamp_;
        QString lastSender_;
        QDateTime lastMsgTimestamp_;

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

        //! Messages received by sync not added to the timeline.
        std::vector<TimelineEvent> bottomMessages_;
        //! Messages received by /messages not added to the timeline.
        std::vector<TimelineEvent> topMessages_;

        //! Render the given timeline events to the bottom of the timeline.
        void renderBottomEvents(const std::vector<TimelineEvent> &events);
        //! Render the given timeline events to the top of the timeline.
        void renderTopEvents(const std::vector<TimelineEvent> &events);

        // The events currently rendered. Used for duplicate detection.
        QMap<QString, TimelineItem *> eventIds_;
        QQueue<PendingMessage> pending_msgs_;
        QList<PendingMessage> pending_sent_msgs_;
};

template<class Widget, mtx::events::MessageType MsgType>
void
TimelineView::addUserMessage(const QString &url,
                             const QString &filename,
                             const QString &mime,
                             uint64_t size)
{
        auto with_sender = (lastSender_ != local_user_) || isDateDifference(lastMsgTimestamp_);
        auto trimmed     = QFileInfo{filename}.fileName(); // Trim file path.

        auto widget = new Widget(url, trimmed, size, this);

        TimelineItem *view_item =
          new TimelineItem(widget, local_user_, with_sender, room_id_, scroll_widget_);

        addTimelineItem(view_item);

        lastMessageDirection_ = TimelineDirection::Bottom;

        QApplication::processEvents();

        // Keep track of the sender and the timestamp of the current message.
        saveLastMessageInfo(local_user_, QDateTime::currentDateTime());

        PendingMessage message;
        message.ty         = MsgType;
        message.txn_id     = http::v2::client()->generate_txn_id();
        message.body       = url;
        message.filename   = trimmed;
        message.mime       = mime;
        message.media_size = size;
        message.widget     = view_item;

        handleNewUserMessage(message);
}

template<class Event>
TimelineItem *
TimelineView::createTimelineItem(const Event &event, bool withSender)
{
        TimelineItem *item = new TimelineItem(event, withSender, room_id_, scroll_widget_);
        return item;
}

template<class Event, class Widget>
TimelineItem *
TimelineView::createTimelineItem(const Event &event, bool withSender)
{
        auto eventWidget = new Widget(event);
        auto item = new TimelineItem(eventWidget, event, withSender, room_id_, scroll_widget_);

        return item;
}

template<class Event>
TimelineItem *
TimelineView::processMessageEvent(const Event &event, TimelineDirection direction)
{
        const auto event_id = QString::fromStdString(event.event_id);
        const auto sender   = QString::fromStdString(event.sender);

        const auto txn_id = event.unsigned_data.transaction_id;
        if ((!txn_id.empty() && isPendingMessage(txn_id, sender, local_user_)) ||
            isDuplicate(event_id)) {
                removePendingMessage(txn_id);
                return nullptr;
        }

        auto with_sender = isSenderRendered(sender, event.origin_server_ts, direction);

        saveMessageInfo(sender, event.origin_server_ts, direction);

        auto item = createTimelineItem<Event>(event, with_sender);

        eventIds_[event_id] = item;

        return item;
}

template<class Event, class Widget>
TimelineItem *
TimelineView::processMessageEvent(const Event &event, TimelineDirection direction)
{
        const auto event_id = QString::fromStdString(event.event_id);
        const auto sender   = QString::fromStdString(event.sender);

        const auto txn_id = event.unsigned_data.transaction_id;
        if ((!txn_id.empty() && isPendingMessage(txn_id, sender, local_user_)) ||
            isDuplicate(event_id)) {
                removePendingMessage(txn_id);
                return nullptr;
        }

        auto with_sender = isSenderRendered(sender, event.origin_server_ts, direction);

        saveMessageInfo(sender, event.origin_server_ts, direction);

        auto item = createTimelineItem<Event, Widget>(event, with_sender);

        eventIds_[event_id] = item;

        return item;
}
