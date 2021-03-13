// SPDX-FileCopyrightText: 2017 Konstantinos Sideris <siderisk@auth.gr>
// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <limits>
#include <set>

#include <QObject>
#include <QPainter>
#include <QScroller>
#include <QStyle>
#include <QStyleOption>
#include <QTimer>

#include "Logging.h"
#include "MainWindow.h"
#include "RoomInfoListItem.h"
#include "RoomList.h"
#include "UserSettingsPage.h"
#include "Utils.h"
#include "ui/OverlayModal.h"

RoomList::RoomList(QSharedPointer<UserSettings> userSettings, QWidget *parent)
  : QWidget(parent)
{
        topLayout_ = new QVBoxLayout(this);
        topLayout_->setSpacing(0);
        topLayout_->setMargin(0);

        scrollArea_ = new QScrollArea(this);
        scrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scrollArea_->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
        scrollArea_->setWidgetResizable(true);
        scrollArea_->setAlignment(Qt::AlignLeading | Qt::AlignTop | Qt::AlignVCenter);
        scrollArea_->setAttribute(Qt::WA_AcceptTouchEvents);

        QScroller::grabGesture(scrollArea_, QScroller::TouchGesture);
        QScroller::grabGesture(scrollArea_, QScroller::LeftMouseButtonGesture);

// The scrollbar on macOS will hide itself when not active so it won't interfere
// with the content.
#if not defined(Q_OS_MAC)
        scrollArea_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
#endif

        scrollAreaContents_ = new QWidget(this);
        scrollAreaContents_->setObjectName("roomlist_area");

        contentsLayout_ = new QVBoxLayout(scrollAreaContents_);
        contentsLayout_->setAlignment(Qt::AlignTop);
        contentsLayout_->setSpacing(0);
        contentsLayout_->setMargin(0);

        scrollArea_->setWidget(scrollAreaContents_);
        topLayout_->addWidget(scrollArea_);

        connect(this, &RoomList::updateRoomAvatarCb, this, &RoomList::updateRoomAvatar);
        connect(userSettings.data(),
                &UserSettings::roomSortingChanged,
                this,
                &RoomList::sortRoomsByLastMessage);
}

void
RoomList::addRoom(const QString &room_id, const RoomInfo &info)
{
        auto room_item = new RoomInfoListItem(room_id, info, scrollArea_);
        room_item->setRoomName(QString::fromStdString(std::move(info.name)));

        connect(room_item, &RoomInfoListItem::clicked, this, &RoomList::highlightSelectedRoom);
        connect(room_item, &RoomInfoListItem::leaveRoom, this, [](const QString &room_id) {
                MainWindow::instance()->openLeaveRoomDialog(room_id);
        });

        QSharedPointer<RoomInfoListItem> roomWidget(room_item, &QObject::deleteLater);
        rooms_.emplace(room_id, roomWidget);
        rooms_sort_cache_.push_back(roomWidget);

        if (!info.avatar_url.empty())
                updateAvatar(room_id, QString::fromStdString(info.avatar_url));

        int pos = contentsLayout_->count() - 1;
        contentsLayout_->insertWidget(pos, room_item);
}

void
RoomList::updateAvatar(const QString &room_id, const QString &url)
{
        emit updateRoomAvatarCb(room_id, url);
}

void
RoomList::removeRoom(const QString &room_id, bool reset)
{
        auto roomIt = rooms_.find(room_id);
        if (roomIt == rooms_.end()) {
                return;
        }

        for (auto roomSortIt = rooms_sort_cache_.begin(); roomSortIt != rooms_sort_cache_.end();
             ++roomSortIt) {
                if (roomIt->second == *roomSortIt) {
                        rooms_sort_cache_.erase(roomSortIt);
                        break;
                }
        }
        rooms_.erase(room_id);

        if (rooms_.empty() || !reset)
                return;

        auto room = firstRoom();

        if (room.second.isNull())
                return;

        room.second->setPressedState(true);
        emit roomChanged(room.first);
}

