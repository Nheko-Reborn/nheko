// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "RoomlistModel.h"

#include "Cache_p.h"
#include "ChatPage.h"
#include "MatrixClient.h"
#include "MxcImageProvider.h"
#include "TimelineModel.h"
#include "TimelineViewManager.h"
#include "UserSettingsPage.h"

RoomlistModel::RoomlistModel(TimelineViewManager *parent)
  : QAbstractListModel(parent)
  , manager(parent)
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

        connect(this,
                &RoomlistModel::totalUnreadMessageCountUpdated,
                ChatPage::instance(),
                &ChatPage::unreadMessages);
}

QHash<int, QByteArray>
RoomlistModel::roleNames() const
{
        return {
          {AvatarUrl, "avatarUrl"},
          {RoomName, "roomName"},
          {RoomId, "roomId"},
          {LastMessage, "lastMessage"},
          {Time, "time"},
          {Timestamp, "timestamp"},
          {HasUnreadMessages, "hasUnreadMessages"},
          {HasLoudNotification, "hasLoudNotification"},
          {NotificationCount, "notificationCount"},
          {IsInvite, "isInvite"},
          {IsSpace, "isSpace"},
          {Tags, "tags"},
          {ParentSpaces, "parentSpaces"},
        };
}

QVariant
RoomlistModel::data(const QModelIndex &index, int role) const
{
        if (index.row() >= 0 && static_cast<size_t>(index.row()) < roomids.size()) {
                auto roomid = roomids.at(index.row());

                if (models.contains(roomid)) {
                        auto room = models.value(roomid);
                        switch (role) {
                        case Roles::AvatarUrl:
                                return room->roomAvatarUrl();
                        case Roles::RoomName:
                                return room->plainRoomName();
                        case Roles::RoomId:
                                return room->roomId();
                        case Roles::LastMessage:
                                return room->lastMessage().body;
                        case Roles::Time:
                                return room->lastMessage().descriptiveTime;
                        case Roles::Timestamp:
                                return QVariant(
                                  static_cast<quint64>(room->lastMessage().timestamp));
                        case Roles::HasUnreadMessages:
                                return this->roomReadStatus.count(roomid) &&
                                       this->roomReadStatus.at(roomid);
                        case Roles::HasLoudNotification:
                                return room->hasMentions();
                        case Roles::NotificationCount:
                                return room->notificationCount();
                        case Roles::IsInvite:
                                return false;
                        case Roles::IsSpace:
                                return room->isSpace();
                        case Roles::Tags: {
                                auto info = cache::singleRoomInfo(roomid.toStdString());
                                QStringList list;
                                for (const auto &t : info.tags)
                                        list.push_back(QString::fromStdString(t));
                                return list;
                        }
                        case Roles::ParentSpaces: {
                                auto parents =
                                  cache::client()->getParentRoomIds(roomid.toStdString());
                                QStringList list;
                                for (const auto &t : parents)
                                        list.push_back(QString::fromStdString(t));
                                return list;
                        }
                        default:
                                return {};
                        }
                } else if (invites.contains(roomid)) {
                        auto room = invites.value(roomid);
                        switch (role) {
                        case Roles::AvatarUrl:
                                return QString::fromStdString(room.avatar_url);
                        case Roles::RoomName:
                                return QString::fromStdString(room.name);
                        case Roles::RoomId:
                                return roomid;
                        case Roles::LastMessage:
                                return room.msgInfo.body;
                        case Roles::Time:
                                return room.msgInfo.descriptiveTime;
                        case Roles::Timestamp:
                                return QVariant(static_cast<quint64>(room.msgInfo.timestamp));
                        case Roles::HasUnreadMessages:
                        case Roles::HasLoudNotification:
                                return false;
                        case Roles::NotificationCount:
                                return 0;
                        case Roles::IsInvite:
                                return true;
                        case Roles::IsSpace:
                                return false;
                        case Roles::Tags:
                                return QStringList();
                        case Roles::ParentSpaces: {
                                auto parents =
                                  cache::client()->getParentRoomIds(roomid.toStdString());
                                QStringList list;
                                for (const auto &t : parents)
                                        list.push_back(QString::fromStdString(t));
                                return list;
                        }
                        default:
                                return {};
                        }
                } else {
                        return {};
                }
        } else {
                return {};
        }
}

