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

#include <QBuffer>
#include <QDebug>
#include <QObject>
#include <QTimer>

#include "Cache.h"
#include "MainWindow.h"
#include "MatrixClient.h"
#include "OverlayModal.h"
#include "RoomInfoListItem.h"
#include "RoomList.h"
#include "RoomSettings.h"
#include "RoomState.h"
#include "UserSettingsPage.h"

RoomList::RoomList(QSharedPointer<MatrixClient> client,
                   QSharedPointer<UserSettings> userSettings,
                   QWidget *parent)
  : QWidget(parent)
  , client_(client)
  , userSettings_{userSettings}
{
        setStyleSheet("border: none;");
        topLayout_ = new QVBoxLayout(this);
        topLayout_->setSpacing(0);
        topLayout_->setMargin(0);

        scrollArea_ = new QScrollArea(this);
        scrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scrollArea_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scrollArea_->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
        scrollArea_->setWidgetResizable(true);
        scrollArea_->setAlignment(Qt::AlignLeading | Qt::AlignTop | Qt::AlignVCenter);

        scrollAreaContents_ = new QWidget(this);

        contentsLayout_ = new QVBoxLayout(scrollAreaContents_);
        contentsLayout_->setSpacing(0);
        contentsLayout_->setMargin(0);
        contentsLayout_->addStretch(1);

        scrollArea_->setWidget(scrollAreaContents_);
        topLayout_->addWidget(scrollArea_);

        connect(client_.data(),
                &MatrixClient::roomAvatarRetrieved,
                this,
                [this](const QString &room_id,
                       const QPixmap &img,
                       const QString &url,
                       const QByteArray &data) {
                        if (!cache_.isNull())
                                cache_->saveImage(url, data);

                        updateRoomAvatar(room_id, img);
                });
}

void
RoomList::clear()
{
        rooms_.clear();
}

void
RoomList::addRoom(const QSharedPointer<RoomSettings> &settings,
                  const QSharedPointer<RoomState> &state,
                  const QString &room_id)
{
        auto room_item = new RoomInfoListItem(settings, state, room_id, scrollArea_);
        connect(room_item, &RoomInfoListItem::clicked, this, &RoomList::highlightSelectedRoom);
        connect(room_item, &RoomInfoListItem::leaveRoom, this, [](const QString &room_id) {
                MainWindow::instance()->openLeaveRoomDialog(room_id);
        });

        rooms_.emplace(room_id, QSharedPointer<RoomInfoListItem>(room_item));

        if (!state->getAvatar().toString().isEmpty())
                updateAvatar(room_id, state->getAvatar().toString());

        int pos = contentsLayout_->count() - 1;
        contentsLayout_->insertWidget(pos, room_item);
}

void
RoomList::updateAvatar(const QString &room_id, const QString &url)
{
        if (url.isEmpty())
                return;

        QByteArray savedImgData;

        if (!cache_.isNull())
                savedImgData = cache_->image(url);

        if (savedImgData.isEmpty()) {
                client_->fetchRoomAvatar(room_id, url);
        } else {
                QPixmap img;
                img.loadFromData(savedImgData);

                updateRoomAvatar(room_id, img);
        }
}