void
RoomList::updateUnreadMessageCount(const QString &roomid, int count, int highlightedCount)
{
        if (!roomExists(roomid)) {
                nhlog::ui()->warn("updateUnreadMessageCount: unknown room_id {}",
                                  roomid.toStdString());
                return;
        }

        rooms_[roomid]->updateUnreadMessageCount(count, highlightedCount);

        calculateUnreadMessageCount();

        sortRoomsByLastMessage();
}

void
RoomList::calculateUnreadMessageCount()
{
        int total_unread_msgs = 0;

        for (const auto &room : rooms_) {
                if (!room.second.isNull())
                        total_unread_msgs += room.second->unreadMessageCount();
        }

        emit totalUnreadMessageCountUpdated(total_unread_msgs);
}

void
RoomList::initialize(const QMap<QString, RoomInfo> &info)
{
        nhlog::ui()->info("initialize room list");

        rooms_.clear();

        // prevent flickering and save time sorting over and over again
        setUpdatesEnabled(false);
        for (auto it = info.begin(); it != info.end(); it++) {
                if (it.value().is_invite)
                        addInvitedRoom(it.key(), it.value());
                else
                        addRoom(it.key(), it.value());
        }

        for (auto it = info.begin(); it != info.end(); it++)
                updateRoomDescription(it.key(), it.value().msgInfo);

        setUpdatesEnabled(true);

        if (rooms_.empty())
                return;

        sortRoomsByLastMessage();

        auto room = firstRoom();
        if (room.second.isNull())
                return;

        room.second->setPressedState(true);
        emit roomChanged(room.first);
}

void
RoomList::cleanupInvites(const std::map<QString, bool> &invites)
{
        if (invites.size() == 0)
                return;

        utils::erase_if(rooms_, [invites](auto &room) {
                auto room_id = room.first;
                auto item    = room.second;

                if (!item)
                        return false;

                return item->isInvite() && (invites.find(room_id) == invites.end());
        });
}

void
RoomList::sync(const std::map<QString, RoomInfo> &info)

{
        for (const auto &room : info)
                updateRoom(room.first, room.second);

        if (!info.empty())
                sortRoomsByLastMessage();
}

void
RoomList::highlightSelectedRoom(const QString &room_id)
{
        emit roomChanged(room_id);

        if (!roomExists(room_id)) {
                nhlog::ui()->warn("roomlist: clicked unknown room_id");
                return;
        }

        for (auto const &room : rooms_) {
                if (room.second.isNull())
                        continue;

                if (room.first != room_id) {
                        room.second->setPressedState(false);
                } else {
                        room.second->setPressedState(true);
                        scrollArea_->ensureWidgetVisible(room.second.data());
                }
        }

        selectedRoom_ = room_id;
}

void
RoomList::nextRoom()
{
        for (int ii = 0; ii < contentsLayout_->count() - 1; ++ii) {
                auto room = qobject_cast<RoomInfoListItem *>(contentsLayout_->itemAt(ii)->widget());

                if (!room)
                        continue;

                if (room->roomId() == selectedRoom_) {
                        auto nextRoom = qobject_cast<RoomInfoListItem *>(
                          contentsLayout_->itemAt(ii + 1)->widget());

                        // Not a room message.
                        if (!nextRoom || nextRoom->isInvite())
                                return;

                        emit roomChanged(nextRoom->roomId());
                        if (!roomExists(nextRoom->roomId())) {
                                nhlog::ui()->warn("roomlist: clicked unknown room_id");
                                return;
                        }

                        room->setPressedState(false);
                        nextRoom->setPressedState(true);

                        scrollArea_->ensureWidgetVisible(nextRoom);
                        selectedRoom_ = nextRoom->roomId();
                        return;
                }
        }
}

