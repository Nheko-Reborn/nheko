// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-FileCopyrightText: 2023 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "RoomSettings.h"

#include <QFileDialog>
#include <QImageReader>
#include <QMimeDatabase>
#include <QStandardPaths>
#include <mtx/events/event_type.hpp>
#include <mtx/responses/common.hpp>
#include <mtx/responses/media.hpp>
#include <mtxclient/http/client.hpp>

#include "Cache.h"
#include "Cache_p.h"
#include "Config.h"
#include "Logging.h"
#include "MatrixClient.h"
#include "Utils.h"

using namespace mtx::events;

RoomSettings::RoomSettings(QString roomid, QObject *parent)
  : QObject(parent)
  , roomid_{std::move(roomid)}
{
    connect(this, &RoomSettings::accessJoinRulesChanged, &RoomSettings::allowedRoomsChanged);
    retrieveRoomInfo();

    // get room setting notifications
    http::client()->get_pushrules(
      "global",
      "override",
      roomid_.toStdString(),
      [this](const mtx::pushrules::PushRule &rule, mtx::http::RequestErr err) {
          if (err) {
              if (err->status_code == 404)
                  http::client()->get_pushrules(
                    "global",
                    "room",
                    roomid_.toStdString(),
                    [this](const mtx::pushrules::PushRule &rule, mtx::http::RequestErr err) {
                        if (err) {
                            notifications_ = 2; // all messages
                            emit notificationsChanged();
                            return;
                        }

                        if (rule.enabled) {
                            notifications_ = 1; // mentions only
                            emit notificationsChanged();
                        }
                    });
              return;
          }

          if (rule.enabled) {
              notifications_ = 0; // muted
              emit notificationsChanged();
          } else {
              notifications_ = 2; // all messages
              emit notificationsChanged();
          }
      });

    // access rules
    this->accessRules_ = cache::client()
                           ->getStateEvent<mtx::events::state::JoinRules>(roomid_.toStdString())
                           .value_or(mtx::events::StateEvent<mtx::events::state::JoinRules>{})
                           .content;
    using mtx::events::state::AccessState;
    guestRules_ = info_.guest_access ? AccessState::CanJoin : AccessState::Forbidden;
    emit accessJoinRulesChanged();

    this->allowedRoomsModel = new RoomSettingsAllowedRoomsModel(this);
}

QString
RoomSettings::roomName() const
{
    return utils::replaceEmoji(QString::fromStdString(info_.name).toHtmlEscaped());
}

QString
RoomSettings::roomTopic() const
{
    return utils::replaceEmoji(
      utils::linkifyMessage(QString::fromStdString(info_.topic)
                              .toHtmlEscaped()
                              .replace(QLatin1String("\n"), QLatin1String("<br>"))));
}

QString
RoomSettings::plainRoomName() const
{
    return QString::fromStdString(info_.name);
}

QString
RoomSettings::plainRoomTopic() const
{
    return QString::fromStdString(info_.topic);
}

QString
RoomSettings::roomId() const
{
    return roomid_;
}

QString
RoomSettings::roomVersion() const
{
    return QString::fromStdString(info_.version);
}

bool
RoomSettings::isLoading() const
{
    return isLoading_;
}

QString
RoomSettings::roomAvatarUrl()
{
    return QString::fromStdString(info_.avatar_url);
}

int
RoomSettings::memberCount() const
{
    return static_cast<int>(info_.member_count);
}

void
RoomSettings::retrieveRoomInfo()
{
    try {
        usesEncryption_ = cache::isRoomEncrypted(roomid_.toStdString());
        info_           = cache::singleRoomInfo(roomid_.toStdString());
    } catch (const lmdb::error &) {
        nhlog::db()->warn("failed to retrieve room info from cache: {}", roomid_.toStdString());
    }
}

int
RoomSettings::notifications()
{
    return notifications_;
}

bool
RoomSettings::privateAccess() const
{
    return accessRules_.join_rule != mtx::events::state::JoinRule::Public;
}

