// SPDX-FileCopyrightText: 2017 Konstantinos Sideris <siderisk@auth.gr>
// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QApplication>
#include <QImageReader>
#include <QInputDialog>
#include <QMessageBox>
#include <QSettings>
#include <QShortcut>

#include <mtx/responses.hpp>

#include "AvatarProvider.h"
#include "Cache.h"
#include "Cache_p.h"
#include "CallManager.h"
#include "ChatPage.h"
#include "DeviceVerificationFlow.h"
#include "EventAccessors.h"
#include "Logging.h"
#include "MainWindow.h"
#include "MatrixClient.h"
#include "Olm.h"
#include "UserSettingsPage.h"
#include "Utils.h"
#include "ui/OverlayModal.h"
#include "ui/Theme.h"
#include "ui/UserProfile.h"

#include "notifications/Manager.h"

#include "dialogs/ReadReceipts.h"
#include "timeline/TimelineViewManager.h"

#include "blurhash.hpp"

// TODO: Needs to be updated with an actual secret.
static const std::string STORAGE_SECRET_KEY("secret");

ChatPage *ChatPage::instance_             = nullptr;
constexpr int CHECK_CONNECTIVITY_INTERVAL = 15'000;
constexpr int RETRY_TIMEOUT               = 5'000;
constexpr size_t MAX_ONETIME_KEYS         = 50;

Q_DECLARE_METATYPE(std::optional<mtx::crypto::EncryptedFile>)
Q_DECLARE_METATYPE(std::optional<RelatedInfo>)
Q_DECLARE_METATYPE(mtx::presence::PresenceState)
Q_DECLARE_METATYPE(mtx::secret_storage::AesHmacSha2KeyDescription)
Q_DECLARE_METATYPE(SecretsToDecrypt)

