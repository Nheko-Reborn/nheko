// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "RoomlistModel.h"

#include <QClipboard>
#include <QGuiApplication>

#include "Cache.h"
#include "Cache_p.h"
#include "ChatPage.h"
#include "Logging.h"
#include "MainWindow.h"
#include "MatrixClient.h"
#include "MxcImageProvider.h"
#include "TimelineModel.h"
#include "TimelineViewManager.h"
#include "UserSettingsPage.h"
#include "Utils.h"
#include "voip/CallManager.h"

#ifdef NHEKO_DBUS_SYS
#include <QDBusConnection>
#endif

RoomlistModel::RoomlistModel(TimelineViewManager *parent)
  : QAbstractListModel(parent)
  , manager(parent)
{
    [[maybe_unused]] static auto id = qRegisterMetaType<RoomPreview>();

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

    connect(
      this,
      &RoomlistModel::fetchedPreview,
      this,
      [this](QString roomid, RoomInfo info) {
          if (this->previewedRooms.contains(roomid)) {
              this->previewedRooms.insert(roomid, std::move(info));
              auto idx = this->roomidToIndex(roomid);
              emit dataChanged(index(idx),
                               index(idx),
                               {
                                 Roles::RoomName,
                                 Roles::AvatarUrl,
                                 Roles::IsSpace,
                                 Roles::IsPreviewFetched,
                                 Qt::DisplayRole,
                               });
          }
      },
      Qt::QueuedConnection);
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
      {IsDirect, "isDirect"},
      {DirectChatOtherUserId, "directChatOtherUserId"},
    };
}

