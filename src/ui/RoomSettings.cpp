// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "RoomSettings.h"

#include <QApplication>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QImageReader>
#include <QMimeDatabase>
#include <QStandardPaths>
#include <QVBoxLayout>
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
#include "ui/TextField.h"

using namespace mtx::events;

EditModal::EditModal(const QString &roomId, QWidget *parent)
  : QWidget(parent)
  , roomId_{roomId}
{
    setAutoFillBackground(true);
    setAttribute(Qt::WA_DeleteOnClose, true);
    setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint);
    setWindowModality(Qt::WindowModal);

    QFont largeFont;
    largeFont.setPointSizeF(largeFont.pointSizeF() * 1.4);
    setMinimumWidth(conf::window::minModalWidth);
    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

    auto layout = new QVBoxLayout(this);

    applyBtn_  = new QPushButton(tr("Apply"), this);
    cancelBtn_ = new QPushButton(tr("Cancel"), this);
    cancelBtn_->setDefault(true);

    auto btnLayout = new QHBoxLayout;
    btnLayout->addStretch(1);
    btnLayout->setSpacing(15);
    btnLayout->addWidget(cancelBtn_);
    btnLayout->addWidget(applyBtn_);

    nameInput_ = new TextField(this);
    nameInput_->setLabel(tr("Name").toUpper());
    topicInput_ = new TextField(this);
    topicInput_->setLabel(tr("Topic").toUpper());

    errorField_ = new QLabel(this);
    errorField_->setWordWrap(true);
    errorField_->hide();

    layout->addWidget(nameInput_);
    layout->addWidget(topicInput_);
    layout->addLayout(btnLayout, 1);

    auto labelLayout = new QHBoxLayout;
    labelLayout->setAlignment(Qt::AlignHCenter);
    labelLayout->addWidget(errorField_);
    layout->addLayout(labelLayout);

    connect(applyBtn_, &QPushButton::clicked, this, &EditModal::applyClicked);
    connect(cancelBtn_, &QPushButton::clicked, this, &EditModal::close);

    auto window = QApplication::activeWindow();

    if (window != nullptr) {
        auto center = window->frameGeometry().center();
        move(center.x() - (width() * 0.5), center.y() - (height() * 0.5));
    }
}

void
EditModal::topicEventSent(const QString &topic)
{
    errorField_->hide();
    emit topicChanged(topic);
    close();
}

void
EditModal::nameEventSent(const QString &name)
{
    errorField_->hide();
    emit nameChanged(name);
    close();
}

void
EditModal::error(const QString &msg)
{
    errorField_->setText(msg);
    errorField_->show();
}

void
EditModal::applyClicked()
{
    // Check if the values are changed from the originals.
    auto newName  = nameInput_->text().trimmed();
    auto newTopic = topicInput_->text().trimmed();

    errorField_->hide();

    if (newName == initialName_ && newTopic == initialTopic_) {
        close();
        return;
    }

    using namespace mtx::events;
    auto proxy = std::make_shared<ThreadProxy>();
    connect(proxy.get(), &ThreadProxy::topicEventSent, this, &EditModal::topicEventSent);
    connect(proxy.get(), &ThreadProxy::nameEventSent, this, &EditModal::nameEventSent);
    connect(proxy.get(), &ThreadProxy::error, this, &EditModal::error);

    if (newName != initialName_ && !newName.isEmpty()) {
        state::Name body;
        body.name = newName.toStdString();

        http::client()->send_state_event(
          roomId_.toStdString(),
          body,
          [proxy, newName](const mtx::responses::EventId &, mtx::http::RequestErr err) {
              if (err) {
                  emit proxy->error(QString::fromStdString(err->matrix_error.error));
                  return;
              }

              emit proxy->nameEventSent(newName);
          });
    }

    if (newTopic != initialTopic_ && !newTopic.isEmpty()) {
        state::Topic body;
        body.topic = newTopic.toStdString();

        http::client()->send_state_event(
          roomId_.toStdString(),
          body,
          [proxy, newTopic](const mtx::responses::EventId &, mtx::http::RequestErr err) {
              if (err) {
                  emit proxy->error(QString::fromStdString(err->matrix_error.error));
                  return;
              }

              emit proxy->topicEventSent(newTopic);
          });
    }
}