ChatPage::ChatPage(QSharedPointer<UserSettings> userSettings, QWidget *parent)
  : QWidget(parent)
  , isConnected_(true)
  , userSettings_{userSettings}
  , notificationsManager(this)
  , callManager_(new CallManager(this))
{
        setObjectName("chatPage");

        instance_ = this;

        qRegisterMetaType<std::optional<mtx::crypto::EncryptedFile>>();
        qRegisterMetaType<std::optional<RelatedInfo>>();
        qRegisterMetaType<mtx::presence::PresenceState>();
        qRegisterMetaType<mtx::secret_storage::AesHmacSha2KeyDescription>();
        qRegisterMetaType<SecretsToDecrypt>();

        topLayout_ = new QHBoxLayout(this);
        topLayout_->setSpacing(0);
        topLayout_->setMargin(0);

        view_manager_ = new TimelineViewManager(callManager_, this);

        topLayout_->addWidget(view_manager_->getWidget());

        connect(this,
                &ChatPage::downloadedSecrets,
                this,
                &ChatPage::decryptDownloadedSecrets,
                Qt::QueuedConnection);

        connect(this, &ChatPage::connectionLost, this, [this]() {
                nhlog::net()->info("connectivity lost");
                isConnected_ = false;
                http::client()->shutdown();
        });
        connect(this, &ChatPage::connectionRestored, this, [this]() {
                nhlog::net()->info("trying to re-connect");
                isConnected_ = true;

                // Drop all pending connections.
                http::client()->shutdown();
                trySync();
        });

        connectivityTimer_.setInterval(CHECK_CONNECTIVITY_INTERVAL);
        connect(&connectivityTimer_, &QTimer::timeout, this, [=]() {
                if (http::client()->access_token().empty()) {
                        connectivityTimer_.stop();
                        return;
                }

                http::client()->versions(
                  [this](const mtx::responses::Versions &, mtx::http::RequestErr err) {
                          if (err) {
                                  emit connectionLost();
                                  return;
                          }

                          if (!isConnected_)
                                  emit connectionRestored();
                  });
        });

        connect(this, &ChatPage::loggedOut, this, &ChatPage::logout);

        connect(view_manager_, &TimelineViewManager::inviteUsers, this, [this](QStringList users) {
                const auto room_id = currentRoom().toStdString();

                for (int ii = 0; ii < users.size(); ++ii) {
                        QTimer::singleShot(ii * 500, this, [this, room_id, ii, users]() {
                                const auto user = users.at(ii);

                                http::client()->invite_user(
                                  room_id,
                                  user.toStdString(),
                                  [this, user](const mtx::responses::RoomInvite &,
                                               mtx::http::RequestErr err) {
                                          if (err) {
                                                  emit showNotification(
                                                    tr("Failed to invite user: %1").arg(user));
                                                  return;
                                          }

                                          emit showNotification(tr("Invited user: %1").arg(user));
                                  });
                        });
                }
        });

        connect(this, &ChatPage::leftRoom, this, &ChatPage::removeRoom);
        connect(this, &ChatPage::newRoom, this, &ChatPage::changeRoom, Qt::QueuedConnection);
        connect(this, &ChatPage::notificationsRetrieved, this, &ChatPage::sendNotifications);
        connect(this,
                &ChatPage::highlightedNotifsRetrieved,
                this,
                [](const mtx::responses::Notifications &notif) {
                        try {
                                cache::saveTimelineMentions(notif);
                        } catch (const lmdb::error &e) {
                                nhlog::db()->error("failed to save mentions: {}", e.what());
                        }
                });

        connect(&notificationsManager,
                &NotificationsManager::notificationClicked,
                this,
                [this](const QString &roomid, const QString &eventid) {
                        Q_UNUSED(eventid)
                        view_manager_->rooms()->setCurrentRoom(roomid);
                        activateWindow();
                });
        connect(&notificationsManager,
                &NotificationsManager::sendNotificationReply,
                this,
                [this](const QString &roomid, const QString &eventid, const QString &body) {
                        view_manager_->rooms()->setCurrentRoom(roomid);
                        view_manager_->queueReply(roomid, eventid, body);
                        activateWindow();
                });

        connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, [this]() {
                // ensure the qml context is shutdown before we destroy all other singletons
                // Otherwise Qml will try to access the room list or settings, after they have been
                // destroyed
                topLayout_->removeWidget(view_manager_->getWidget());
                delete view_manager_->getWidget();
        });

        connect(
          this,
          &ChatPage::initializeViews,
          view_manager_,
          [this](const mtx::responses::Rooms &rooms) { view_manager_->sync(rooms); },
          Qt::QueuedConnection);
        connect(this,
                &ChatPage::initializeEmptyViews,
                view_manager_,
                &TimelineViewManager::initializeRoomlist);
        connect(
          this, &ChatPage::chatFocusChanged, view_manager_, &TimelineViewManager::chatFocusChanged);
        connect(this, &ChatPage::syncUI, this, [this](const mtx::responses::Rooms &rooms) {
                view_manager_->sync(rooms);

                bool hasNotifications = false;
                for (const auto &room : rooms.join) {
                        if (room.second.unread_notifications.notification_count > 0)
                                hasNotifications = true;
                }

                if (hasNotifications && userSettings_->hasNotifications())
                        http::client()->notifications(
                          5,
                          "",
                          "",
                          [this](const mtx::responses::Notifications &res,
                                 mtx::http::RequestErr err) {
                                  if (err) {
                                          nhlog::net()->warn(
                                            "failed to retrieve notifications: {} ({})",
                                            err->matrix_error.error,
                                            static_cast<int>(err->status_code));
                                          return;
                                  }

                                  emit notificationsRetrieved(std::move(res));
                          });
        });

        connect(
          this, &ChatPage::tryInitialSyncCb, this, &ChatPage::tryInitialSync, Qt::QueuedConnection);
        connect(this, &ChatPage::trySyncCb, this, &ChatPage::trySync, Qt::QueuedConnection);
        connect(
          this,
          &ChatPage::tryDelayedSyncCb,
          this,
          [this]() { QTimer::singleShot(RETRY_TIMEOUT, this, &ChatPage::trySync); },
          Qt::QueuedConnection);

        connect(this,
                &ChatPage::newSyncResponse,
                this,
                &ChatPage::handleSyncResponse,
                Qt::QueuedConnection);

        connect(this, &ChatPage::dropToLoginPageCb, this, &ChatPage::dropToLoginPage);

        connectCallMessage<mtx::events::msg::CallInvite>();
        connectCallMessage<mtx::events::msg::CallCandidates>();
        connectCallMessage<mtx::events::msg::CallAnswer>();
        connectCallMessage<mtx::events::msg::CallHangUp>();
}

void
ChatPage::logout()
{
        resetUI();
        deleteConfigs();

        emit closing();
        connectivityTimer_.stop();
}

void
ChatPage::dropToLoginPage(const QString &msg)
{
        nhlog::ui()->info("dropping to the login page: {}", msg.toStdString());

        http::client()->shutdown();
        connectivityTimer_.stop();

        resetUI();
        deleteConfigs();

        emit showLoginPage(msg);
}

void
ChatPage::resetUI()
{
        view_manager_->clearAll();

        emit unreadMessages(0);
}

void
ChatPage::deleteConfigs()
{
        QSettings settings;

        if (UserSettings::instance()->profile() != "") {
                settings.beginGroup("profile");
                settings.beginGroup(UserSettings::instance()->profile());
        }
        settings.beginGroup("auth");
        settings.remove("");
        settings.endGroup(); // auth

        http::client()->shutdown();
        cache::deleteData();
}