bool
RoomSettings::guestAccess() const
{
    return guestRules_ == mtx::events::state::AccessState::CanJoin;
}
bool
RoomSettings::knockingEnabled() const
{
    return accessRules_.join_rule == mtx::events::state::JoinRule::Knock ||
           accessRules_.join_rule == mtx::events::state::JoinRule::KnockRestricted;
}
bool
RoomSettings::restrictedEnabled() const
{
    return accessRules_.join_rule == mtx::events::state::JoinRule::Restricted ||
           accessRules_.join_rule == mtx::events::state::JoinRule::KnockRestricted;
}

QStringList
RoomSettings::allowedRooms() const
{
    QStringList rooms;
    assert(accessRules_.allow.size() < std::numeric_limits<int>::max());
    rooms.reserve(static_cast<int>(accessRules_.allow.size()));
    for (const auto &e : accessRules_.allow) {
        if (e.type == mtx::events::state::JoinAllowanceType::RoomMembership)
            rooms.push_back(QString::fromStdString(e.room_id));
    }
    return rooms;
}
void
RoomSettings::setAllowedRooms(QStringList rooms)
{
    accessRules_.allow.clear();
    for (const auto &e : rooms) {
        accessRules_.allow.push_back(
          {mtx::events::state::JoinAllowanceType::RoomMembership, e.toStdString()});
    }
}

void
RoomSettings::enableEncryption()
{
    if (usesEncryption_)
        return;

    const auto room_id = roomid_.toStdString();
    http::client()->enable_encryption(
      room_id, [room_id, this](const mtx::responses::EventId &, mtx::http::RequestErr err) {
          if (err) {
              int status_code = static_cast<int>(err->status_code);
              nhlog::net()->warn("failed to enable encryption in room ({}): {} {}",
                                 room_id,
                                 err->matrix_error.error,
                                 status_code);
              emit displayError(tr("Failed to enable encryption: %1")
                                  .arg(QString::fromStdString(err->matrix_error.error)));
              usesEncryption_ = false;
              emit encryptionChanged();
              return;
          }

          nhlog::net()->info("enabled encryption on room ({})", room_id);
      });

    usesEncryption_ = true;
    emit encryptionChanged();
}

bool
RoomSettings::canChangeJoinRules() const
{
    try {
        return cache::hasEnoughPowerLevel(
          {EventType::RoomJoinRules}, roomid_.toStdString(), utils::localUser().toStdString());
    } catch (const lmdb::error &e) {
        nhlog::db()->warn("lmdb error: {}", e.what());
    }

    return false;
}

bool
RoomSettings::canChangeName() const
{
    try {
        return cache::hasEnoughPowerLevel(
          {EventType::RoomName}, roomid_.toStdString(), utils::localUser().toStdString());
    } catch (const lmdb::error &e) {
        nhlog::db()->warn("lmdb error: {}", e.what());
    }

    return false;
}

bool
RoomSettings::canChangeTopic() const
{
    try {
        return cache::hasEnoughPowerLevel(
          {EventType::RoomTopic}, roomid_.toStdString(), utils::localUser().toStdString());
    } catch (const lmdb::error &e) {
        nhlog::db()->warn("lmdb error: {}", e.what());
    }

    return false;
}

bool
RoomSettings::canChangeAvatar() const
{
    try {
        return cache::hasEnoughPowerLevel(
          {EventType::RoomAvatar}, roomid_.toStdString(), utils::localUser().toStdString());
    } catch (const lmdb::error &e) {
        nhlog::db()->warn("lmdb error: {}", e.what());
    }

    return false;
}

bool
RoomSettings::isEncryptionEnabled() const
{
    return usesEncryption_;
}

bool
RoomSettings::supportsKnocking() const
{
    const static std::set<std::string_view> unsupported{
      "",
      "1",
      "2",
      "3",
      "4",
      "5",
      "6",
    };
    return !unsupported.count(info_.version);
}
bool
RoomSettings::supportsRestricted() const
{
    const static std::set<std::string_view> unsupported{
      "",
      "1",
      "2",
      "3",
      "4",
      "5",
      "6",
      "7",
    };
    return !unsupported.count(info_.version);
}
bool
RoomSettings::supportsKnockRestricted() const
{
    const static std::set<std::string_view> unsupported{
      "",
      "1",
      "2",
      "3",
      "4",
      "5",
      "6",
      "7",
      "8",
      "9",
    };
    return !unsupported.count(info_.version);
}

