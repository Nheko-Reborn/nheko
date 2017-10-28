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
#include <QObject>

#include "JoinRoomDialog.h"
#include "LeaveRoomDialog.h"
#include "MainWindow.h"
#include "MatrixClient.h"
#include "OverlayModal.h"
#include "RoomInfoListItem.h"
#include "RoomList.h"
#include "RoomSettings.h"
#include "RoomState.h"
#include "Sync.h"

RoomList::RoomList(QSharedPointer<MatrixClient> client, QWidget *parent)
  : QWidget(parent)
  , client_(client)
{
        setStyleSheet("QWidget { border: none; }");

        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        setSizePolicy(sizePolicy);

        topLayout_ = new QVBoxLayout(this);
        topLayout_->setSpacing(0);
        topLayout_->setMargin(0);

        scrollArea_ = new QScrollArea(this);
        scrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scrollArea_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scrollArea_->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
        scrollArea_->setWidgetResizable(true);
        scrollArea_->setAlignment(Qt::AlignLeading | Qt::AlignLeft | Qt::AlignVCenter);

        scrollAreaContents_ = new QWidget();

        contentsLayout_ = new QVBoxLayout(scrollAreaContents_);
        contentsLayout_->setSpacing(0);
        contentsLayout_->setMargin(0);
        contentsLayout_->addStretch(1);

        scrollArea_->setWidget(scrollAreaContents_);
        topLayout_->addWidget(scrollArea_);

        connect(client_.data(),
                SIGNAL(roomAvatarRetrieved(const QString &, const QPixmap &)),
                this,
                SLOT(updateRoomAvatar(const QString &, const QPixmap &)));
}

RoomList::~RoomList() {}

void
RoomList::clear()
{
        rooms_.clear();
}

void
RoomList::addRoom(const QSharedPointer<RoomSettings> &settings,
                  const RoomState &state,
                  const QString &room_id)
{
        RoomInfoListItem *room_item = new RoomInfoListItem(settings, state, room_id, scrollArea_);
        connect(room_item, &RoomInfoListItem::clicked, this, &RoomList::highlightSelectedRoom);
        connect(room_item, &RoomInfoListItem::leaveRoom, this, &RoomList::openLeaveRoomDialog);

        rooms_.insert(room_id, QSharedPointer<RoomInfoListItem>(room_item));

        client_->fetchRoomAvatar(room_id, state.getAvatar());

        contentsLayout_->insertWidget(0, room_item);
}

void
RoomList::removeRoom(const QString &room_id, bool reset)
{
        rooms_.remove(room_id);

        if (rooms_.isEmpty() || !reset)
                return;

        auto first_room = rooms_.first();
        first_room->setPressedState(true);

        emit roomChanged(rooms_.firstKey());
}

void
RoomList::updateUnreadMessageCount(const QString &roomid, int count)
{
        if (!rooms_.contains(roomid)) {
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

        for (const auto &room : rooms_)
                total_unread_msgs += room->unreadMessageCount();

        emit totalUnreadMessageCountUpdated(total_unread_msgs);
}

void
RoomList::setInitialRooms(const QMap<QString, QSharedPointer<RoomSettings>> &settings,
                          const QMap<QString, RoomState> &states)
{
        rooms_.clear();

        if (settings.size() != states.size()) {
                qWarning() << "Initializing room list";
                qWarning() << "Different number of room states and room settings";
                return;
        }

        for (auto it = states.constBegin(); it != states.constEnd(); ++it) {
                auto room_id = it.key();
                auto state   = it.value();

                if (!state.getAvatar().toString().isEmpty())
                        client_->fetchRoomAvatar(room_id, state.getAvatar());

                RoomInfoListItem *room_item =
                  new RoomInfoListItem(settings[room_id], state, room_id, scrollArea_);
                connect(
                  room_item, &RoomInfoListItem::clicked, this, &RoomList::highlightSelectedRoom);
                connect(
                  room_item, &RoomInfoListItem::leaveRoom, this, &RoomList::openLeaveRoomDialog);

                rooms_.insert(room_id, QSharedPointer<RoomInfoListItem>(room_item));

                int pos = contentsLayout_->count() - 1;
                contentsLayout_->insertWidget(pos, room_item);
        }

        if (rooms_.isEmpty())
                return;

        auto first_room = rooms_.first();
        first_room->setPressedState(true);

        emit roomChanged(rooms_.firstKey());
}

void
RoomList::openLeaveRoomDialog(const QString &room_id)
{
        if (leaveRoomDialog_.isNull()) {
                leaveRoomDialog_ = QSharedPointer<LeaveRoomDialog>(new LeaveRoomDialog(this));

                connect(leaveRoomDialog_.data(),
                        &LeaveRoomDialog::closing,
                        this,
                        [=](bool leaving) { closeLeaveRoomDialog(leaving, room_id); });
        }

        if (leaveRoomModal_.isNull()) {
                leaveRoomModal_ = QSharedPointer<OverlayModal>(
                  new OverlayModal(MainWindow::instance(), leaveRoomDialog_.data()));
                leaveRoomModal_->setDuration(0);
                leaveRoomModal_->setColor(QColor(30, 30, 30, 170));
        }

        leaveRoomModal_->fadeIn();
}

void
RoomList::sync(const QMap<QString, RoomState> &states)
{
        for (auto it = states.constBegin(); it != states.constEnd(); ++it) {
                auto room_id = it.key();
                auto state   = it.value();

                if (!rooms_.contains(room_id)) {
                        addRoom(
                          QSharedPointer<RoomSettings>(new RoomSettings(room_id)), state, room_id);
                }

                auto room = rooms_[room_id];

                auto current_avatar = room->state().getAvatar();
                auto new_avatar     = state.getAvatar();

                if (current_avatar != new_avatar && !new_avatar.toString().isEmpty())
                        client_->fetchRoomAvatar(room_id, new_avatar);

                room->setState(state);
        }
}

void
RoomList::highlightSelectedRoom(const QString &room_id)
{
        emit roomChanged(room_id);

        if (!rooms_.contains(room_id)) {
                qDebug() << "RoomList: clicked unknown roomid";
                return;
        }

        // TODO: Send a read receipt for the last event.
        auto room = rooms_[room_id];
        room->clearUnreadMessageCount();

        calculateUnreadMessageCount();

        for (auto it = rooms_.constBegin(); it != rooms_.constEnd(); ++it) {
                if (it.key() != room_id) {
                        it.value()->setPressedState(false);
                } else {
                        it.value()->setPressedState(true);
                        scrollArea_->ensureWidgetVisible(
                          qobject_cast<QWidget *>(it.value().data()));
                }
        }
}

void
RoomList::updateRoomAvatar(const QString &roomid, const QPixmap &img)
{
        if (!rooms_.contains(roomid)) {
                qWarning() << "Avatar update on non existent room" << roomid;
                return;
        }

        rooms_.value(roomid)->setAvatar(img.toImage());
}

void
RoomList::updateRoomDescription(const QString &roomid, const DescInfo &info)
{
        if (!rooms_.contains(roomid)) {
                qWarning() << "Description update on non existent room" << roomid << info.body;
                return;
        }

        rooms_.value(roomid)->setDescriptionMessage(info);
}

void
RoomList::closeJoinRoomDialog(bool isJoining, QString roomAlias)
{
        joinRoomModal_->fadeOut();

        if (isJoining) {
                client_->joinRoom(roomAlias);
        }
}

void
RoomList::closeLeaveRoomDialog(bool leaving, const QString &room_id)
{
        leaveRoomModal_->fadeOut();

        if (leaving) {
                client_->leaveRoom(room_id);
        }
}