void
ChatPage::bootstrap(QString userid, QString homeserver, QString token)
{
        using namespace mtx::identifiers;

        try {
                http::client()->set_user(parse<User>(userid.toStdString()));
        } catch (const std::invalid_argument &) {
                nhlog::ui()->critical("bootstrapped with invalid user_id: {}",
                                      userid.toStdString());
        }

        http::client()->set_server(homeserver.toStdString());
        http::client()->set_access_token(token.toStdString());
        http::client()->verify_certificates(
          !UserSettings::instance()->disableCertificateValidation());

        // The Olm client needs the user_id & device_id that will be included
        // in the generated payloads & keys.
        olm::client()->set_user_id(http::client()->user_id().to_string());
        olm::client()->set_device_id(http::client()->device_id());

        try {
                cache::init(userid);

                connect(cache::client(),
                        &Cache::newReadReceipts,
                        view_manager_,
                        &TimelineViewManager::updateReadReceipts);

                connect(cache::client(),
                        &Cache::removeNotification,
                        &notificationsManager,
                        &NotificationsManager::removeNotification);

                const bool isInitialized = cache::isInitialized();
                const auto cacheVersion  = cache::formatVersion();

                callManager_->refreshTurnServer();

                if (!isInitialized) {
                        cache::setCurrentFormat();
                } else {
                        if (cacheVersion == cache::CacheVersion::Current) {
                                loadStateFromCache();
                                return;
                        } else if (cacheVersion == cache::CacheVersion::Older) {
                                if (!cache::runMigrations()) {
                                        QMessageBox::critical(
                                          this,
                                          tr("Cache migration failed!"),
                                          tr("Migrating the cache to the current version failed. "
                                             "This can have different reasons. Please open an "
                                             "issue and try to use an older version in the mean "
                                             "time. Alternatively you can try deleting the cache "
                                             "manually."));
                                        QCoreApplication::quit();
                                }
                                loadStateFromCache();
                                return;
                        } else if (cacheVersion == cache::CacheVersion::Newer) {
                                QMessageBox::critical(
                                  this,
                                  tr("Incompatible cache version"),
                                  tr("The cache on your disk is newer than this version of Nheko "
                                     "supports. Please update or clear your cache."));
                                QCoreApplication::quit();
                                return;
                        }
                }

        } catch (const lmdb::error &e) {
                nhlog::db()->critical("failure during boot: {}", e.what());
                cache::deleteData();
                nhlog::net()->info("falling back to initial sync");
        }

        try {
                // It's the first time syncing with this device
                // There isn't a saved olm account to restore.
                nhlog::crypto()->info("creating new olm account");
                olm::client()->create_new_account();
                cache::saveOlmAccount(olm::client()->save(STORAGE_SECRET_KEY));
        } catch (const lmdb::error &e) {
                nhlog::crypto()->critical("failed to save olm account {}", e.what());
                emit dropToLoginPageCb(QString::fromStdString(e.what()));
                return;
        } catch (const mtx::crypto::olm_exception &e) {
                nhlog::crypto()->critical("failed to create new olm account {}", e.what());
                emit dropToLoginPageCb(QString::fromStdString(e.what()));
                return;
        }

        getProfileInfo();
        tryInitialSync();
}

void
ChatPage::loadStateFromCache()
{
        nhlog::db()->info("restoring state from cache");

        try {
                olm::client()->load(cache::restoreOlmAccount(), STORAGE_SECRET_KEY);

                emit initializeEmptyViews();
                emit initializeMentions(cache::getTimelineMentions());

                cache::calculateRoomReadStatus();

        } catch (const mtx::crypto::olm_exception &e) {
                nhlog::crypto()->critical("failed to restore olm account: {}", e.what());
                emit dropToLoginPageCb(tr("Failed to restore OLM account. Please login again."));
                return;
        } catch (const lmdb::error &e) {
                nhlog::db()->critical("failed to restore cache: {}", e.what());
                emit dropToLoginPageCb(tr("Failed to restore save data. Please login again."));
                return;
        } catch (const json::exception &e) {
                nhlog::db()->critical("failed to parse cache data: {}", e.what());
                return;
        }

        nhlog::crypto()->info("ed25519   : {}", olm::client()->identity_keys().ed25519);
        nhlog::crypto()->info("curve25519: {}", olm::client()->identity_keys().curve25519);

        getProfileInfo();

        emit contentLoaded();

        // Start receiving events.
        emit trySyncCb();
}

void
ChatPage::removeRoom(const QString &room_id)
{
        try {
                cache::removeRoom(room_id);
                cache::removeInvite(room_id.toStdString());
        } catch (const lmdb::error &e) {
                nhlog::db()->critical("failure while removing room: {}", e.what());
                // TODO: Notify the user.
        }
}

