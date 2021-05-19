// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "RoomlistModel.h"

#include "ChatPage.h"
#include "MatrixClient.h"
#include "MxcImageProvider.h"
#include "TimelineModel.h"
#include "TimelineViewManager.h"
#include "UserSettingsPage.h"

RoomlistModel::RoomlistModel(TimelineViewManager *parent)
  : manager(parent)
{
        connect(ChatPage::instance(), &ChatPage::decryptSidebarChanged, this, [this]() {
                auto decrypt = ChatPage::instance()->userSettings()->decryptSidebar();
                QHash<QString, QSharedPointer<TimelineModel>>::iterator i;
                for (i = models.begin(); i != models.end(); ++i) {
                        auto ptr = i.value();

                        if (!ptr.isNull()) {
                                ptr->setDecryptDescription(decrypt);
                                ptr->updateLastMessage();
                        }
                }
        });
}

QHash<int, QByteArray>
RoomlistModel::roleNames() const
{
        return {
          {AvatarUrl, "avatarUrl"},
          {RoomName, "roomName"},
          {LastMessage, "lastMessage"},
          {HasUnreadMessages, "hasUnreadMessages"},
          {NotificationCount, "notificationCount"},
        };
}

QVariant
RoomlistModel::data(const QModelIndex &index, int role) const
{
        if (index.row() >= 0 && static_cast<size_t>(index.row()) < roomids.size()) {
                auto room = models.value(roomids.at(index.row()));
                switch (role) {
                case Roles::AvatarUrl:
                        return room->roomAvatarUrl();
                case Roles::RoomName:
                        return room->roomName();
                case Roles::LastMessage:
                        return QString("Nico: Hahaha, this is funny!");
                case Roles::HasUnreadMessages:
                        return true;
                case Roles::NotificationCount:
                        return 5;
                default:
                        return {};
                }
        } else {
                return {};
        }
}

void
RoomlistModel::addRoom(const QString &room_id, bool suppressInsertNotification)
{
        if (!models.contains(room_id)) {
                QSharedPointer<TimelineModel> newRoom(new TimelineModel(manager, room_id));
                newRoom->setDecryptDescription(
                  ChatPage::instance()->userSettings()->decryptSidebar());

                connect(newRoom.data(),
                        &TimelineModel::newEncryptedImage,
                        manager->imageProvider(),
                        &MxcImageProvider::addEncryptionInfo);
                connect(newRoom.data(),
                        &TimelineModel::forwardToRoom,
                        manager,
                        &TimelineViewManager::forwardMessageToRoom);

                if (!suppressInsertNotification)
                        beginInsertRows(QModelIndex(), (int)roomids.size(), (int)roomids.size());
                models.insert(room_id, std::move(newRoom));
                roomids.push_back(room_id);
                if (!suppressInsertNotification)
                        endInsertRows();
        }
}

void
RoomlistModel::sync(const mtx::responses::Rooms &rooms)
{
        for (const auto &[room_id, room] : rooms.join) {
                // addRoom will only add the room, if it doesn't exist
                addRoom(QString::fromStdString(room_id));
                const auto &room_model = models.value(QString::fromStdString(room_id));
                room_model->syncState(room.state);
                room_model->addEvents(room.timeline);
                connect(room_model.data(),
                        &TimelineModel::newCallEvent,
                        manager->callManager(),
                        &CallManager::syncEvent,
                        Qt::UniqueConnection);

                if (ChatPage::instance()->userSettings()->typingNotifications()) {
                        for (const auto &ev : room.ephemeral.events) {
                                if (auto t = std::get_if<
                                      mtx::events::EphemeralEvent<mtx::events::ephemeral::Typing>>(
                                      &ev)) {
                                        std::vector<QString> typing;
                                        typing.reserve(t->content.user_ids.size());
                                        for (const auto &user : t->content.user_ids) {
                                                if (user != http::client()->user_id().to_string())
                                                        typing.push_back(
                                                          QString::fromStdString(user));
                                        }
                                        room_model->updateTypingUsers(typing);
                                }
                        }
                }
        }
}

void
RoomlistModel::initializeRooms(const std::vector<QString> &roomIds_)
{
        beginResetModel();
        models.clear();
        roomids.clear();
        roomids = roomIds_;
        for (const auto &id : roomIds_)
                addRoom(id, true);
        endResetModel();
}

void
RoomlistModel::clear()
{
        beginResetModel();
        models.clear();
        roomids.clear();
        endResetModel();
}