void
RoomlistModel::updateReadStatus(const std::map<QString, bool> roomReadStatus_)
{
        std::vector<int> roomsToUpdate;
        roomsToUpdate.resize(roomReadStatus_.size());
        for (const auto &[roomid, roomUnread] : roomReadStatus_) {
                if (roomUnread != roomReadStatus[roomid]) {
                        roomsToUpdate.push_back(this->roomidToIndex(roomid));
                }

                this->roomReadStatus[roomid] = roomUnread;
        }

        for (auto idx : roomsToUpdate) {
                emit dataChanged(index(idx),
                                 index(idx),
                                 {
                                   Roles::HasUnreadMessages,
                                 });
        }
}
void
RoomlistModel::addRoom(const QString &room_id, bool suppressInsertNotification)
{
        if (!models.contains(room_id)) {
                // ensure we get read status updates and are only connected once
                connect(cache::client(),
                        &Cache::roomReadStatus,
                        this,
                        &RoomlistModel::updateReadStatus,
                        Qt::UniqueConnection);

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
                connect(
                  newRoom.data(), &TimelineModel::lastMessageChanged, this, [room_id, this]() {
                          auto idx = this->roomidToIndex(room_id);
                          emit dataChanged(index(idx),
                                           index(idx),
                                           {
                                             Roles::HasLoudNotification,
                                             Roles::LastMessage,
                                             Roles::Timestamp,
                                             Roles::NotificationCount,
                                             Qt::DisplayRole,
                                           });
                  });
                connect(
                  newRoom.data(), &TimelineModel::roomAvatarUrlChanged, this, [room_id, this]() {
                          auto idx = this->roomidToIndex(room_id);
                          emit dataChanged(index(idx),
                                           index(idx),
                                           {
                                             Roles::AvatarUrl,
                                           });
                  });
                connect(newRoom.data(), &TimelineModel::roomNameChanged, this, [room_id, this]() {
                        auto idx = this->roomidToIndex(room_id);
                        emit dataChanged(index(idx),
                                         index(idx),
                                         {
                                           Roles::RoomName,
                                         });
                });
                connect(
                  newRoom.data(), &TimelineModel::notificationsChanged, this, [room_id, this]() {
                          auto idx = this->roomidToIndex(room_id);
                          emit dataChanged(index(idx),
                                           index(idx),
                                           {
                                             Roles::HasLoudNotification,
                                             Roles::NotificationCount,
                                             Qt::DisplayRole,
                                           });

                          int total_unread_msgs = 0;

                          for (const auto &room : models) {
                                  if (!room.isNull())
                                          total_unread_msgs += room->notificationCount();
                          }

                          emit totalUnreadMessageCountUpdated(total_unread_msgs);
                  });

                newRoom->updateLastMessage();

                bool wasInvite = invites.contains(room_id);
                if (!suppressInsertNotification && !wasInvite)
                        beginInsertRows(QModelIndex(), (int)roomids.size(), (int)roomids.size());

                models.insert(room_id, std::move(newRoom));

                if (wasInvite) {
                        auto idx = roomidToIndex(room_id);
                        invites.remove(room_id);
                        emit dataChanged(index(idx), index(idx));
                } else {
                        roomids.push_back(room_id);
                }

                if (!suppressInsertNotification && !wasInvite)
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
                room_model->sync(room);
                // room_model->addEvents(room.timeline);
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

        for (const auto &[room_id, room] : rooms.leave) {
                (void)room;
                auto idx = this->roomidToIndex(QString::fromStdString(room_id));
                if (idx != -1) {
                        beginRemoveRows(QModelIndex(), idx, idx);
                        roomids.erase(roomids.begin() + idx);
                        if (models.contains(QString::fromStdString(room_id)))
                                models.remove(QString::fromStdString(room_id));
                        else if (invites.contains(QString::fromStdString(room_id)))
                                invites.remove(QString::fromStdString(room_id));
                        endRemoveRows();
                }
        }

        for (const auto &[room_id, room] : rooms.invite) {
                (void)room;
                auto qroomid = QString::fromStdString(room_id);

                auto invite = cache::client()->invite(room_id);
                if (!invite)
                        continue;

                if (invites.contains(qroomid)) {
                        invites[qroomid] = *invite;
                        auto idx         = roomidToIndex(qroomid);
                        emit dataChanged(index(idx), index(idx));
                } else {
                        beginInsertRows(QModelIndex(), (int)roomids.size(), (int)roomids.size());
                        invites.insert(qroomid, *invite);
                        roomids.push_back(std::move(qroomid));
                        endInsertRows();
                }
        }
}

void
RoomlistModel::initializeRooms()
{
        beginResetModel();
        models.clear();
        roomids.clear();
        invites.clear();
        currentRoom_ = nullptr;

        invites = cache::client()->invites();
        for (const auto &id : invites.keys())
                roomids.push_back(id);

        for (const auto &id : cache::client()->roomIds())
                addRoom(id, true);

        endResetModel();
}

void
RoomlistModel::clear()
{
        beginResetModel();
        models.clear();
        invites.clear();
        roomids.clear();
        currentRoom_ = nullptr;
        emit currentRoomChanged();
        endResetModel();
}

void
RoomlistModel::acceptInvite(QString roomid)
{
        if (invites.contains(roomid)) {
                auto idx = roomidToIndex(roomid);

                if (idx != -1) {
                        beginRemoveRows(QModelIndex(), idx, idx);
                        roomids.erase(roomids.begin() + idx);
                        invites.remove(roomid);
                        endRemoveRows();
                        ChatPage::instance()->joinRoom(roomid);
                }
        }
}
void
RoomlistModel::declineInvite(QString roomid)
{
        if (invites.contains(roomid)) {
                auto idx = roomidToIndex(roomid);

                if (idx != -1) {
                        beginRemoveRows(QModelIndex(), idx, idx);
                        roomids.erase(roomids.begin() + idx);
                        invites.remove(roomid);
                        endRemoveRows();
                        ChatPage::instance()->leaveRoom(roomid);
                }
        }
}
void
RoomlistModel::leave(QString roomid)
{
        if (models.contains(roomid)) {
                auto idx = roomidToIndex(roomid);

                if (idx != -1) {
                        beginRemoveRows(QModelIndex(), idx, idx);
                        roomids.erase(roomids.begin() + idx);
                        models.remove(roomid);
                        endRemoveRows();
                        ChatPage::instance()->leaveRoom(roomid);
                }
        }
}

void
RoomlistModel::setCurrentRoom(QString roomid)
{
        nhlog::ui()->debug("Trying to switch to: {}", roomid.toStdString());
        if (models.contains(roomid)) {
                currentRoom_ = models.value(roomid);
                emit currentRoomChanged();
                nhlog::ui()->debug("Switched to: {}", roomid.toStdString());
        }
}

namespace {
enum NotificationImportance : short
{
        ImportanceDisabled = -1,
        AllEventsRead      = 0,
        NewMessage         = 1,
        NewMentions        = 2,
        Invite             = 3,
        SubSpace           = 4,
        CurrentSpace       = 5,
};
}

short int
FilteredRoomlistModel::calculateImportance(const QModelIndex &idx) const
{
        // Returns the degree of importance of the unread messages in the room.
        // If sorting by importance is disabled in settings, this only ever
        // returns ImportanceDisabled or Invite
        if (sourceModel()->data(idx, RoomlistModel::IsSpace).toBool()) {
                if (filterType == FilterBy::Space &&
                    filterStr == sourceModel()->data(idx, RoomlistModel::RoomId).toString())
                        return CurrentSpace;
                else
                        return SubSpace;
        } else if (sourceModel()->data(idx, RoomlistModel::IsInvite).toBool()) {
                return Invite;
        } else if (!this->sortByImportance) {
                return ImportanceDisabled;
        } else if (sourceModel()->data(idx, RoomlistModel::HasLoudNotification).toBool()) {
                return NewMentions;
        } else if (sourceModel()->data(idx, RoomlistModel::NotificationCount).toInt() > 0) {
                return NewMessage;
        } else {
                return AllEventsRead;
        }
}
bool
FilteredRoomlistModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
        QModelIndex const left_idx  = sourceModel()->index(left.row(), 0, QModelIndex());
        QModelIndex const right_idx = sourceModel()->index(right.row(), 0, QModelIndex());

        // Sort by "importance" (i.e. invites before mentions before
        // notifs before new events before old events), then secondly
        // by recency.

        // Checking importance first
        const auto a_importance = calculateImportance(left_idx);
        const auto b_importance = calculateImportance(right_idx);
        if (a_importance != b_importance) {
                return a_importance > b_importance;
        }

        // Now sort by recency
        // Zero if empty, otherwise the time that the event occured
        uint64_t a_recency = sourceModel()->data(left_idx, RoomlistModel::Timestamp).toULongLong();
        uint64_t b_recency = sourceModel()->data(right_idx, RoomlistModel::Timestamp).toULongLong();

        if (a_recency != b_recency)
                return a_recency > b_recency;
        else
                return left.row() < right.row();
}

