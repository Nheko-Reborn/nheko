#include "RoomSettings.h"

#include <mtx/responses/common.hpp>
#include <mtx/responses/media.hpp>

#include "Cache.h"
#include "Logging.h"
#include "MatrixClient.h"
#include "Utils.h"

using namespace mtx::events;

RoomSettings::RoomSettings(QString roomid, QObject *parent)
  : roomid_{std::move(roomid)}
  , QObject(parent)
{
        retrieveRoomInfo();

        // get room setting notifications
        http::client()->get_pushrules(
          "global",
          "override",
          roomid_.toStdString(),
          [this](const mtx::pushrules::PushRule &rule, mtx::http::RequestErr &err) {
                  if (err) {
                          if (err->status_code == boost::beast::http::status::not_found)
                                  http::client()->get_pushrules(
                                    "global",
                                    "room",
                                    roomid_.toStdString(),
                                    [this](const mtx::pushrules::PushRule &rule,
                                           mtx::http::RequestErr &err) {
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
        if (info_.join_rule == state::JoinRule::Public) {
                if (info_.guest_access) {
                        accessRules_ = 0;
                } else {
                        accessRules_ = 1;
                }
        } else {
                accessRules_ = 2;
        }
        emit accessJoinRulesChanged();
}

QString
RoomSettings::roomName() const
{
        return QString(info_.name.c_str());
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

int
RoomSettings::memberCount() const
{
        return info_.member_count;
}

void
RoomSettings::retrieveRoomInfo()
{
        try {
                usesEncryption_ = cache::isRoomEncrypted(roomid_.toStdString());
                info_           = cache::singleRoomInfo(roomid_.toStdString());
                // setAvatar();
        } catch (const lmdb::error &) {
                nhlog::db()->warn("failed to retrieve room info from cache: {}",
                                  roomid_.toStdString());
        }
}

int
RoomSettings::notifications()
{
        return notifications_;
}

int
RoomSettings::accessJoinRules()
{
        return accessRules_;
}

bool
RoomSettings::respondsToKeyRequests()
{
        return usesEncryption_ && utils::respondsToKeyRequests(roomid_);
}

void
RoomSettings::changeKeyRequestsPreference(bool isOn)
{
        utils::setKeyRequestsPreference(roomid_, isOn);
        emit keyRequestsChanged();
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
                          //emit enableEncryptionError(
                          //  tr("Failed to enable encryption: %1")
                          //    .arg(QString::fromStdString(err->matrix_error.error)));
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
                return cache::hasEnoughPowerLevel({EventType::RoomJoinRules},
                                                  roomid_.toStdString(),
                                                  utils::localUser().toStdString());
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
RoomSettings::changeAccessRules(int index)
{
        using namespace mtx::events::state;

        auto guest_access = [](int index) -> state::GuestAccess {
                state::GuestAccess event;

                if (index == 0)
                        event.guest_access = state::AccessState::CanJoin;
                else
                        event.guest_access = state::AccessState::Forbidden;

                return event;
        }(index);

        auto join_rule = [](int index) -> state::JoinRules {
                state::JoinRules event;

                switch (index) {
                case 0:
                case 1:
                        event.join_rule = state::JoinRule::Public;
                        break;
                default:
                        event.join_rule = state::JoinRule::Invite;
                }

                return event;
        }(index);

        updateAccessRules(roomid_.toStdString(), join_rule, guest_access);
}

void
RoomSettings::updateAccessRules(const std::string &room_id,
                                const mtx::events::state::JoinRules &join_rule,
                                const mtx::events::state::GuestAccess &guest_access)
{
        // startLoadingSpinner();
        // resetErrorLabel();

        http::client()->send_state_event(
          room_id,
          join_rule,
          [this, room_id, guest_access](const mtx::responses::EventId &,
                                        mtx::http::RequestErr err) {
                  if (err) {
                          nhlog::net()->warn("failed to send m.room.join_rule: {} {}",
                                             static_cast<int>(err->status_code),
                                             err->matrix_error.error);
                          // emit showErrorMessage(QString::fromStdString(err->matrix_error.error));

                          return;
                  }

                  http::client()->send_state_event(
                    room_id,
                    guest_access,
                    [this](const mtx::responses::EventId &, mtx::http::RequestErr err) {
                            if (err) {
                                    nhlog::net()->warn("failed to send m.room.guest_access: {} {}",
                                                       static_cast<int>(err->status_code),
                                                       err->matrix_error.error);
                                    // emit showErrorMessage(
                                    //  QString::fromStdString(err->matrix_error.error));

                                    return;
                            }

                            // emit signal that stops loading spinner and reset error label
                    });
          });
}