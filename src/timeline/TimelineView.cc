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
#include <QFileInfo>
#include <QTimer>

#include "Cache.h"
#include "ChatPage.h"
#include "Config.h"
#include "FloatingButton.h"
#include "UserSettingsPage.h"
#include "Utils.h"

#include "timeline/TimelineView.h"
#include "timeline/widgets/AudioItem.h"
#include "timeline/widgets/FileItem.h"
#include "timeline/widgets/ImageItem.h"
#include "timeline/widgets/VideoItem.h"

using TimelineEvent = mtx::events::collections::TimelineEvents;

DateSeparator::DateSeparator(QDateTime datetime, QWidget *parent)
  : QWidget{parent}
{
        auto now  = QDateTime::currentDateTime();
        auto days = now.daysTo(datetime);

        font_.setWeight(60);
        font_.setPixelSize(conf::timeline::fonts::dateSeparator);

        QString fmt;

        if (now.date().year() != datetime.date().year())
                fmt = QString("ddd d MMMM yy");
        else
                fmt = QString("ddd d MMMM");

        if (days == 0)
                msg_ = tr("Today");
        else if (std::abs(days) == 1)
                msg_ = tr("Yesterday");
        else
                msg_ = datetime.toString(fmt);

        QFontMetrics fm{font_};
        width_  = fm.width(msg_) + HPadding * 2;
        height_ = fm.ascent() + 2 * VPadding;
}

void
DateSeparator::paintEvent(QPaintEvent *)
{
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setFont(font_);

        // Center the box horizontally & vertically.
        auto textRegion = QRectF(width() / 2 - width_ / 2, HMargin, width_, height_);

        QPainterPath ppath;
        ppath.addRoundedRect(textRegion, height_ / 2, height_ / 2);

        p.setPen(Qt::NoPen);
        p.fillPath(ppath, boxColor());
        p.drawPath(ppath);

        p.setPen(QPen(textColor()));
        p.drawText(textRegion, Qt::AlignCenter, msg_);
}

TimelineView::TimelineView(const mtx::responses::Timeline &timeline,
                           QSharedPointer<MatrixClient> client,
                           const QString &room_id,
                           QWidget *parent)
  : QWidget(parent)
  , room_id_{room_id}
  , client_{client}
{
        init();
        addEvents(timeline);
}