void
ChatPage::sendNotifications(const mtx::responses::Notifications &res)
{
        for (const auto &item : res.notifications) {
                const auto event_id = mtx::accessors::event_id(item.event);

                try {
                        if (item.read) {
                                cache::removeReadNotification(event_id);
                                continue;
                        }

                        if (!cache::isNotificationSent(event_id)) {
                                const auto room_id = QString::fromStdString(item.room_id);

                                // We should only sent one notification per event.
                                cache::markSentNotification(event_id);

                                // Don't send a notification when the current room is opened.
                                if (isRoomActive(room_id))
                                        continue;

                                if (userSettings_->hasAlertOnNotification()) {
                                        QApplication::alert(this);
                                }

                                if (userSettings_->hasDesktopNotifications()) {
                                        auto info = cache::singleRoomInfo(item.room_id);

                                        AvatarProvider::resolve(
                                          QString::fromStdString(info.avatar_url),
                                          96,
                                          this,
                                          [this, item](QPixmap image) {
                                                  notificationsManager.postNotification(
                                                    item, image.toImage());
                                          });
                                }
                        }
                } catch (const lmdb::error &e) {
                        nhlog::db()->warn("error while sending notification: {}", e.what());
                }
        }
}

void
ChatPage::tryInitialSync()
{
        nhlog::crypto()->info("ed25519   : {}", olm::client()->identity_keys().ed25519);
        nhlog::crypto()->info("curve25519: {}", olm::client()->identity_keys().curve25519);

        // Upload one time keys for the device.
        nhlog::crypto()->info("generating one time keys");
        olm::client()->generate_one_time_keys(MAX_ONETIME_KEYS);

        http::client()->upload_keys(
          olm::client()->create_upload_keys_request(),
          [this](const mtx::responses::UploadKeys &res, mtx::http::RequestErr err) {
                  if (err) {
                          const int status_code = static_cast<int>(err->status_code);

                          if (status_code == 404) {
                                  nhlog::net()->warn(
                                    "skipping key uploading. server doesn't provide /keys/upload");
                                  return startInitialSync();
                          }

                          nhlog::crypto()->critical("failed to upload one time keys: {} {}",
                                                    err->matrix_error.error,
                                                    status_code);

                          QString errorMsg(tr("Failed to setup encryption keys. Server response: "
                                              "%1 %2. Please try again later.")
                                             .arg(QString::fromStdString(err->matrix_error.error))
                                             .arg(status_code));

                          emit dropToLoginPageCb(errorMsg);
                          return;
                  }

                  olm::mark_keys_as_published();

                  for (const auto &entry : res.one_time_key_counts)
                          nhlog::net()->info(
                            "uploaded {} {} one-time keys", entry.second, entry.first);

                  startInitialSync();
          });
}

void
ChatPage::startInitialSync()
{
        nhlog::net()->info("trying initial sync");

        mtx::http::SyncOpts opts;
        opts.timeout      = 0;
        opts.set_presence = currentPresence();

        http::client()->sync(
          opts, [this](const mtx::responses::Sync &res, mtx::http::RequestErr err) {
                  // TODO: Initial Sync should include mentions as well...

                  if (err) {
                          const auto error      = QString::fromStdString(err->matrix_error.error);
                          const auto msg        = tr("Please try to login again: %1").arg(error);
                          const auto err_code   = mtx::errors::to_string(err->matrix_error.errcode);
                          const int status_code = static_cast<int>(err->status_code);

                          nhlog::net()->error("initial sync error: {} {} {} {}",
                                              err->parse_error,
                                              status_code,
                                              err->error_code,
                                              err_code);

                          // non http related errors
                          if (status_code <= 0 || status_code >= 600) {
                                  startInitialSync();
                                  return;
                          }

                          switch (status_code) {
                          case 502:
                          case 504:
                          case 524: {
                                  startInitialSync();
                                  return;
                          }
                          default: {
                                  emit dropToLoginPageCb(msg);
                                  return;
                          }
                          }
                  }

                  nhlog::net()->info("initial sync completed");

                  try {
                          cache::client()->saveState(res);

                          olm::handle_to_device_messages(res.to_device.events);

                          emit initializeViews(std::move(res.rooms));
                          emit initializeMentions(cache::getTimelineMentions());

                          cache::calculateRoomReadStatus();
                  } catch (const lmdb::error &e) {
                          nhlog::db()->error("failed to save state after initial sync: {}",
                                             e.what());
                          startInitialSync();
                          return;
                  }

                  emit trySyncCb();
                  emit contentLoaded();
          });
}