void
RoomSettings::changeNotifications(int currentIndex)
{
    notifications_ = currentIndex;

    std::string room_id = roomid_.toStdString();
    if (notifications_ == 0) {
        // mute room
        // delete old rule first, then add new rule
        mtx::pushrules::PushRule rule;
        rule.actions = {mtx::pushrules::actions::dont_notify{}};
        mtx::pushrules::PushCondition condition;
        condition.kind    = "event_match";
        condition.key     = "room_id";
        condition.pattern = room_id;
        rule.conditions   = {condition};

        http::client()->put_pushrules(
          "global", "override", room_id, rule, [room_id](mtx::http::RequestErr &err) {
              if (err)
                  nhlog::net()->error("failed to set pushrule for room {}: {} {}",
                                      room_id,
                                      static_cast<int>(err->status_code),
                                      err->matrix_error.error);
              http::client()->delete_pushrules(
                "global", "room", room_id, [room_id](mtx::http::RequestErr &) {});
          });
    } else if (notifications_ == 1) {
        // mentions only
        // delete old rule first, then add new rule
        mtx::pushrules::PushRule rule;
        rule.actions = {mtx::pushrules::actions::dont_notify{}};
        http::client()->put_pushrules(
          "global", "room", room_id, rule, [room_id](mtx::http::RequestErr &err) {
              if (err)
                  nhlog::net()->error("failed to set pushrule for room {}: {} {}",
                                      room_id,
                                      static_cast<int>(err->status_code),
                                      err->matrix_error.error);
              http::client()->delete_pushrules(
                "global", "override", room_id, [room_id](mtx::http::RequestErr &) {});
          });
    } else {
        // all messages
        http::client()->delete_pushrules(
          "global", "override", room_id, [room_id](mtx::http::RequestErr &) {
              http::client()->delete_pushrules(
                "global", "room", room_id, [room_id](mtx::http::RequestErr &) {});
          });
    }
}

void
RoomSettings::changeAccessRules(bool private_,
                                bool guestsAllowed,
                                bool knockingAllowed,
                                bool restrictedAllowed)
{
    using namespace mtx::events::state;

    auto guest_access = [guestsAllowed]() -> state::GuestAccess {
        state::GuestAccess event;

        if (guestsAllowed)
            event.guest_access = state::AccessState::CanJoin;
        else
            event.guest_access = state::AccessState::Forbidden;

        return event;
    }();

    auto join_rule = [this, private_, knockingAllowed, restrictedAllowed]() -> state::JoinRules {
        state::JoinRules event = this->accessRules_;

        if (!private_) {
            event.join_rule = state::JoinRule::Public;
        } else if (knockingAllowed && restrictedAllowed && supportsKnockRestricted()) {
            event.join_rule = state::JoinRule::KnockRestricted;
        } else if (knockingAllowed && supportsKnocking()) {
            event.join_rule = state::JoinRule::Knock;
        } else if (restrictedAllowed && supportsRestricted()) {
            event.join_rule = state::JoinRule::Restricted;
        } else {
            event.join_rule = state::JoinRule::Invite;
        }

        return event;
    }();

    updateAccessRules(roomid_.toStdString(), join_rule, guest_access);
}

void
RoomSettings::changeName(const QString &name)
{
    // Check if the values are changed from the originals.
    auto newName = name.trimmed().toStdString();

    if (newName == info_.name) {
        return;
    }

    using namespace mtx::events;
    auto proxy = std::make_shared<ThreadProxy>();
    connect(proxy.get(), &ThreadProxy::nameEventSent, this, [this](const QString &newRoomName) {
        this->info_.name = newRoomName.toStdString();
        emit roomNameChanged();
    });
    connect(proxy.get(), &ThreadProxy::error, this, &RoomSettings::displayError);

    state::Name body;
    body.name = newName;

    http::client()->send_state_event(
      roomid_.toStdString(),
      body,
      [proxy, newName](const mtx::responses::EventId &, mtx::http::RequestErr err) {
          if (err) {
              emit proxy->error(QString::fromStdString(err->matrix_error.error));
              return;
          }

          emit proxy->nameEventSent(QString::fromStdString(newName));
      });
}