FilteredRoomlistModel::FilteredRoomlistModel(RoomlistModel *model, QObject *parent)
  : QSortFilterProxyModel(parent)
  , roomlistmodel(model)
{
        this->sortByImportance = UserSettings::instance()->sortByImportance();
        setSourceModel(model);
        setDynamicSortFilter(true);

        QObject::connect(UserSettings::instance().get(),
                         &UserSettings::roomSortingChanged,
                         this,
                         [this](bool sortByImportance_) {
                                 this->sortByImportance = sortByImportance_;
                                 invalidate();
                         });

        connect(roomlistmodel,
                &RoomlistModel::currentRoomChanged,
                this,
                &FilteredRoomlistModel::currentRoomChanged);

        sort(0);
}

void
FilteredRoomlistModel::updateHiddenTagsAndSpaces()
{
        hiddenTags.clear();
        hiddenSpaces.clear();
        for (const auto &t : UserSettings::instance()->hiddenTags()) {
                if (t.startsWith("tag:"))
                        hiddenTags.push_back(t.mid(4));
                else if (t.startsWith("space:"))
                        hiddenSpaces.push_back(t.mid(6));
        }

        invalidateFilter();
}

bool
FilteredRoomlistModel::filterAcceptsRow(int sourceRow, const QModelIndex &) const
{
        if (filterType == FilterBy::Nothing) {
                if (!hiddenTags.empty()) {
                        auto tags =
                          sourceModel()
                            ->data(sourceModel()->index(sourceRow, 0), RoomlistModel::Tags)
                            .toStringList();

                        for (const auto &t : tags)
                                if (hiddenTags.contains(t))
                                        return false;
                } else if (!hiddenSpaces.empty()) {
                        auto parents =
                          sourceModel()
                            ->data(sourceModel()->index(sourceRow, 0), RoomlistModel::ParentSpaces)
                            .toStringList();
                        for (const auto &t : parents)
                                if (hiddenSpaces.contains(t))
                                        return false;
                } else if (sourceModel()
                             ->data(sourceModel()->index(sourceRow, 0), RoomlistModel::IsSpace)
                             .toBool()) {
                        return false;
                }

                return true;
        } else if (filterType == FilterBy::Tag) {
                auto tags = sourceModel()
                              ->data(sourceModel()->index(sourceRow, 0), RoomlistModel::Tags)
                              .toStringList();

                if (!tags.contains(filterStr))
                        return false;
                else if (!hiddenTags.empty()) {
                        for (const auto &t : tags)
                                if (t != filterStr && hiddenTags.contains(t))
                                        return false;
                } else if (!hiddenSpaces.empty()) {
                        auto parents =
                          sourceModel()
                            ->data(sourceModel()->index(sourceRow, 0), RoomlistModel::ParentSpaces)
                            .toStringList();
                        for (const auto &t : parents)
                                if (hiddenSpaces.contains(t))
                                        return false;
                } else if (sourceModel()
                             ->data(sourceModel()->index(sourceRow, 0), RoomlistModel::IsSpace)
                             .toBool()) {
                        return false;
                }
                return true;
        } else if (filterType == FilterBy::Space) {
                auto parents =
                  sourceModel()
                    ->data(sourceModel()->index(sourceRow, 0), RoomlistModel::ParentSpaces)
                    .toStringList();
                auto tags = sourceModel()
                              ->data(sourceModel()->index(sourceRow, 0), RoomlistModel::Tags)
                              .toStringList();

                if (filterStr == sourceModel()
                                   ->data(sourceModel()->index(sourceRow, 0), RoomlistModel::RoomId)
                                   .toString())
                        return true;
                else if (!parents.contains(filterStr))
                        return false;
                else if (!hiddenTags.empty()) {
                        for (const auto &t : tags)
                                if (hiddenTags.contains(t))
                                        return false;
                } else if (!hiddenSpaces.empty()) {
                        for (const auto &t : parents)
                                if (hiddenSpaces.contains(t))
                                        return false;
                } else if (sourceModel()
                             ->data(sourceModel()->index(sourceRow, 0), RoomlistModel::IsSpace)
                             .toBool() &&
                           !parents.contains(filterStr)) {
                        return false;
                }
                return true;
        } else {
                return true;
        }
}