void
ChatPage::handleSyncResponse(const mtx::responses::Sync &res, const std::string &prev_batch_token)
{
        try {
                if (prev_batch_token != cache::nextBatchToken()) {
                        nhlog::net()->warn("Duplicate sync, dropping");
                        return;
                }
        } catch (const lmdb::error &e) {
                nhlog::db()->warn("Logged out in the mean time, dropping sync");
        }

        nhlog::net()->debug("sync completed: {}", res.next_batch);

        // Ensure that we have enough one-time keys available.
        ensureOneTimeKeyCount(res.device_one_time_keys_count);

        // TODO: fine grained error handling
        try {
                cache::client()->saveState(res);
                olm::handle_to_device_messages(res.to_device.events);

                auto updates = cache::getRoomInfo(cache::client()->roomsWithStateUpdates(res));

                emit syncUI(res.rooms);

                // if we process a lot of syncs (1 every 200ms), this means we clean the
                // db every 100s
                static int syncCounter = 0;
                if (syncCounter++ >= 500) {
                        cache::deleteOldData();
                        syncCounter = 0;
                }
        } catch (const lmdb::map_full_error &e) {
                nhlog::db()->error("lmdb is full: {}", e.what());
                cache::deleteOldData();
        } catch (const lmdb::error &e) {
                nhlog::db()->error("saving sync response: {}", e.what());
        }

        emit trySyncCb();
}

void
ChatPage::trySync()
{
        mtx::http::SyncOpts opts;
        opts.set_presence = currentPresence();

        if (!connectivityTimer_.isActive())
                connectivityTimer_.start();

        try {
                opts.since = cache::nextBatchToken();
        } catch (const lmdb::error &e) {
                nhlog::db()->error("failed to retrieve next batch token: {}", e.what());
                return;
        }

        http::client()->sync(
          opts,
          [this, since = opts.since](const mtx::responses::Sync &res, mtx::http::RequestErr err) {
                  if (err) {
                          const auto error      = QString::fromStdString(err->matrix_error.error);
                          const auto msg        = tr("Please try to login again: %1").arg(error);
                          const auto err_code   = mtx::errors::to_string(err->matrix_error.errcode);
                          const int status_code = static_cast<int>(err->status_code);

                          if ((http::is_logged_in() &&
                               (err->matrix_error.errcode ==
                                  mtx::errors::ErrorCode::M_UNKNOWN_TOKEN ||
                                err->matrix_error.errcode ==
                                  mtx::errors::ErrorCode::M_MISSING_TOKEN)) ||
                              !http::is_logged_in()) {
                                  emit dropToLoginPageCb(msg);
                                  return;
                          }

                          nhlog::net()->error("sync error: {} {} {} {}",
                                              err->parse_error,
                                              status_code,
                                              err->error_code,
                                              err_code);
                          emit tryDelayedSyncCb();
                          return;
                  }

                  emit newSyncResponse(res, since);
          });
}

void
ChatPage::joinRoom(const QString &room)
{
        const auto room_id = room.toStdString();
        joinRoomVia(room_id, {}, false);
}

void
ChatPage::joinRoomVia(const std::string &room_id,
                      const std::vector<std::string> &via,
                      bool promptForConfirmation)
{
        if (promptForConfirmation &&
            QMessageBox::Yes !=
              QMessageBox::question(
                this,
                tr("Confirm join"),
                tr("Do you really want to join %1?").arg(QString::fromStdString(room_id))))
                return;

        http::client()->join_room(
          room_id, via, [this, room_id](const mtx::responses::RoomId &, mtx::http::RequestErr err) {
                  if (err) {
                          emit showNotification(
                            tr("Failed to join room: %1")
                              .arg(QString::fromStdString(err->matrix_error.error)));
                          return;
                  }

                  emit tr("You joined the room");

                  // We remove any invites with the same room_id.
                  try {
                          cache::removeInvite(room_id);
                  } catch (const lmdb::error &e) {
                          emit showNotification(tr("Failed to remove invite: %1").arg(e.what()));
                  }

                  view_manager_->rooms()->setCurrentRoom(QString::fromStdString(room_id));
          });
}

void
ChatPage::createRoom(const mtx::requests::CreateRoom &req)
{
        http::client()->create_room(
          req, [this](const mtx::responses::CreateRoom &res, mtx::http::RequestErr err) {
                  if (err) {
                          const auto err_code   = mtx::errors::to_string(err->matrix_error.errcode);
                          const auto error      = err->matrix_error.error;
                          const int status_code = static_cast<int>(err->status_code);

                          nhlog::net()->warn(
                            "failed to create room: {} {} ({})", error, err_code, status_code);

                          emit showNotification(
                            tr("Room creation failed: %1").arg(QString::fromStdString(error)));
                          return;
                  }

                  QString newRoomId = QString::fromStdString(res.room_id.to_string());
                  emit showNotification(tr("Room %1 created.").arg(newRoomId));
                  emit newRoom(newRoomId);
          });
}