QVariant
RoomlistModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= 0 && static_cast<size_t>(index.row()) < roomids.size()) {
        auto roomid = roomids.at(index.row());

        if (role == Roles::ParentSpaces) {
            auto parents = cache::client()->getParentRoomIds(roomid.toStdString());
            QStringList list;
            list.reserve(static_cast<int>(parents.size()));
            for (const auto &t : parents)
                list.push_back(QString::fromStdString(t));
            return list;
        } else if (role == Roles::RoomId) {
            return roomid;
        } else if (role == Roles::IsDirect) {
            return directChatToUser.count(roomid) > 0;
        } else if (role == Roles::DirectChatOtherUserId) {
            return directChatToUser.count(roomid) ? directChatToUser.at(roomid).front()
                                                  : QLatin1String("");
        }

        if (models.contains(roomid)) {
            auto room = models.value(roomid);
            switch (role) {
            case Roles::AvatarUrl:
                return room->roomAvatarUrl();
            case Roles::RoomName:
                return room->plainRoomName();
            case Roles::LastMessage:
                return room->lastMessage().body;
            case Roles::Time:
                return room->lastMessage().descriptiveTime;
            case Roles::Timestamp:
                return QVariant{static_cast<quint64>(room->lastMessageTimestamp())};
            case Roles::HasUnreadMessages:
                return this->roomReadStatus.count(roomid) && this->roomReadStatus.at(roomid);
            case Roles::HasLoudNotification:
                return room->hasMentions();
            case Roles::NotificationCount:
                return room->notificationCount();
            case Roles::IsInvite:
                return false;
            case Roles::IsSpace:
                return room->isSpace();
            case Roles::IsPreview:
                return false;
            case Roles::Tags: {
                auto info = cache::singleRoomInfo(roomid.toStdString());
                QStringList list;
                list.reserve(static_cast<int>(info.tags.size()));
                for (const auto &t : info.tags)
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
            case Roles::LastMessage:
                return tr("Pending invite.");
            case Roles::Time:
                return QString();
            case Roles::Timestamp:
                return QVariant{static_cast<quint64>(0)};
            case Roles::HasUnreadMessages:
            case Roles::HasLoudNotification:
                return false;
            case Roles::NotificationCount:
                return 0;
            case Roles::IsInvite:
                return true;
            case Roles::IsSpace:
                return false;
            case Roles::IsPreview:
                return false;
            case Roles::Tags:
                return QStringList();
            default:
                return {};
            }
        } else if (previewedRooms.contains(roomid) && previewedRooms.value(roomid).has_value()) {
            auto room = previewedRooms.value(roomid).value();
            switch (role) {
            case Roles::AvatarUrl:
                return QString::fromStdString(room.avatar_url);
            case Roles::RoomName:
                return QString::fromStdString(room.name);
            case Roles::LastMessage:
                return tr("Previewing this room");
            case Roles::Time:
                return QString();
            case Roles::Timestamp:
                return QVariant{static_cast<quint64>(0)};
            case Roles::HasUnreadMessages:
            case Roles::HasLoudNotification:
                return false;
            case Roles::NotificationCount:
                return 0;
            case Roles::IsInvite:
                return false;
            case Roles::IsSpace:
                return room.is_space;
            case Roles::IsPreview:
                return true;
            case Roles::IsPreviewFetched:
                return true;
            case Roles::Tags:
                return QStringList();
            default:
                return {};
            }
        } else {
            if (role == Roles::IsPreview)
                return true;
            else if (role == Roles::IsPreviewFetched)
                return false;

            switch (role) {
            case Roles::AvatarUrl:
                return QString();
            case Roles::RoomName:
                return tr("No preview available");
            case Roles::LastMessage:
                return QString();
            case Roles::Time:
                return QString();
            case Roles::Timestamp:
                return QVariant(static_cast<quint64>(0));
            case Roles::HasUnreadMessages:
            case Roles::HasLoudNotification:
                return false;
            case Roles::NotificationCount:
                return 0;
            case Roles::IsInvite:
                return false;
            case Roles::IsSpace:
                return false;
            case Roles::Tags:
                return QStringList();
            default:
                return {};
            }
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
        newRoom->setDecryptDescription(ChatPage::instance()->userSettings()->decryptSidebar());

        connect(newRoom.data(),
                &TimelineModel::newEncryptedImage,
                MainWindow::instance()->imageProvider(),
                &MxcImageProvider::addEncryptionInfo);
        connect(newRoom.data(),
                &TimelineModel::forwardToRoom,
                manager,
                &TimelineViewManager::forwardMessageToRoom);
        connect(newRoom.data(), &TimelineModel::lastMessageChanged, this, [room_id, this]() {
            auto idx = this->roomidToIndex(room_id);
            emit dataChanged(index(idx),
                             index(idx),
                             {
                               Roles::HasLoudNotification,
                               Roles::LastMessage,
                               Roles::Time,
                               Roles::Timestamp,
                               Roles::NotificationCount,
                               Qt::DisplayRole,
                             });
        });
        connect(newRoom.data(), &TimelineModel::roomAvatarUrlChanged, this, [room_id, this]() {
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
        connect(newRoom.data(), &TimelineModel::notificationsChanged, this, [room_id, this]() {
            auto idx = this->roomidToIndex(room_id);
            emit dataChanged(index(idx),
                             index(idx),
                             {
                               Roles::HasLoudNotification,
                               Roles::NotificationCount,
                               Qt::DisplayRole,
                             });

            if (getRoomById(room_id)->isSpace())
                return; // no need to update space notifications

            int total_unread_msgs = 0;

            for (const auto &room : qAsConst(models)) {
                if (!room.isNull() && !room->isSpace())
                    total_unread_msgs += room->notificationCount();
            }

            emit totalUnreadMessageCountUpdated(total_unread_msgs);
        });

        // newRoom->updateLastMessage();

        std::vector<QString> previewsToAdd;
        if (newRoom->isSpace()) {
            auto childs = cache::client()->getChildRoomIds(room_id.toStdString());
            for (const auto &c : childs) {
                auto id = QString::fromStdString(c);
                if (!(models.contains(id) || invites.contains(id) || previewedRooms.contains(id))) {
                    previewsToAdd.push_back(std::move(id));
                }
            }
        }

        bool wasInvite  = invites.contains(room_id);
        bool wasPreview = previewedRooms.contains(room_id);
        if (!suppressInsertNotification && ((!wasInvite && !wasPreview) || !previewedRooms.empty()))
            // if the old room was already in the list, don't add it. Also add all
            // previews at the same time.
            beginInsertRows(
              QModelIndex(),
              (int)roomids.size(),
              (int)(roomids.size() + previewsToAdd.size() - ((wasInvite || wasPreview) ? 1 : 0)));

        models.insert(room_id, std::move(newRoom));
        if (wasInvite) {
            auto idx = roomidToIndex(room_id);
            invites.remove(room_id);
            emit dataChanged(index(idx), index(idx));
        } else if (wasPreview) {
            auto idx = roomidToIndex(room_id);
            previewedRooms.remove(room_id);
            emit dataChanged(index(idx), index(idx));
        } else {
            roomids.push_back(room_id);
        }

        if ((wasInvite || wasPreview) && currentRoomPreview_ &&
            currentRoomPreview_->roomid() == room_id) {
            currentRoom_ = models.value(room_id);
            currentRoomPreview_.reset();
            emit currentRoomChanged();
        }

        for (auto p : previewsToAdd) {
            previewedRooms.insert(p, std::nullopt);
            roomids.push_back(std::move(p));
        }

        if (!suppressInsertNotification && ((!wasInvite && !wasPreview) || !previewedRooms.empty()))
            endInsertRows();

        emit ChatPage::instance()->newRoom(room_id);
    }
}

void
RoomlistModel::fetchPreviews(QString roomid_, const std::string &from)
{
    auto roomid = roomid_.toStdString();
    if (from.empty()) {
        // check if we need to fetch anything
        auto children = cache::client()->getChildRoomIds(roomid);
        bool fetch    = false;
        for (const auto &c : children) {
            auto id = QString::fromStdString(c);
            if (invites.contains(id) || models.contains(id) ||
                (previewedRooms.contains(id) && previewedRooms.value(id).has_value()))
                continue;
            else {
                fetch = true;
                break;
            }
        }
        if (!fetch) {
            nhlog::net()->info("Not feching previews for children of {}", roomid);
            return;
        }
    }

    nhlog::net()->info("Feching previews for children of {}", roomid);
    http::client()->get_hierarchy(
      roomid,
      [this, roomid, roomid_](const mtx::responses::HierarchyRooms &h, mtx::http::RequestErr err) {
          if (err) {
              nhlog::net()->error("Failed to fetch previews for children of {}: {}", roomid, *err);
              return;
          }

          nhlog::net()->info("Feched previews for children of {}: {}", roomid, h.rooms.size());

          for (const auto &e : h.rooms) {
              RoomInfo info{};
              info.name         = e.name;
              info.is_space     = e.room_type == mtx::events::state::room_type::space;
              info.avatar_url   = e.avatar_url;
              info.topic        = e.topic;
              info.guest_access = e.guest_can_join;
              info.join_rule    = e.join_rule;
              info.member_count = e.num_joined_members;

              emit fetchedPreview(QString::fromStdString(e.room_id), info);
          }

          if (!h.next_batch.empty())
              fetchPreviews(roomid_, h.next_batch);
      },
      from,
      50,
      1,
      false);
}

std::set<QString>
RoomlistModel::updateDMs(mtx::events::AccountDataEvent<mtx::events::account_data::Direct> event)
{
    std::set<QString> roomsToUpdate;
    std::map<QString, std::vector<QString>> directChatToUserTemp;

    for (const auto &[user, rooms] : event.content.user_to_rooms) {
        QString u = QString::fromStdString(user);

        for (const auto &r : rooms) {
            directChatToUserTemp[QString::fromStdString(r)].push_back(u);
        }
    }

    for (auto l = directChatToUser.begin(), r = directChatToUserTemp.begin();
         l != directChatToUser.end() && r != directChatToUserTemp.end();) {
        if (l == directChatToUser.end()) {
            while (r != directChatToUserTemp.end()) {
                roomsToUpdate.insert(r->first);
                ++r;
            }
        } else if (r == directChatToUserTemp.end()) {
            while (l != directChatToUser.end()) {
                roomsToUpdate.insert(l->first);
                ++l;
            }
        } else if (l->first == r->first) {
            if (l->second != r->second)
                roomsToUpdate.insert(l->first);

            ++l;
            ++r;
        } else if (l->first < r->first) {
            roomsToUpdate.insert(l->first);
            ++l;
        } else if (l->first > r->first) {
            roomsToUpdate.insert(r->first);
            ++r;
        } else {
            throw std::logic_error("Infinite loop when updating DMs!");
        }
    }

    this->directChatToUser = directChatToUserTemp;

    return roomsToUpdate;
}

void
RoomlistModel::sync(const mtx::responses::Sync &sync_)
{
    for (const auto &e : sync_.account_data.events) {
        if (auto event =
              std::get_if<mtx::events::AccountDataEvent<mtx::events::account_data::Direct>>(&e)) {
            auto updatedDMs = updateDMs(*event);
            for (const auto &r : updatedDMs) {
                if (auto idx = roomidToIndex(r); idx != -1)
                    emit dataChanged(index(idx), index(idx), {IsDirect, DirectChatOtherUserId});
            }
        }
    }

    for (const auto &[room_id, room] : sync_.rooms.join) {
        auto qroomid = QString::fromStdString(room_id);

        // addRoom will only add the room, if it doesn't exist
        addRoom(qroomid);
        const auto &room_model = models.value(qroomid);
        room_model->sync(room);
        // room_model->addEvents(room.timeline);
        connect(room_model.data(),
                &TimelineModel::newCallEvent,
                ChatPage::instance()->callManager(),
                &CallManager::syncEvent,
                Qt::UniqueConnection);

        if (ChatPage::instance()->userSettings()->typingNotifications()) {
            for (const auto &ev : room.ephemeral.events) {
                if (auto t =
                      std::get_if<mtx::events::EphemeralEvent<mtx::events::ephemeral::Typing>>(
                        &ev)) {
                    std::vector<QString> typing;
                    typing.reserve(t->content.user_ids.size());
                    for (const auto &user : t->content.user_ids) {
                        if (user != http::client()->user_id().to_string())
                            typing.push_back(QString::fromStdString(user));
                    }
                    room_model->updateTypingUsers(typing);
                }
            }
        }
        for (const auto &e : room.account_data.events) {
            if (std::holds_alternative<
                  mtx::events::AccountDataEvent<mtx::events::account_data::Tags>>(e)) {
                if (auto idx = roomidToIndex(qroomid); idx != -1)
                    emit dataChanged(index(idx), index(idx), {Tags});
            }
        }
    }

    for (const auto &[room_id, room] : sync_.rooms.leave) {
        (void)room;
        auto qroomid = QString::fromStdString(room_id);

        if ((currentRoom_ && currentRoom_->roomId() == qroomid) ||
            (currentRoomPreview_ && currentRoomPreview_->roomid() == qroomid))
            resetCurrentRoom();

        auto idx = this->roomidToIndex(qroomid);
        if (idx != -1) {
            beginRemoveRows(QModelIndex(), idx, idx);
            roomids.erase(roomids.begin() + idx);
            if (models.contains(qroomid))
                models.remove(qroomid);
            else if (invites.contains(qroomid))
                invites.remove(qroomid);
            endRemoveRows();
        }
    }

    for (const auto &[room_id, room] : sync_.rooms.invite) {
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

    auto e = cache::client()->getAccountData(mtx::events::EventType::Direct);
    if (e) {
        if (auto event =
              std::get_if<mtx::events::AccountDataEvent<mtx::events::account_data::Direct>>(
                &e.value())) {
            updateDMs(*event);
        }
    }

    invites = cache::client()->invites();
    for (auto id = invites.keyBegin(); id != invites.keyEnd(); ++id) {
        roomids.push_back(*id);
    }

    for (const auto &id : cache::client()->roomIds())
        addRoom(id, true);

    nhlog::db()->info("Restored {} rooms from cache", rowCount());

    endResetModel();

#ifdef NHEKO_DBUS_SYS
    if (MainWindow::instance()->dbusAvailable()) {
        dbusInterface_ = new NhekoDBusBackend{this};
        if (!QDBusConnection::sessionBus().registerObject(
              "/", dbusInterface_, QDBusConnection::ExportScriptableSlots))
            nhlog::ui()->warn("Failed to register rooms with D-Bus");
    }
#endif
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
RoomlistModel::joinPreview(QString roomid)
{
    if (previewedRooms.contains(roomid)) {
        ChatPage::instance()->joinRoomVia(
          roomid.toStdString(), utils::roomVias(roomid.toStdString()), false);
    }
}
void
RoomlistModel::acceptInvite(QString roomid)
{
    if (invites.contains(roomid)) {
        // Don't remove invite yet, so that we can switch to it
        auto members = cache::getMembersFromInvite(roomid.toStdString(), 0, -1);
        auto local   = utils::localUser();
        for (const auto &m : members) {
            if (m.user_id == local && m.is_direct) {
                nhlog::db()->info("marking {} as direct", roomid.toStdString());
                utils::markRoomAsDirect(roomid, members);
                break;
            }
        }

        ChatPage::instance()->joinRoom(roomid);
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
            ChatPage::instance()->leaveRoom(roomid, "");
        }
    }
}
void
RoomlistModel::leave(QString roomid, QString reason)
{
    if (models.contains(roomid)) {
        auto idx = roomidToIndex(roomid);

        if (idx != -1) {
            beginRemoveRows(QModelIndex(), idx, idx);
            roomids.erase(roomids.begin() + idx);
            models.remove(roomid);
            endRemoveRows();
            ChatPage::instance()->leaveRoom(roomid, reason);
        }
    }
}

RoomPreview
RoomlistModel::getRoomPreviewById(QString roomid) const
{
    RoomPreview preview{};

    if (invites.contains(roomid) || previewedRooms.contains(roomid)) {
        std::optional<RoomInfo> i;
        if (invites.contains(roomid)) {
            i                 = invites.value(roomid);
            preview.isInvite_ = true;
        } else {
            i                 = previewedRooms.value(roomid);
            preview.isInvite_ = false;
        }

        if (i) {
            preview.roomid_        = roomid;
            preview.roomName_      = QString::fromStdString(i->name);
            preview.roomTopic_     = QString::fromStdString(i->topic);
            preview.roomAvatarUrl_ = QString::fromStdString(i->avatar_url);
        } else {
            preview.roomid_ = roomid;
        }
    }

    return preview;
}

void
RoomlistModel::setCurrentRoom(QString roomid)
{
    if ((currentRoom_ && currentRoom_->roomId() == roomid) ||
        (currentRoomPreview_ && currentRoomPreview_->roomid() == roomid))
        return;

    if (roomid.isEmpty()) {
        currentRoom_        = nullptr;
        currentRoomPreview_ = {};
        emit currentRoomChanged();
    }

    nhlog::ui()->debug("Trying to switch to: {}", roomid.toStdString());
    if (models.contains(roomid)) {
        currentRoom_ = models.value(roomid);
        currentRoomPreview_.reset();
        emit currentRoomChanged();
        nhlog::ui()->debug("Switched to: {}", roomid.toStdString());
    } else if (invites.contains(roomid) || previewedRooms.contains(roomid)) {
        currentRoom_ = nullptr;
        std::optional<RoomInfo> i;

        RoomPreview p;

        if (invites.contains(roomid)) {
            i           = invites.value(roomid);
            p.isInvite_ = true;
        } else {
            i           = previewedRooms.value(roomid);
            p.isInvite_ = false;
        }

        if (i) {
            p.roomid_           = roomid;
            p.roomName_         = QString::fromStdString(i->name);
            p.roomTopic_        = QString::fromStdString(i->topic);
            p.roomAvatarUrl_    = QString::fromStdString(i->avatar_url);
            currentRoomPreview_ = std::move(p);
            nhlog::ui()->debug("Switched to (preview): {}",
                               currentRoomPreview_->roomid_.toStdString());
        } else {
            p.roomid_           = roomid;
            currentRoomPreview_ = p;
            nhlog::ui()->debug("Switched to (empty): {}",
                               currentRoomPreview_->roomid_.toStdString());
        }

        emit currentRoomChanged();
    }
}

namespace {
enum NotificationImportance : short
{
    ImportanceDisabled = -3,
    NoPreview          = -2,
    Preview            = -1,
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
    } else if (sourceModel()->data(idx, RoomlistModel::IsPreview).toBool()) {
        if (sourceModel()->data(idx, RoomlistModel::IsPreviewFetched).toBool())
            return Preview;
        else
            return NoPreview;
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
    hideDMs = false;

    auto hidden = UserSettings::instance()->hiddenTags();
    for (const auto &t : qAsConst(hidden)) {
        if (t.startsWith(u"tag:"))
            hiddenTags.push_back(t.mid(4));
        else if (t.startsWith(u"space:"))
            hiddenSpaces.push_back(t.mid(6));
        else if (t == QLatin1String("dm"))
            hideDMs = true;
    }

    invalidateFilter();
}

bool
FilteredRoomlistModel::filterAcceptsRow(int sourceRow, const QModelIndex &) const
{
    if (filterType == FilterBy::Nothing) {
        if (sourceModel()
              ->data(sourceModel()->index(sourceRow, 0), RoomlistModel::IsPreview)
              .toBool()) {
            return false;
        }

        if (sourceModel()
              ->data(sourceModel()->index(sourceRow, 0), RoomlistModel::IsSpace)
              .toBool()) {
            return false;
        }

        if (!hiddenTags.empty()) {
            auto tags = sourceModel()
                          ->data(sourceModel()->index(sourceRow, 0), RoomlistModel::Tags)
                          .toStringList();

            for (const auto &t : tags)
                if (hiddenTags.contains(t))
                    return false;
        }

        if (!hiddenSpaces.empty()) {
            auto parents = sourceModel()
                             ->data(sourceModel()->index(sourceRow, 0), RoomlistModel::ParentSpaces)
                             .toStringList();
            for (const auto &t : parents)
                if (hiddenSpaces.contains(t))
                    return false;
        }

        if (hideDMs) {
            return !sourceModel()
                      ->data(sourceModel()->index(sourceRow, 0), RoomlistModel::IsDirect)
                      .toBool();
        }

        return true;
    } else if (filterType == FilterBy::DirectChats) {
        if (sourceModel()
              ->data(sourceModel()->index(sourceRow, 0), RoomlistModel::IsPreview)
              .toBool()) {
            return false;
        }

        if (sourceModel()
              ->data(sourceModel()->index(sourceRow, 0), RoomlistModel::IsSpace)
              .toBool()) {
            return false;
        }

        if (!hiddenTags.empty()) {
            auto tags = sourceModel()
                          ->data(sourceModel()->index(sourceRow, 0), RoomlistModel::Tags)
                          .toStringList();

            for (const auto &t : tags)
                if (hiddenTags.contains(t))
                    return false;
        }

        if (!hiddenSpaces.empty()) {
            auto parents = sourceModel()
                             ->data(sourceModel()->index(sourceRow, 0), RoomlistModel::ParentSpaces)
                             .toStringList();
            for (const auto &t : parents)
                if (hiddenSpaces.contains(t))
                    return false;
        }

        return sourceModel()
          ->data(sourceModel()->index(sourceRow, 0), RoomlistModel::IsDirect)
          .toBool();
    } else if (filterType == FilterBy::Tag) {
        if (sourceModel()
              ->data(sourceModel()->index(sourceRow, 0), RoomlistModel::IsPreview)
              .toBool()) {
            return false;
        }

        if (sourceModel()
              ->data(sourceModel()->index(sourceRow, 0), RoomlistModel::IsSpace)
              .toBool()) {
            return false;
        }

        auto tags = sourceModel()
                      ->data(sourceModel()->index(sourceRow, 0), RoomlistModel::Tags)
                      .toStringList();

        if (!tags.contains(filterStr))
            return false;

        if (!hiddenTags.empty()) {
            for (const auto &t : tags)
                if (t != filterStr && hiddenTags.contains(t))
                    return false;
        }

        if (!hiddenSpaces.empty()) {
            auto parents = sourceModel()
                             ->data(sourceModel()->index(sourceRow, 0), RoomlistModel::ParentSpaces)
                             .toStringList();
            for (const auto &t : parents)
                if (hiddenSpaces.contains(t))
                    return false;
        }

        if (hideDMs) {
            return !sourceModel()
                      ->data(sourceModel()->index(sourceRow, 0), RoomlistModel::IsDirect)
                      .toBool();
        }

        return true;
    } else if (filterType == FilterBy::Space) {
        if (filterStr == sourceModel()
                           ->data(sourceModel()->index(sourceRow, 0), RoomlistModel::RoomId)
                           .toString())
            return true;

        auto parents = sourceModel()
                         ->data(sourceModel()->index(sourceRow, 0), RoomlistModel::ParentSpaces)
                         .toStringList();

        if (!parents.contains(filterStr))
            return false;

        if (!hiddenTags.empty()) {
            auto tags = sourceModel()
                          ->data(sourceModel()->index(sourceRow, 0), RoomlistModel::Tags)
                          .toStringList();

            for (const auto &t : tags)
                if (hiddenTags.contains(t))
                    return false;
        }

        if (!hiddenSpaces.empty()) {
            for (const auto &t : parents)
                if (t != filterStr && hiddenSpaces.contains(t))
                    return false;
        }

        if (sourceModel()
              ->data(sourceModel()->index(sourceRow, 0), RoomlistModel::IsSpace)
              .toBool() &&
            !parents.contains(filterStr)) {
            return false;
        }

        if (hideDMs) {
            return !sourceModel()
                      ->data(sourceModel()->index(sourceRow, 0), RoomlistModel::IsDirect)
                      .toBool();
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
                  nhlog::ui()->error(
                    "Failed to add tag: {}, {}", tag.toStdString(), err->matrix_error.error);
              }
          });
    } else {
        http::client()->delete_tag(
          roomid.toStdString(), tag.toStdString(), [tag](mtx::http::RequestErr err) {
              if (err) {
                  nhlog::ui()->error(
                    "Failed to delete tag: {}, {}", tag.toStdString(), err->matrix_error.error);
              }
          });
    }
}

void
FilteredRoomlistModel::copyLink(QString roomid)
{
    auto link = QStringLiteral("%1?%2").arg(TimelineModel::getBareRoomLink(roomid),
                                            TimelineModel::getRoomVias(roomid));
    QGuiApplication::clipboard()->setText(link);
}

void
FilteredRoomlistModel::nextRoomWithActivity()
{
    int roomWithMention       = -1;
    int roomWithNotification  = -1;
    int roomWithUnreadMessage = -1;
    auto r                    = currentRoom();
    int currentRoomIdx        = r ? roomidToIndex(r->roomId()) : -1;
    // first look for mentions
    for (int i = 0; i < (int)roomlistmodel->roomids.size(); i++) {
        if (i == currentRoomIdx)
            continue;
        if (this->data(index(i, 0), RoomlistModel::HasLoudNotification).toBool()) {
            roomWithMention = i;
            break;
        }
        if (roomWithNotification == -1 &&
            this->data(index(i, 0), RoomlistModel::NotificationCount).toInt() > 0) {
            roomWithNotification = i;
            // don't break, we must continue looking for rooms with mentions
        }
        if (roomWithNotification == -1 && roomWithUnreadMessage == -1 &&
            this->data(index(i, 0), RoomlistModel::HasUnreadMessages).toBool()) {
            roomWithUnreadMessage = i;
            // don't break, we must continue looking for rooms with mentions
        }
    }
    QString targetRoomId = nullptr;
    if (roomWithMention != -1) {
        targetRoomId = this->data(index(roomWithMention, 0), RoomlistModel::RoomId).toString();
        nhlog::ui()->debug("choosing {} for mentions", targetRoomId.toStdString());
    } else if (roomWithNotification != -1) {
        targetRoomId = this->data(index(roomWithNotification, 0), RoomlistModel::RoomId).toString();
        nhlog::ui()->debug("choosing {} for notifications", targetRoomId.toStdString());
    } else if (roomWithUnreadMessage != -1) {
        targetRoomId =
          this->data(index(roomWithUnreadMessage, 0), RoomlistModel::RoomId).toString();
        nhlog::ui()->debug("choosing {} for unread messages", targetRoomId.toStdString());
    }
    if (targetRoomId != nullptr) {
        setCurrentRoom(targetRoomId);
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
            setCurrentRoom(data(index(idx, 0), RoomlistModel::Roles::RoomId).toString());
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
            setCurrentRoom(data(index(idx, 0), RoomlistModel::Roles::RoomId).toString());
        }
    }
}