void
RoomList::previousRoom()
{
        for (int ii = 1; ii < contentsLayout_->count(); ++ii) {
                auto room = qobject_cast<RoomInfoListItem *>(contentsLayout_->itemAt(ii)->widget());

                if (!room)
                        continue;

                if (room->roomId() == selectedRoom_) {
                        auto nextRoom = qobject_cast<RoomInfoListItem *>(
                          contentsLayout_->itemAt(ii - 1)->widget());

                        // Not a room message.
                        if (!nextRoom || nextRoom->isInvite())
                                return;

                        emit roomChanged(nextRoom->roomId());
                        if (!roomExists(nextRoom->roomId())) {
                                nhlog::ui()->warn("roomlist: clicked unknown room_id");
                                return;
                        }

                        room->setPressedState(false);
                        nextRoom->setPressedState(true);

                        scrollArea_->ensureWidgetVisible(nextRoom);
                        selectedRoom_ = nextRoom->roomId();
                        return;
                }
        }
}

void
RoomList::updateRoomAvatar(const QString &roomid, const QString &img)
{
        if (!roomExists(roomid)) {
                nhlog::ui()->warn("avatar update on non-existent room_id: {}",
                                  roomid.toStdString());
                return;
        }

        rooms_[roomid]->setAvatar(img);

        // Used to inform other widgets for the new image data.
        emit roomAvatarChanged(roomid, img);
}

void
RoomList::updateRoomDescription(const QString &roomid, const DescInfo &info)
{
        if (!roomExists(roomid)) {
                nhlog::ui()->warn("description update on non-existent room_id: {}, {}",
                                  roomid.toStdString(),
                                  info.body.toStdString());
                return;
        }

        rooms_[roomid]->setDescriptionMessage(info);

        if (underMouse()) {
                // When the user hover out of the roomlist a sort will be triggered.
                isSortPending_ = true;
                return;
        }

        isSortPending_ = false;

        emit sortRoomsByLastMessage();
}

struct room_sort
{
        bool operator()(const QSharedPointer<RoomInfoListItem> &a,
                        const QSharedPointer<RoomInfoListItem> &b) const
        {
                // Sort by "importance" (i.e. invites before mentions before
                // notifs before new events before old events), then secondly
                // by recency.

                // Checking importance first
                const auto a_importance = a->calculateImportance();
                const auto b_importance = b->calculateImportance();
                if (a_importance != b_importance) {
                        return a_importance > b_importance;
                }

                // Now sort by recency
                // Zero if empty, otherwise the time that the event occured
                const uint64_t a_recency =
                  a->lastMsgInfo_.userid.isEmpty() ? 0 : a->lastMsgInfo_.timestamp;
                const uint64_t b_recency =
                  b->lastMsgInfo_.userid.isEmpty() ? 0 : b->lastMsgInfo_.timestamp;
                return a_recency > b_recency;
        }
};

void
RoomList::sortRoomsByLastMessage()
{
        isSortPending_ = false;

        std::stable_sort(begin(rooms_sort_cache_), end(rooms_sort_cache_), room_sort{});

        int newIndex = 0;
        for (const auto &roomWidget : rooms_sort_cache_) {
                const auto currentIndex = contentsLayout_->indexOf(roomWidget.data());

                if (currentIndex != newIndex) {
                        contentsLayout_->removeWidget(roomWidget.data());
                        contentsLayout_->insertWidget(newIndex, roomWidget.data());
                }
                newIndex++;
        }
}

void
RoomList::leaveEvent(QEvent *event)
{
        if (isSortPending_)
                QTimer::singleShot(700, this, &RoomList::sortRoomsByLastMessage);

        QWidget::leaveEvent(event);
}

void
RoomList::closeJoinRoomDialog(bool isJoining, QString roomAlias)
{
        joinRoomModal_->hide();

        if (isJoining)
                emit joinRoom(roomAlias);
}