void
ChatPage::leaveRoom(const QString &room_id)
{
        http::client()->leave_room(
          room_id.toStdString(),
          [this, room_id](const mtx::responses::Empty &, mtx::http::RequestErr err) {
                  if (err) {
                          emit showNotification(
                            tr("Failed to leave room: %1")
                              .arg(QString::fromStdString(err->matrix_error.error)));
                          return;
                  }

                  emit leftRoom(room_id);
          });
}

void
ChatPage::changeRoom(const QString &room_id)
{
        view_manager_->rooms()->setCurrentRoom(room_id);
}

void
ChatPage::inviteUser(QString userid, QString reason)
{
        auto room = currentRoom();

        if (QMessageBox::question(this,
                                  tr("Confirm invite"),
                                  tr("Do you really want to invite %1 (%2)?")
                                    .arg(cache::displayName(room, userid))
                                    .arg(userid)) != QMessageBox::Yes)
                return;

        http::client()->invite_user(
          room.toStdString(),
          userid.toStdString(),
          [this, userid, room](const mtx::responses::Empty &, mtx::http::RequestErr err) {
                  if (err) {
                          emit showNotification(
                            tr("Failed to invite %1 to %2: %3")
                              .arg(userid)
                              .arg(room)
                              .arg(QString::fromStdString(err->matrix_error.error)));
                  } else
                          emit showNotification(tr("Invited user: %1").arg(userid));
          },
          reason.trimmed().toStdString());
}
void
ChatPage::kickUser(QString userid, QString reason)
{
        auto room = currentRoom();

        if (QMessageBox::question(this,
                                  tr("Confirm kick"),
                                  tr("Do you really want to kick %1 (%2)?")
                                    .arg(cache::displayName(room, userid))
                                    .arg(userid)) != QMessageBox::Yes)
                return;

        http::client()->kick_user(
          room.toStdString(),
          userid.toStdString(),
          [this, userid, room](const mtx::responses::Empty &, mtx::http::RequestErr err) {
                  if (err) {
                          emit showNotification(
                            tr("Failed to kick %1 from %2: %3")
                              .arg(userid)
                              .arg(room)
                              .arg(QString::fromStdString(err->matrix_error.error)));
                  } else
                          emit showNotification(tr("Kicked user: %1").arg(userid));
          },
          reason.trimmed().toStdString());
}
void
ChatPage::banUser(QString userid, QString reason)
{
        auto room = currentRoom();

        if (QMessageBox::question(this,
                                  tr("Confirm ban"),
                                  tr("Do you really want to ban %1 (%2)?")
                                    .arg(cache::displayName(room, userid))
                                    .arg(userid)) != QMessageBox::Yes)
                return;

        http::client()->ban_user(
          room.toStdString(),
          userid.toStdString(),
          [this, userid, room](const mtx::responses::Empty &, mtx::http::RequestErr err) {
                  if (err) {
                          emit showNotification(
                            tr("Failed to ban %1 in %2: %3")
                              .arg(userid)
                              .arg(room)
                              .arg(QString::fromStdString(err->matrix_error.error)));
                  } else
                          emit showNotification(tr("Banned user: %1").arg(userid));
          },
          reason.trimmed().toStdString());
}
void
ChatPage::unbanUser(QString userid, QString reason)
{
        auto room = currentRoom();

        if (QMessageBox::question(this,
                                  tr("Confirm unban"),
                                  tr("Do you really want to unban %1 (%2)?")
                                    .arg(cache::displayName(room, userid))
                                    .arg(userid)) != QMessageBox::Yes)
                return;

        http::client()->unban_user(
          room.toStdString(),
          userid.toStdString(),
          [this, userid, room](const mtx::responses::Empty &, mtx::http::RequestErr err) {
                  if (err) {
                          emit showNotification(
                            tr("Failed to unban %1 in %2: %3")
                              .arg(userid)
                              .arg(room)
                              .arg(QString::fromStdString(err->matrix_error.error)));
                  } else
                          emit showNotification(tr("Unbanned user: %1").arg(userid));
          },
          reason.trimmed().toStdString());
}

void
ChatPage::receivedSessionKey(const std::string &room_id, const std::string &session_id)
{
        view_manager_->receivedSessionKey(room_id, session_id);
}

QString
ChatPage::status() const
{
        return QString::fromStdString(cache::statusMessage(utils::localUser().toStdString()));
}

void
ChatPage::setStatus(const QString &status)
{
        http::client()->put_presence_status(
          currentPresence(), status.toStdString(), [](mtx::http::RequestErr err) {
                  if (err) {
                          nhlog::net()->warn("failed to set presence status_msg: {}",
                                             err->matrix_error.error);
                  }
          });
}