void
FilteredRoomlistModel::toggleTag(QString roomid, QString tag, bool on)
{
        if (on) {
                http::client()->put_tag(
                  roomid.toStdString(), tag.toStdString(), {}, [tag](mtx::http::RequestErr err) {
                          if (err) {
                                  nhlog::ui()->error("Failed to add tag: {}, {}",
                                                     tag.toStdString(),
                                                     err->matrix_error.error);
                          }
                  });
        } else {
                http::client()->delete_tag(
                  roomid.toStdString(), tag.toStdString(), [tag](mtx::http::RequestErr err) {
                          if (err) {
                                  nhlog::ui()->error("Failed to delete tag: {}, {}",
                                                     tag.toStdString(),
                                                     err->matrix_error.error);
                          }
                  });
        }
}

void
FilteredRoomlistModel::nextRoom()
{
        auto r = currentRoom();

        if (r) {
                int idx = roomidToIndex(r->roomId());
                idx++;
                if (idx < rowCount()) {
                        setCurrentRoom(
                          data(index(idx, 0), RoomlistModel::Roles::RoomId).toString());
                }
        }
}

void
FilteredRoomlistModel::previousRoom()
{
        auto r = currentRoom();

        if (r) {
                int idx = roomidToIndex(r->roomId());
                idx--;
                if (idx >= 0) {
                        setCurrentRoom(
                          data(index(idx, 0), RoomlistModel::Roles::RoomId).toString());
                }
        }
}