void
RoomSettings::changeTopic(const QString &topic)
{
    // Check if the values are changed from the originals.
    auto newTopic = topic.trimmed().toStdString();

    if (newTopic == info_.topic) {
        return;
    }

    using namespace mtx::events;
    auto proxy = std::make_shared<ThreadProxy>();
    connect(proxy.get(), &ThreadProxy::topicEventSent, this, [this](const QString &newRoomTopic) {
        this->info_.topic = newRoomTopic.toStdString();
        emit roomTopicChanged();
    });
    connect(proxy.get(), &ThreadProxy::error, this, &RoomSettings::displayError);

    state::Topic body;
    body.topic = newTopic;

    http::client()->send_state_event(
      roomid_.toStdString(),
      body,
      [proxy, newTopic](const mtx::responses::EventId &, mtx::http::RequestErr err) {
          if (err) {
              emit proxy->error(QString::fromStdString(err->matrix_error.error));
              return;
          }

          emit proxy->topicEventSent(QString::fromStdString(newTopic));
      });
}

void
RoomSettings::updateAccessRules(const std::string &room_id,
                                const mtx::events::state::JoinRules &join_rule,
                                const mtx::events::state::GuestAccess &guest_access)
{
    isLoading_            = true;
    allowedRoomsModified_ = false;
    emit loadingChanged();
    emit allowedRoomsModifiedChanged();

    http::client()->send_state_event(
      room_id,
      join_rule,
      [this, room_id, guest_access, join_rule](const mtx::responses::EventId &,
                                               mtx::http::RequestErr err) {
          if (err) {
              nhlog::net()->warn("failed to send m.room.join_rule: {} {}",
                                 static_cast<int>(err->status_code),
                                 err->matrix_error.error);
              emit displayError(QString::fromStdString(err->matrix_error.error));
              isLoading_ = false;
              emit loadingChanged();
              return;
          }

          http::client()->send_state_event(
            room_id,
            guest_access,
            [this, join_rule](const mtx::responses::EventId &, mtx::http::RequestErr err) {
                if (err) {
                    nhlog::net()->warn("failed to send m.room.guest_access: {} {}",
                                       static_cast<int>(err->status_code),
                                       err->matrix_error.error);
                    emit displayError(QString::fromStdString(err->matrix_error.error));
                }

                isLoading_ = false;
                emit loadingChanged();

                this->accessRules_ = join_rule;
                emit accessJoinRulesChanged();
            });
      });
}

void
RoomSettings::stopLoading()
{
    isLoading_ = false;
    emit loadingChanged();
}

void
RoomSettings::avatarChanged()
{
    retrieveRoomInfo();
    emit avatarUrlChanged();
}

void
RoomSettings::updateAvatar()
{
    const QString picturesFolder =
      QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    const QString fileName = QFileDialog::getOpenFileName(
      nullptr, tr("Select an avatar"), picturesFolder, tr("All Files (*)"));

    if (fileName.isEmpty())
        return;

    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForFile(fileName, QMimeDatabase::MatchContent);

    const auto format = mime.name().split(QStringLiteral("/"))[0];

    QFile file{fileName, this};
    if (format != QLatin1String("image")) {
        emit displayError(tr("The selected file is not an image"));
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        emit displayError(tr("Error while reading file: %1").arg(file.errorString()));
        return;
    }

    isLoading_ = true;
    emit loadingChanged();

    // Events emitted from the http callbacks (different threads) will
    // be queued back into the UI thread through this proxy object.
    auto proxy = std::make_shared<ThreadProxy>();
    connect(proxy.get(), &ThreadProxy::error, this, &RoomSettings::displayError);
    connect(proxy.get(), &ThreadProxy::stopLoading, this, &RoomSettings::stopLoading);

    const auto bin        = file.peek(file.size());
    const auto payload    = std::string(bin.data(), bin.size());
    const auto dimensions = QImageReader(&file).size();

    // First we need to create a new mxc URI
    // (i.e upload media to the Matrix content repository) for the new avatar.
    http::client()->upload(
      payload,
      mime.name().toStdString(),
      QFileInfo(fileName).fileName().toStdString(),
      [proxy = std::move(proxy),
       dimensions,
       payload,
       mimetype = mime.name().toStdString(),
       size     = payload.size(),
       room_id  = roomid_.toStdString(),
       content = std::move(bin)](const mtx::responses::ContentURI &res, mtx::http::RequestErr err) {
          if (err) {
              emit proxy->stopLoading();
              emit proxy->error(tr("Failed to upload image: %s")
                                  .arg(QString::fromStdString(err->matrix_error.error)));
              return;
          }

          using namespace mtx::events;
          state::Avatar avatar_event;
          avatar_event.image_info.w        = dimensions.width();
          avatar_event.image_info.h        = dimensions.height();
          avatar_event.image_info.mimetype = mimetype;
          avatar_event.image_info.size     = size;
          avatar_event.url                 = res.content_uri;

          http::client()->send_state_event(
            room_id,
            avatar_event,
            [content = std::move(content),
             proxy = std::move(proxy)](const mtx::responses::EventId &, mtx::http::RequestErr err) {
                if (err) {
                    emit proxy->error(tr("Failed to upload image: %s")
                                        .arg(QString::fromStdString(err->matrix_error.error)));
                    return;
                }

                emit proxy->stopLoading();
            });
      });
}