mtx::presence::PresenceState
ChatPage::currentPresence() const
{
        switch (userSettings_->presence()) {
        case UserSettings::Presence::Online:
                return mtx::presence::online;
        case UserSettings::Presence::Unavailable:
                return mtx::presence::unavailable;
        case UserSettings::Presence::Offline:
                return mtx::presence::offline;
        default:
                return mtx::presence::online;
        }
}

void
ChatPage::ensureOneTimeKeyCount(const std::map<std::string, uint16_t> &counts)
{
        for (const auto &entry : counts) {
                if (entry.second < MAX_ONETIME_KEYS) {
                        const int nkeys = MAX_ONETIME_KEYS - entry.second;

                        nhlog::crypto()->info("uploading {} {} keys", nkeys, entry.first);
                        olm::client()->generate_one_time_keys(nkeys);

                        http::client()->upload_keys(
                          olm::client()->create_upload_keys_request(),
                          [](const mtx::responses::UploadKeys &, mtx::http::RequestErr err) {
                                  if (err) {
                                          nhlog::crypto()->warn(
                                            "failed to update one-time keys: {} {}",
                                            err->matrix_error.error,
                                            static_cast<int>(err->status_code));
                                          return;
                                  }

                                  olm::mark_keys_as_published();
                          });
                }
        }
}

void
ChatPage::getProfileInfo()
{
        const auto userid = utils::localUser().toStdString();

        http::client()->get_profile(
          userid, [this](const mtx::responses::Profile &res, mtx::http::RequestErr err) {
                  if (err) {
                          nhlog::net()->warn("failed to retrieve own profile info");
                          return;
                  }

                  emit setUserDisplayName(QString::fromStdString(res.display_name));

                  emit setUserAvatar(QString::fromStdString(res.avatar_url));
          });
}

void
ChatPage::initiateLogout()
{
        http::client()->logout([this](const mtx::responses::Logout &, mtx::http::RequestErr err) {
                if (err) {
                        // TODO: handle special errors
                        emit contentLoaded();
                        nhlog::net()->warn("failed to logout: {} - {}",
                                           mtx::errors::to_string(err->matrix_error.errcode),
                                           err->matrix_error.error);
                        return;
                }

                emit loggedOut();
        });

        emit showOverlayProgressBar();
}

template<typename T>
void
ChatPage::connectCallMessage()
{
        connect(callManager_,
                qOverload<const QString &, const T &>(&CallManager::newMessage),
                view_manager_,
                qOverload<const QString &, const T &>(&TimelineViewManager::queueCallMessage));
}

void
ChatPage::decryptDownloadedSecrets(mtx::secret_storage::AesHmacSha2KeyDescription keyDesc,
                                   const SecretsToDecrypt &secrets)
{
        QString text = QInputDialog::getText(
          ChatPage::instance(),
          QCoreApplication::translate("CrossSigningSecrets", "Decrypt secrets"),
          keyDesc.name.empty()
            ? QCoreApplication::translate(
                "CrossSigningSecrets",
                "Enter your recovery key or passphrase to decrypt your secrets:")
            : QCoreApplication::translate(
                "CrossSigningSecrets",
                "Enter your recovery key or passphrase called %1 to decrypt your secrets:")
                .arg(QString::fromStdString(keyDesc.name)),
          QLineEdit::Password);

        if (text.isEmpty())
                return;

        auto decryptionKey = mtx::crypto::key_from_recoverykey(text.toStdString(), keyDesc);

        if (!decryptionKey)
                decryptionKey = mtx::crypto::key_from_passphrase(text.toStdString(), keyDesc);

        if (!decryptionKey) {
                QMessageBox::information(
                  ChatPage::instance(),
                  QCoreApplication::translate("CrossSigningSecrets", "Decryption failed"),
                  QCoreApplication::translate("CrossSigningSecrets",
                                              "Failed to decrypt secrets with the "
                                              "provided recovery key or passphrase"));
                return;
        }

        for (const auto &[secretName, encryptedSecret] : secrets) {
                auto decrypted = mtx::crypto::decrypt(encryptedSecret, *decryptionKey, secretName);
                if (!decrypted.empty())
                        cache::storeSecret(secretName, decrypted);
        }
}