TimelineView::TimelineView(QSharedPointer<MatrixClient> client,
                           const QString &room_id,
                           QWidget *parent)
  : QWidget(parent)
  , room_id_{room_id}
  , client_{client}
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
        if (!isScrollbarActivated() && !isTimelineFinished) {
                if (!isVisible())
                        return;

                isPaginationInProgress_ = true;
                client_->messages(room_id_, prev_batch_token_);
                paginationTimer_->start(5000);

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

        toggleScrollDownButton();

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
TimelineView::addBackwardsEvents(const QString &room_id, const mtx::responses::Messages &msgs)
{
        if (room_id_ != room_id)
                return;

        // We've reached the start of the timline and there're no more messages.
        if ((msgs.end == msgs.start) && msgs.chunk.size() == 0) {
                isTimelineFinished = true;
                return;
        }

        isTimelineFinished = false;

        // Queue incoming messages to be rendered later.
        topMessages_.insert(topMessages_.end(),
                            std::make_move_iterator(msgs.chunk.begin()),
                            std::make_move_iterator(msgs.chunk.end()));

        // The RoomList message preview will be updated only if this
        // is the first batch of messages received through /messages
        // i.e there are no other messages currently present.
        if (!topMessages_.empty() && scroll_layout_->count() == 1)
                notifyForLastEvent(findFirstViewableEvent(topMessages_));

        if (isVisible()) {
                renderTopEvents(topMessages_);

                // Free up space for new messages.
                topMessages_.clear();

                // Send a read receipt for the last event.
                if (isActiveWindow())
                        readLastEvent();
        }

        prev_batch_token_       = QString::fromStdString(msgs.end);
        isPaginationInProgress_ = false;
}

TimelineItem *
TimelineView::parseMessageEvent(const mtx::events::collections::TimelineEvents &event,
                                TimelineDirection direction)
{
        namespace msg     = mtx::events::msg;
        using AudioEvent  = mtx::events::RoomEvent<msg::Audio>;
        using EmoteEvent  = mtx::events::RoomEvent<msg::Emote>;
        using FileEvent   = mtx::events::RoomEvent<msg::File>;
        using ImageEvent  = mtx::events::RoomEvent<msg::Image>;
        using NoticeEvent = mtx::events::RoomEvent<msg::Notice>;
        using TextEvent   = mtx::events::RoomEvent<msg::Text>;
        using VideoEvent  = mtx::events::RoomEvent<msg::Video>;

        if (mpark::holds_alternative<mtx::events::RedactionEvent<msg::Redaction>>(event)) {
                auto redaction_event =
                  mpark::get<mtx::events::RedactionEvent<msg::Redaction>>(event);
                const auto event_id = QString::fromStdString(redaction_event.redacts);

                QTimer::singleShot(0, this, [event_id, this]() {
                        if (eventIds_.contains(event_id))
                                removeEvent(event_id);
                });

                return nullptr;
        } else if (mpark::holds_alternative<mtx::events::RoomEvent<msg::Audio>>(event)) {
                auto audio = mpark::get<mtx::events::RoomEvent<msg::Audio>>(event);
                return processMessageEvent<AudioEvent, AudioItem>(audio, direction);
        } else if (mpark::holds_alternative<mtx::events::RoomEvent<msg::Emote>>(event)) {
                auto emote = mpark::get<mtx::events::RoomEvent<msg::Emote>>(event);
                return processMessageEvent<EmoteEvent>(emote, direction);
        } else if (mpark::holds_alternative<mtx::events::RoomEvent<msg::File>>(event)) {
                auto file = mpark::get<mtx::events::RoomEvent<msg::File>>(event);
                return processMessageEvent<FileEvent, FileItem>(file, direction);
        } else if (mpark::holds_alternative<mtx::events::RoomEvent<msg::Image>>(event)) {
                auto image = mpark::get<mtx::events::RoomEvent<msg::Image>>(event);
                return processMessageEvent<ImageEvent, ImageItem>(image, direction);
        } else if (mpark::holds_alternative<mtx::events::RoomEvent<msg::Notice>>(event)) {
                auto notice = mpark::get<mtx::events::RoomEvent<msg::Notice>>(event);
                return processMessageEvent<NoticeEvent>(notice, direction);
        } else if (mpark::holds_alternative<mtx::events::RoomEvent<msg::Text>>(event)) {
                auto text = mpark::get<mtx::events::RoomEvent<msg::Text>>(event);
                return processMessageEvent<TextEvent>(text, direction);
        } else if (mpark::holds_alternative<mtx::events::RoomEvent<msg::Video>>(event)) {
                auto video = mpark::get<mtx::events::RoomEvent<msg::Video>>(event);
                return processMessageEvent<VideoEvent, VideoItem>(video, direction);
        } else if (mpark::holds_alternative<mtx::events::Sticker>(event)) {
                return processMessageEvent<mtx::events::Sticker, StickerItem>(
                  mpark::get<mtx::events::Sticker>(event), direction);
        }

        return nullptr;
}

void
TimelineView::renderBottomEvents(const std::vector<TimelineEvent> &events)
{
        int counter = 0;

        for (const auto &event : events) {
                TimelineItem *item = parseMessageEvent(event, TimelineDirection::Bottom);

                if (item != nullptr) {
                        addTimelineItem(item, TimelineDirection::Bottom);
                        counter++;

                        // Prevent blocking of the event-loop
                        // by calling processEvents every 10 items we render.
                        if (counter % 10 == 0)
                                QApplication::processEvents();
                }
        }

        lastMessageDirection_ = TimelineDirection::Bottom;

        QApplication::processEvents();
}

void
TimelineView::renderTopEvents(const std::vector<TimelineEvent> &events)
{
        std::vector<TimelineItem *> items;

        // Reset the sender of the first message in the timeline
        // cause we're about to insert a new one.
        firstSender_.clear();
        firstMsgTimestamp_ = QDateTime();

        // Parse in reverse order to determine where we should not show sender's
        // name.
        auto ii = events.size();
        while (ii != 0) {
                --ii;

                TimelineItem *item = parseMessageEvent(events[ii], TimelineDirection::Top);

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

        // If this batch is the first being rendered (i.e the first and the last
        // events originate from this batch), set the last sender.
        if (lastSender_.isEmpty() && !items.empty())
                saveLastMessageInfo(items.at(0)->descriptionMessage().userid,
                                    items.at(0)->descriptionMessage().datetime);
}

void
TimelineView::addEvents(const mtx::responses::Timeline &timeline)
{
        if (isInitialSync) {
                prev_batch_token_ = QString::fromStdString(timeline.prev_batch);
                isInitialSync     = false;
        }

        bottomMessages_.insert(bottomMessages_.end(),
                               std::make_move_iterator(timeline.events.begin()),
                               std::make_move_iterator(timeline.events.end()));

        if (!bottomMessages_.empty())
                notifyForLastEvent(findLastViewableEvent(bottomMessages_));

        // If the current timeline is open and there are messages to be rendered.
        if (isVisible() && !bottomMessages_.empty()) {
                renderBottomEvents(bottomMessages_);

                // Free up space for new messages.
                bottomMessages_.clear();

                // Send a read receipt for the last event.
                if (isActiveWindow())
                        readLastEvent();
        }
}

void
TimelineView::init()
{
        QSettings settings;
        local_user_ = settings.value("auth/user_id").toString();

        QIcon icon;
        icon.addFile(":/icons/icons/ui/angle-arrow-down.png");
        scrollDownBtn_ = new FloatingButton(icon, this);
        scrollDownBtn_->setBackgroundColor(QColor("#F5F5F5"));
        scrollDownBtn_->setForegroundColor(QColor("black"));
        scrollDownBtn_->hide();

        connect(scrollDownBtn_, &QPushButton::clicked, this, [this]() {
                const int max = scroll_area_->verticalScrollBar()->maximum();
                scroll_area_->verticalScrollBar()->setValue(max);
        });
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
        scroll_layout_->setContentsMargins(15, 0, 15, 15);
        scroll_layout_->addStretch(1);
        scroll_layout_->setSpacing(0);
        scroll_layout_->setObjectName("timelinescrollarea");

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
TimelineView::isSenderRendered(const QString &user_id,
                               uint64_t origin_server_ts,
                               TimelineDirection direction)
{
        if (direction == TimelineDirection::Bottom) {
                return (lastSender_ != user_id) ||
                       isDateDifference(lastMsgTimestamp_,
                                        QDateTime::fromMSecsSinceEpoch(origin_server_ts));
        } else {
                return (firstSender_ != user_id) ||
                       isDateDifference(firstMsgTimestamp_,
                                        QDateTime::fromMSecsSinceEpoch(origin_server_ts));
        }
}

void
TimelineView::addTimelineItem(TimelineItem *item, TimelineDirection direction)
{
        const auto newDate = item->descriptionMessage().datetime;

        if (direction == TimelineDirection::Bottom) {
                const auto lastItemPosition = scroll_layout_->count() - 1;
                auto lastItem =
                  qobject_cast<TimelineItem *>(scroll_layout_->itemAt(lastItemPosition)->widget());

                if (lastItem) {
                        auto oldDate = lastItem->descriptionMessage().datetime;

                        if (oldDate.daysTo(newDate) != 0) {
                                auto separator = new DateSeparator(newDate, this);

                                if (separator)
                                        scroll_layout_->addWidget(separator);
                        }
                }

                pushTimelineItem(item);
        } else {
                // The first item (position 0) is a stretch widget that pushes
                // the widgets to the bottom of the page.
                if (scroll_layout_->count() > 1) {
                        auto firstItem =
                          qobject_cast<TimelineItem *>(scroll_layout_->itemAt(1)->widget());

                        if (firstItem) {
                                auto oldDate = firstItem->descriptionMessage().datetime;

                                if (newDate.daysTo(oldDate) != 0) {
                                        auto separator = new DateSeparator(oldDate);

                                        if (separator)
                                                scroll_layout_->insertWidget(1, separator);
                                }
                        }
                }

                scroll_layout_->insertWidget(1, item);
        }
}

void
TimelineView::updatePendingMessage(int txn_id, QString event_id)
{
        if (!pending_msgs_.isEmpty() &&
            pending_msgs_.head().txn_id == txn_id) { // We haven't received it yet
                auto msg     = pending_msgs_.dequeue();
                msg.event_id = event_id;

                if (msg.widget) {
                        msg.widget->setEventId(event_id);
                        msg.widget->markReceived();
                        eventIds_[event_id] = msg.widget;
                }

                pending_sent_msgs_.append(msg);
        }

        sendNextPendingMessage();
}

void
TimelineView::addUserMessage(mtx::events::MessageType ty, const QString &body)
{
        auto with_sender = (lastSender_ != local_user_) || isDateDifference(lastMsgTimestamp_);

        TimelineItem *view_item =
          new TimelineItem(ty, local_user_, body, with_sender, room_id_, scroll_widget_);

        addTimelineItem(view_item);

        lastMessageDirection_ = TimelineDirection::Bottom;

        QApplication::processEvents();

        saveLastMessageInfo(local_user_, QDateTime::currentDateTime());

        int txn_id = client_->incrementTransactionId();
        PendingMessage message(ty, txn_id, body, "", "", -1, "", view_item);
        handleNewUserMessage(message);
}

void
TimelineView::handleNewUserMessage(PendingMessage msg)
{
        pending_msgs_.enqueue(msg);
        if (pending_msgs_.size() == 1 && pending_sent_msgs_.isEmpty())
                sendNextPendingMessage();
}

void
TimelineView::sendNextPendingMessage()
{
        if (pending_msgs_.size() == 0)
                return;

        PendingMessage &m = pending_msgs_.head();
        switch (m.ty) {
        case mtx::events::MessageType::Audio:
        case mtx::events::MessageType::Image:
        case mtx::events::MessageType::Video:
        case mtx::events::MessageType::File:
                // FIXME: Improve the API
                client_->sendRoomMessage(
                  m.ty, m.txn_id, room_id_, m.filename, m.mime, m.media_size, m.body);
                break;
        default:
                client_->sendRoomMessage(m.ty, m.txn_id, room_id_, m.body, m.mime, m.media_size);
                break;
        }
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

void
TimelineView::notifyForLastEvent(const TimelineEvent &event)
{
        auto descInfo = utils::getMessageDescription(event, local_user_, room_id_);

        if (!descInfo.timestamp.isEmpty())
                emit updateLastTimelineMessage(room_id_, descInfo);
}

bool
TimelineView::isPendingMessage(const QString &txnid,
                               const QString &sender,
                               const QString &local_userid)
{
        if (sender != local_userid)
                return false;

        auto match_txnid = [txnid](const auto &msg) -> bool {
                return QString::number(msg.txn_id) == txnid;
        };

        return std::any_of(pending_msgs_.cbegin(), pending_msgs_.cend(), match_txnid) ||
               std::any_of(pending_sent_msgs_.cbegin(), pending_sent_msgs_.cend(), match_txnid);
}

void
TimelineView::removePendingMessage(const QString &txnid)
{
        if (txnid.isEmpty())
                return;

        for (auto it = pending_sent_msgs_.begin(); it != pending_sent_msgs_.end(); ++it) {
                if (QString::number(it->txn_id) == txnid) {
                        int index = std::distance(pending_sent_msgs_.begin(), it);
                        pending_sent_msgs_.removeAt(index);

                        if (pending_sent_msgs_.isEmpty())
                                sendNextPendingMessage();

                        return;
                }
        }
        for (auto it = pending_msgs_.begin(); it != pending_msgs_.end(); ++it) {
                if (QString::number(it->txn_id) == txnid) {
                        int index = std::distance(pending_msgs_.begin(), it);
                        pending_msgs_.removeAt(index);
                        return;
                }
        }
}

void
TimelineView::handleFailedMessage(int txnid)
{
        Q_UNUSED(txnid);
        // Note: We do this even if the message has already been echoed.
        QTimer::singleShot(2000, this, SLOT(sendNextPendingMessage()));
}

void
TimelineView::paintEvent(QPaintEvent *)
{
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void
TimelineView::readLastEvent() const
{
        if (!ChatPage::instance()->userSettings()->isReadReceiptsEnabled())
                return;

        const auto eventId = getLastEventId();

        if (!eventId.isEmpty())
                client_->readEvent(room_id_, eventId);
}

QString
TimelineView::getLastEventId() const
{
        auto index = scroll_layout_->count();

        // Search backwards for the first event that has a valid event id.
        while (index > 0) {
                --index;

                auto lastItem          = scroll_layout_->itemAt(index);
                auto *lastTimelineItem = qobject_cast<TimelineItem *>(lastItem->widget());

                if (lastTimelineItem && !lastTimelineItem->eventId().isEmpty())
                        return lastTimelineItem->eventId();
        }

        return QString("");
}

void
TimelineView::showEvent(QShowEvent *event)
{
        if (!topMessages_.empty()) {
                renderTopEvents(topMessages_);
                topMessages_.clear();
        }

        if (!bottomMessages_.empty()) {
                renderBottomEvents(bottomMessages_);
                bottomMessages_.clear();
                scrollDown();
        }

        toggleScrollDownButton();

        readLastEvent();

        QWidget::showEvent(event);
}

bool
TimelineView::event(QEvent *event)
{
        if (event->type() == QEvent::WindowActivate)
                readLastEvent();

        return QWidget::event(event);
}

void
TimelineView::toggleScrollDownButton()
{
        const int maxScroll     = scroll_area_->verticalScrollBar()->maximum();
        const int currentScroll = scroll_area_->verticalScrollBar()->value();

        if (maxScroll - currentScroll > SCROLL_BAR_GAP) {
                scrollDownBtn_->show();
                scrollDownBtn_->raise();
        } else {
                scrollDownBtn_->hide();
        }
}

void
TimelineView::removeEvent(const QString &event_id)
{
        if (!eventIds_.contains(event_id)) {
                qWarning() << "unknown event_id couldn't be removed:" << event_id;
                return;
        }

        auto removedItem = eventIds_[event_id];

        // Find the next and the previous widgets in the timeline
        auto prevWidget = relativeWidget(removedItem, -1);
        auto nextWidget = relativeWidget(removedItem, 1);

        // See if they are timeline items
        auto prevItem = qobject_cast<TimelineItem *>(prevWidget);
        auto nextItem = qobject_cast<TimelineItem *>(nextWidget);

        // ... or a date separator
        auto prevLabel = qobject_cast<DateSeparator *>(prevWidget);

        // If it's a TimelineItem add an avatar.
        if (prevItem) {
                prevItem->addAvatar();
        }

        if (nextItem) {
                nextItem->addAvatar();
        } else if (prevLabel) {
                // If there's no chat message after this, and we have a label before us, delete the
                // label.
                prevLabel->deleteLater();
        }

        // If we deleted the last item in the timeline...
        if (!nextItem && prevItem)
                saveLastMessageInfo(prevItem->descriptionMessage().userid,
                                    prevItem->descriptionMessage().datetime);

        // If we deleted the first item in the timeline...
        if (!prevItem && nextItem)
                saveFirstMessageInfo(nextItem->descriptionMessage().userid,
                                     nextItem->descriptionMessage().datetime);

        // If we deleted the only item in the timeline...
        if (!prevItem && !nextItem) {
                firstSender_.clear();
                firstMsgTimestamp_ = QDateTime();
                lastSender_.clear();
                lastMsgTimestamp_ = QDateTime();
        }

        // Finally remove the event.
        removedItem->deleteLater();
        eventIds_.remove(event_id);

        // Update the room list with a view of the last message after
        // all events have been processed.
        QTimer::singleShot(0, this, [this]() { notifyForLastEvent(); });
}

QWidget *
TimelineView::relativeWidget(TimelineItem *item, int dt) const
{
        int pos = scroll_layout_->indexOf(item);

        if (pos == -1)
                return nullptr;

        pos = pos + dt;

        bool isOutOfBounds = (pos <= 0 || pos > scroll_layout_->count() - 1);

        return isOutOfBounds ? nullptr : scroll_layout_->itemAt(pos)->widget();
}

TimelineEvent
TimelineView::findFirstViewableEvent(const std::vector<TimelineEvent> &events)
{
        auto it = std::find_if(events.begin(), events.end(), [](const auto &event) {
                return mtx::events::EventType::RoomMessage == utils::event_type(event);
        });

        return (it == std::end(events)) ? events.front() : *it;
}

TimelineEvent
TimelineView::findLastViewableEvent(const std::vector<TimelineEvent> &events)
{
        auto it = std::find_if(events.rbegin(), events.rend(), [](const auto &event) {
                return mtx::events::EventType::RoomMessage == utils::event_type(event);
        });

        return (it == std::rend(events)) ? events.back() : *it;
}

void
TimelineView::saveMessageInfo(const QString &sender,
                              uint64_t origin_server_ts,
                              TimelineDirection direction)
{
        updateLastSender(sender, direction);

        if (direction == TimelineDirection::Bottom)
                lastMsgTimestamp_ = QDateTime::fromMSecsSinceEpoch(origin_server_ts);
        else
                firstMsgTimestamp_ = QDateTime::fromMSecsSinceEpoch(origin_server_ts);
}

bool
TimelineView::isDateDifference(const QDateTime &first, const QDateTime &second) const
{
        // Check if the dates are in a different day.
        if (std::abs(first.daysTo(second)) != 0)
                return true;

        const uint64_t diffInSeconds   = std::abs(first.msecsTo(second)) / 1000;
        constexpr uint64_t fifteenMins = 15 * 60;

        return diffInSeconds > fifteenMins;
}