void
RoomList::removeFilter(const std::set<QString> &roomsToHide)
{
        setUpdatesEnabled(false);
        for (int i = 0; i < contentsLayout_->count(); i++) {
                auto widget =
                  qobject_cast<RoomInfoListItem *>(contentsLayout_->itemAt(i)->widget());
                if (widget) {
                        if (roomsToHide.find(widget->roomId()) == roomsToHide.end())
                                widget->show();
                        else
                                widget->hide();
                }
        }
        setUpdatesEnabled(true);
}

void
RoomList::applyFilter(const std::set<QString> &filter)
{
        // Disabling paint updates will resolve issues with screen flickering on big room lists.
        setUpdatesEnabled(false);

        for (int i = 0; i < contentsLayout_->count(); i++) {
                // If filter contains the room for the current RoomInfoListItem,
                // show the list item, otherwise hide it
                auto listitem =
                  qobject_cast<RoomInfoListItem *>(contentsLayout_->itemAt(i)->widget());

                if (!listitem)
                        continue;

                if (filter.find(listitem->roomId()) != filter.end())
                        listitem->show();
                else
                        listitem->hide();
        }

        setUpdatesEnabled(true);

        // If the already selected room is part of the group, make sure it's visible.
        if (!selectedRoom_.isEmpty() && (filter.find(selectedRoom_) != filter.end()))
                return;

        selectFirstVisibleRoom();
}

void
RoomList::selectFirstVisibleRoom()
{
        for (int i = 0; i < contentsLayout_->count(); i++) {
                auto item = qobject_cast<RoomInfoListItem *>(contentsLayout_->itemAt(i)->widget());

                if (item && item->isVisible()) {
                        highlightSelectedRoom(item->roomId());
                        break;
                }
        }
}

void
RoomList::paintEvent(QPaintEvent *)
{
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void
RoomList::updateRoom(const QString &room_id, const RoomInfo &info)
{
        if (!roomExists(room_id)) {
                if (info.is_invite)
                        addInvitedRoom(room_id, info);
                else
                        addRoom(room_id, info);

                return;
        }

        auto room = rooms_[room_id];
        updateAvatar(room_id, QString::fromStdString(info.avatar_url));
        room->setRoomName(QString::fromStdString(info.name));
        room->setRoomType(info.is_invite);
        room->update();
}

void
RoomList::addInvitedRoom(const QString &room_id, const RoomInfo &info)
{
        auto room_item = new RoomInfoListItem(room_id, info, scrollArea_);

        connect(room_item, &RoomInfoListItem::acceptInvite, this, &RoomList::acceptInvite);
        connect(room_item, &RoomInfoListItem::declineInvite, this, &RoomList::declineInvite);

        QSharedPointer<RoomInfoListItem> roomWidget(room_item);
        rooms_.emplace(room_id, roomWidget);
        rooms_sort_cache_.push_back(roomWidget);

        updateAvatar(room_id, QString::fromStdString(info.avatar_url));

        int pos = contentsLayout_->count() - 1;
        contentsLayout_->insertWidget(pos, room_item);
}

std::pair<QString, QSharedPointer<RoomInfoListItem>>
RoomList::firstRoom() const
{
        for (int i = 0; i < contentsLayout_->count(); i++) {
                auto item = qobject_cast<RoomInfoListItem *>(contentsLayout_->itemAt(i)->widget());

                if (item) {
                        auto topRoom = rooms_.find(item->roomId());
                        if (topRoom != rooms_.end()) {
                                return std::pair<QString, QSharedPointer<RoomInfoListItem>>(
                                  item->roomId(), topRoom->second);
                        }
                }
        }

        return {};
}

void
RoomList::updateReadStatus(const std::map<QString, bool> &status)
{
        for (const auto &room : status) {
                if (roomExists(room.first)) {
                        auto item = rooms_.at(room.first);

                        if (item)
                                item->setReadState(room.second);
                }
        }
}