void
ChatPage::startChat(QString userid)
{
        auto joined_rooms = cache::joinedRooms();
        auto room_infos   = cache::getRoomInfo(joined_rooms);

        for (std::string room_id : joined_rooms) {
                if (room_infos[QString::fromStdString(room_id)].member_count == 2) {
                        auto room_members = cache::roomMembers(room_id);
                        if (std::find(room_members.begin(),
                                      room_members.end(),
                                      (userid).toStdString()) != room_members.end()) {
                                view_manager_->rooms()->setCurrentRoom(
                                  QString::fromStdString(room_id));
                                return;
                        }
                }
        }

        if (QMessageBox::Yes !=
            QMessageBox::question(
              this,
              tr("Confirm invite"),
              tr("Do you really want to start a private chat with %1?").arg(userid)))
                return;

        mtx::requests::CreateRoom req;
        req.preset     = mtx::requests::Preset::PrivateChat;
        req.visibility = mtx::common::RoomVisibility::Private;
        if (utils::localUser() != userid) {
                req.invite    = {userid.toStdString()};
                req.is_direct = true;
        }
        emit ChatPage::instance()->createRoom(req);
}

static QString
mxidFromSegments(QStringRef sigil, QStringRef mxid)
{
        if (mxid.isEmpty())
                return "";

        auto mxid_ = QUrl::fromPercentEncoding(mxid.toUtf8());

        if (sigil == "u") {
                return "@" + mxid_;
        } else if (sigil == "roomid") {
                return "!" + mxid_;
        } else if (sigil == "r") {
                return "#" + mxid_;
                //} else if (sigil == "group") {
                //        return "+" + mxid_;
        } else {
                return "";
        }
}

void
ChatPage::handleMatrixUri(const QByteArray &uri)
{
        nhlog::ui()->info("Received uri! {}", uri.toStdString());
        QUrl uri_{QString::fromUtf8(uri)};

        if (uri_.scheme() != "matrix")
                return;

        auto tempPath = uri_.path(QUrl::ComponentFormattingOption::FullyEncoded);
        if (tempPath.startsWith('/'))
                tempPath.remove(0, 1);
        auto segments = tempPath.splitRef('/');

        if (segments.size() != 2 && segments.size() != 4)
                return;

        auto sigil1 = segments[0];
        auto mxid1  = mxidFromSegments(sigil1, segments[1]);
        if (mxid1.isEmpty())
                return;

        QString mxid2;
        if (segments.size() == 4 && segments[2] == "e") {
                if (segments[3].isEmpty())
                        return;
                else
                        mxid2 = "$" + QUrl::fromPercentEncoding(segments[3].toUtf8());
        }

        std::vector<std::string> vias;
        QString action;

        for (QString item : uri_.query(QUrl::ComponentFormattingOption::FullyEncoded).split('&')) {
                nhlog::ui()->info("item: {}", item.toStdString());

                if (item.startsWith("action=")) {
                        action = item.remove("action=");
                } else if (item.startsWith("via=")) {
                        vias.push_back(
                          QUrl::fromPercentEncoding(item.remove("via=").toUtf8()).toStdString());
                }
        }

        if (sigil1 == "u") {
                if (action.isEmpty()) {
                        if (auto t = view_manager_->rooms()->currentRoom())
                                t->openUserProfile(mxid1);
                } else if (action == "chat") {
                        this->startChat(mxid1);
                }
        } else if (sigil1 == "roomid") {
                auto joined_rooms = cache::joinedRooms();
                auto targetRoomId = mxid1.toStdString();

                for (auto roomid : joined_rooms) {
                        if (roomid == targetRoomId) {
                                view_manager_->rooms()->setCurrentRoom(mxid1);
                                if (!mxid2.isEmpty())
                                        view_manager_->showEvent(mxid1, mxid2);
                                return;
                        }
                }

                if (action == "join" || action.isEmpty()) {
                        joinRoomVia(targetRoomId, vias);
                }
        } else if (sigil1 == "r") {
                auto joined_rooms    = cache::joinedRooms();
                auto targetRoomAlias = mxid1.toStdString();

                for (auto roomid : joined_rooms) {
                        auto aliases = cache::client()->getRoomAliases(roomid);
                        if (aliases) {
                                if (aliases->alias == targetRoomAlias) {
                                        view_manager_->rooms()->setCurrentRoom(
                                          QString::fromStdString(roomid));
                                        if (!mxid2.isEmpty())
                                                view_manager_->showEvent(
                                                  QString::fromStdString(roomid), mxid2);
                                        return;
                                }
                        }
                }

                if (action == "join" || action.isEmpty()) {
                        joinRoomVia(mxid1.toStdString(), vias);
                }
        }
}

void
ChatPage::handleMatrixUri(const QUrl &uri)
{
        handleMatrixUri(uri.toString(QUrl::ComponentFormattingOption::FullyEncoded).toUtf8());
}

bool
ChatPage::isRoomActive(const QString &room_id)
{
        return isActiveWindow() && currentRoom() == room_id;
}

QString
ChatPage::currentRoom() const
{
        if (view_manager_->rooms()->currentRoom())
                return view_manager_->rooms()->currentRoom()->roomId();
        else
                return "";
}