void
RoomList::removeRoom(const QString &room_id, bool reset)
{
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
RoomList::updateUnreadMessageCount(const QString &roomid, int count)
{
        if (!roomExists(roomid)) {
                qWarning() << "UpdateUnreadMessageCount: Unknown roomid";
                return;
        }

        rooms_[roomid]->updateUnreadMessageCount(count);

        calculateUnreadMessageCount();
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
RoomList::setInitialRooms(const std::map<QString, QSharedPointer<RoomSettings>> &settings,
                          const std::map<QString, QSharedPointer<RoomState>> &states)
{
        rooms_.clear();

        if (settings.size() != states.size()) {
                qWarning() << "Initializing room list";
                qWarning() << "Different number of room states and room settings";
                return;
        }

        for (auto const &state : states)
                addRoom(settings.at(state.first), state.second, state.first);

        if (rooms_.empty())
                return;

        auto room = firstRoom();
        if (room.second.isNull())
                return;

        room.second->setPressedState(true);
        emit roomChanged(room.first);
}

void
RoomList::sync(const std::map<QString, QSharedPointer<RoomState>> &states,
               const std::map<QString, QSharedPointer<RoomSettings>> &settings)

{
        for (auto const &state : states) {
                if (!roomExists(state.first))
                        addRoom(settings.at(state.first), state.second, state.first);

                auto room       = rooms_[state.first];
                auto new_avatar = state.second->getAvatar();

                updateAvatar(state.first, new_avatar.toString());

                room->setState(state.second);
        }
}

void
RoomList::highlightSelectedRoom(const QString &room_id)
{
        emit roomChanged(room_id);

        if (!roomExists(room_id)) {
                qDebug() << "RoomList: clicked unknown roomid";
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
RoomList::updateRoomAvatar(const QString &roomid, const QPixmap &img)
{
        if (!roomExists(roomid)) {
                qWarning() << "Avatar update on non existent room" << roomid;
                return;
        }

        rooms_[roomid]->setAvatar(img.toImage());

        // Used to inform other widgets for the new image data.
        emit roomAvatarChanged(roomid, img);
}

void
RoomList::updateRoomDescription(const QString &roomid, const DescInfo &info)
{
        if (!roomExists(roomid)) {
                qWarning() << "Description update on non existent room" << roomid << info.body;
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

void
RoomList::sortRoomsByLastMessage()
{
        if (!userSettings_->isOrderingEnabled())
                return;

        isSortPending_ = false;

        std::multimap<uint64_t, RoomInfoListItem *, std::greater<uint64_t>> times;

        for (int ii = 0; ii < contentsLayout_->count(); ++ii) {
                auto room = qobject_cast<RoomInfoListItem *>(contentsLayout_->itemAt(ii)->widget());

                if (!room)
                        continue;

                // Not a room message.
                if (room->lastMessageInfo().userid.isEmpty())
                        times.emplace(0, room);
                else
                        times.emplace(room->lastMessageInfo().datetime.toMSecsSinceEpoch(), room);
        }

        for (auto it = times.cbegin(); it != times.cend(); ++it) {
                const auto roomWidget   = it->second;
                const auto currentIndex = contentsLayout_->indexOf(roomWidget);
                const auto newIndex     = std::distance(times.cbegin(), it);

                if (currentIndex == newIndex)
                        continue;

                contentsLayout_->removeWidget(roomWidget);
                contentsLayout_->insertWidget(newIndex, roomWidget);
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
                client_->joinRoom(roomAlias);
}

void
RoomList::setFilterRooms(bool isFilteringEnabled)
{
        for (int i = 0; i < contentsLayout_->count(); i++) {
                // If roomFilter_ contains the room for the current RoomInfoListItem,
                // show the list item, otherwise hide it
                auto listitem =
                  qobject_cast<RoomInfoListItem *>(contentsLayout_->itemAt(i)->widget());

                if (!listitem)
                        continue;

                if (!isFilteringEnabled || filterItemExists(listitem->roomId()))
                        listitem->show();
                else
                        listitem->hide();
        }

        if (isFilteringEnabled && !filterItemExists(selectedRoom_)) {
                RoomInfoListItem *firstVisibleRoom = nullptr;

                for (int i = 0; i < contentsLayout_->count(); i++) {
                        QWidget *item = contentsLayout_->itemAt(i)->widget();

                        if (item != nullptr && item->isVisible()) {
                                firstVisibleRoom = qobject_cast<RoomInfoListItem *>(item);
                                break;
                        }
                }

                if (firstVisibleRoom != nullptr)
                        highlightSelectedRoom(firstVisibleRoom->roomId());
        } else {
                scrollArea_->ensureWidgetVisible(rooms_[selectedRoom_].data());
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
RoomList::syncInvites(const std::map<std::string, mtx::responses::InvitedRoom> &rooms)
{
        for (auto it = rooms.cbegin(); it != rooms.cend(); ++it) {
                const auto room_id = QString::fromStdString(it->first);

                if (!roomExists(room_id))
                        addInvitedRoom(room_id, it->second);
        }
}

void
RoomList::setRoomFilter(std::vector<QString> room_ids)
{
        roomFilter_ = room_ids;
        setFilterRooms(true);
}

void
RoomList::addInvitedRoom(const QString &room_id, const mtx::responses::InvitedRoom &room)
{
        auto room_item = new RoomInfoListItem(room_id, room, scrollArea_);
        connect(room_item, &RoomInfoListItem::acceptInvite, this, &RoomList::acceptInvite);
        connect(room_item, &RoomInfoListItem::declineInvite, this, &RoomList::declineInvite);

        rooms_.emplace(room_id, QSharedPointer<RoomInfoListItem>(room_item));

        updateAvatar(room_id, QString::fromStdString(room.avatar()));

        int pos = contentsLayout_->count() - 1;
        contentsLayout_->insertWidget(pos, room_item);
}

std::pair<QString, QSharedPointer<RoomInfoListItem>>
RoomList::firstRoom() const
{
        auto firstRoom = rooms_.begin();

        while (firstRoom->second.isNull() && firstRoom != rooms_.end())
                firstRoom++;

        return std::pair<QString, QSharedPointer<RoomInfoListItem>>(firstRoom->first,
                                                                    firstRoom->second);
}