RoomSettingsAllowedRoomsModel::RoomSettingsAllowedRoomsModel(RoomSettings *parent)
  : QAbstractListModel(parent)
  , settings(parent)
{
    this->allowedRoomIds = settings->allowedRooms();

    auto prIds = cache::client()->getParentRoomIds(settings->roomId().toStdString());
    for (const auto &prId : prIds) {
        this->parentSpaces.insert(QString::fromStdString(prId));
    }

    this->listedRoomIds = QStringList(parentSpaces.begin(), parentSpaces.end());

    for (const auto &e : qAsConst(this->allowedRoomIds)) {
        if (!this->parentSpaces.count(e))
            this->listedRoomIds.push_back(e);
    }
}

QHash<int, QByteArray>
RoomSettingsAllowedRoomsModel::roleNames() const
{
    return {
      {Roles::Name, "name"},
      {Roles::IsAllowed, "allowed"},
      {Roles::IsSpaceParent, "isParent"},
    };
}

int
RoomSettingsAllowedRoomsModel::rowCount(const QModelIndex &) const
{
    return listedRoomIds.size();
}

QVariant
RoomSettingsAllowedRoomsModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() > listedRoomIds.size())
        return {};

    if (role == Roles::IsAllowed) {
        return allowedRoomIds.contains(listedRoomIds.at(index.row()));
    } else if (role == Roles::IsSpaceParent) {
        return parentSpaces.find(listedRoomIds.at(index.row())) != parentSpaces.cend();
    } else if (role == Roles::Name) {
        auto id   = listedRoomIds.at(index.row());
        auto info = cache::client()->getRoomInfo({
          id.toStdString(),
        });
        if (!info.empty())
            return QString::fromStdString(info[id].name);
        else
            return "";
    } else {
        return {};
    }
}

bool
RoomSettingsAllowedRoomsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.row() < 0 || index.row() > listedRoomIds.size())
        return false;

    if (role != Roles::IsAllowed)
        return false;

    if (value.toBool()) {
        if (!allowedRoomIds.contains(listedRoomIds.at(index.row())))
            allowedRoomIds.push_back(listedRoomIds.at(index.row()));
    } else {
        allowedRoomIds.removeAll(listedRoomIds.at(index.row()));
    }

    return true;
}

void
RoomSettingsAllowedRoomsModel::addRoom(QString room)
{
    if (listedRoomIds.contains(room) || !room.startsWith('!'))
        return;

    beginInsertRows(QModelIndex(), listedRoomIds.size(), listedRoomIds.size());
    listedRoomIds.push_back(room);
    allowedRoomIds.push_back(room);
    endInsertRows();
}

void
RoomSettings::applyAllowedFromModel()
{
    this->setAllowedRooms(this->allowedRoomsModel->allowedRoomIds);
    this->allowedRoomsModified_ = true;
    emit allowedRoomsModifiedChanged();
}