void
EditModal::setFields(const QString &roomName, const QString &roomTopic)
{
    initialName_  = roomName;
    initialTopic_ = roomTopic;

    nameInput_->setText(roomName);
    topicInput_->setText(roomTopic);
}

RoomSettings::RoomSettings(QString roomid, QObject *parent)
  : QObject(parent)
  , roomid_{std::move(roomid)}
{
    retrieveRoomInfo();

    // get room setting notifications
    http::client()->get_pushrules(
      "global",
      "override",
      roomid_.toStdString(),
      [this](const mtx::pushrules::PushRule &rule, mtx::http::RequestErr &err) {
          if (err) {
              if (err->status_code == 404)
                  http::client()->get_pushrules(
                    "global",
                    "room",
                    roomid_.toStdString(),
                    [this](const mtx::pushrules::PushRule &rule, mtx::http::RequestErr &err) {
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
    } else if (info_.join_rule == state::JoinRule::Invite) {
        accessRules_ = 2;
    } else if (info_.join_rule == state::JoinRule::Knock) {
        accessRules_ = 3;
    } else if (info_.join_rule == state::JoinRule::Restricted) {
        accessRules_ = 4;
    }
    emit accessJoinRulesChanged();
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
    return info_.member_count;
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

int
RoomSettings::accessJoinRules()
{
    return accessRules_;
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
RoomSettings::canChangeNameAndTopic() const
{
    try {
        return cache::hasEnoughPowerLevel({EventType::RoomName, EventType::RoomTopic},
                                          roomid_.toStdString(),
                                          utils::localUser().toStdString());
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
    return info_.version != "" && info_.version != "1" && info_.version != "2" &&
           info_.version != "3" && info_.version != "4" && info_.version != "5" &&
           info_.version != "6";
}
bool
RoomSettings::supportsRestricted() const
{
    return info_.version != "" && info_.version != "1" && info_.version != "2" &&
           info_.version != "3" && info_.version != "4" && info_.version != "5" &&
           info_.version != "6" && info_.version != "7";
}

void
RoomSettings::openEditModal()
{
    retrieveRoomInfo();

    auto modal = new EditModal(roomid_);
    modal->setFields(QString::fromStdString(info_.name), QString::fromStdString(info_.topic));
    modal->raise();
    modal->show();
    connect(modal, &EditModal::nameChanged, this, [this](const QString &newName) {
        info_.name = newName.toStdString();
        emit roomNameChanged();
    });

    connect(modal, &EditModal::topicChanged, this, [this](const QString &newTopic) {
        info_.topic = newTopic.toStdString();
        emit roomTopicChanged();
    });
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
        case 2:
            event.join_rule = state::JoinRule::Invite;
            break;
        case 3:
            event.join_rule = state::JoinRule::Knock;
            break;
        case 4:
            event.join_rule = state::JoinRule::Restricted;
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
    isLoading_ = true;
    emit loadingChanged();

    http::client()->send_state_event(
      room_id,
      join_rule,
      [this, room_id, guest_access](const mtx::responses::EventId &, mtx::http::RequestErr err) {
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
            [this](const mtx::responses::EventId &, mtx::http::RequestErr err) {
                if (err) {
                    nhlog::net()->warn("failed to send m.room.guest_access: {} {}",
                                       static_cast<int>(err->status_code),
                                       err->matrix_error.error);
                    emit displayError(QString::fromStdString(err->matrix_error.error));
                }

                isLoading_ = false;
                emit loadingChanged();
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
