// SPDX-FileCopyrightText: 2017 Konstantinos Sideris <siderisk@auth.gr>
// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QApplication>
#include <QInputDialog>
#include <QMessageBox>

#include <mtx/responses.hpp>

#include "AvatarProvider.h"
#include "Cache.h"
#include "Cache_p.h"
#include "ChatPage.h"
#include "EventAccessors.h"
#include "Logging.h"
#include "MainWindow.h"
#include "MatrixClient.h"
#include "UserSettingsPage.h"
#include "Utils.h"
#include "encryption/DeviceVerificationFlow.h"
#include "encryption/Olm.h"
#include "ui/RoomSummary.h"
#include "ui/Theme.h"
#include "ui/UserProfile.h"
#include "voip/CallManager.h"

#include "notifications/Manager.h"

#include "timeline/TimelineViewManager.h"

#include "blurhash.hpp"

ChatPage *ChatPage::instance_             = nullptr;
constexpr int CHECK_CONNECTIVITY_INTERVAL = 15'000;
constexpr int RETRY_TIMEOUT               = 5'000;
constexpr size_t MAX_ONETIME_KEYS         = 50;

Q_DECLARE_METATYPE(std::optional<mtx::crypto::EncryptedFile>)
Q_DECLARE_METATYPE(std::optional<RelatedInfo>)
Q_DECLARE_METATYPE(mtx::presence::PresenceState)
Q_DECLARE_METATYPE(mtx::secret_storage::AesHmacSha2KeyDescription)
Q_DECLARE_METATYPE(SecretsToDecrypt)

ChatPage::ChatPage(QSharedPointer<UserSettings> userSettings, QObject *parent)
  : QObject(parent)
  , isConnected_(true)
  , userSettings_{userSettings}
  , notificationsManager(new NotificationsManager(this))
  , callManager_(new CallManager(this))
{
    setObjectName(QStringLiteral("chatPage"));

    instance_ = this;

    qRegisterMetaType<std::optional<mtx::crypto::EncryptedFile>>();
    qRegisterMetaType<std::optional<RelatedInfo>>();
    qRegisterMetaType<mtx::presence::PresenceState>();
    qRegisterMetaType<mtx::secret_storage::AesHmacSha2KeyDescription>();
    qRegisterMetaType<SecretsToDecrypt>();

    view_manager_ = new TimelineViewManager(callManager_, this);

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
    connect(&connectivityTimer_, &QTimer::timeout, this, [this]() {
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

    connect(
      view_manager_,
      &TimelineViewManager::inviteUsers,
      this,
      [this](QString roomId, QStringList users) {
          for (int ii = 0; ii < users.size(); ++ii) {
              QTimer::singleShot(ii * 500, this, [this, roomId, ii, users]() {
                  const auto user = users.at(ii);

                  http::client()->invite_user(
                    roomId.toStdString(),
                    user.toStdString(),
                    [this, user](const mtx::responses::RoomInvite &, mtx::http::RequestErr err) {
                        if (err) {
                            emit showNotification(tr("Failed to invite user: %1").arg(user));
                            return;
                        }

                        emit showNotification(tr("Invited user: %1").arg(user));
                    });
              });
          }
      });

    connect(this,
            &ChatPage::internalKnock,
            this,
            qOverload<const QString &, const std::vector<std::string> &, QString, bool, bool>(
              &ChatPage::knockRoom),
            Qt::QueuedConnection);
    connect(this, &ChatPage::leftRoom, this, &ChatPage::removeRoom);
    connect(this, &ChatPage::changeToRoom, this, &ChatPage::changeRoom, Qt::QueuedConnection);
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

    connect(notificationsManager,
            &NotificationsManager::notificationClicked,
            this,
            [this](const QString &roomid, const QString &eventid) {
                Q_UNUSED(eventid)
                auto exWin = MainWindow::instance()->windowForRoom(roomid);
                if (exWin) {
                    exWin->requestActivate();
                } else {
                    view_manager_->rooms()->setCurrentRoom(roomid);
                    MainWindow::instance()->requestActivate();
                }
            });
    connect(notificationsManager,
            &NotificationsManager::sendNotificationReply,
            this,
            [this](const QString &roomid, const QString &eventid, const QString &body) {
                view_manager_->queueReply(roomid, eventid, body);
                auto exWin = MainWindow::instance()->windowForRoom(roomid);
                if (exWin) {
                    exWin->requestActivate();
                } else {
                    view_manager_->rooms()->setCurrentRoom(roomid);
                    MainWindow::instance()->requestActivate();
                }
            });

    connect(
      this,
      &ChatPage::initializeViews,
      view_manager_,
      [this](const mtx::responses::Sync &sync) { view_manager_->sync(sync); },
      Qt::QueuedConnection);
    connect(this,
            &ChatPage::initializeEmptyViews,
            view_manager_,
            &TimelineViewManager::initializeRoomlist);
    connect(this, &ChatPage::syncUI, this, [this](const mtx::responses::Sync &sync) {
        view_manager_->sync(sync);

        static unsigned int prevNotificationCount = 0;
        unsigned int notificationCount            = 0;
        for (const auto &room : sync.rooms.join) {
            notificationCount += room.second.unread_notifications.notification_count;
        }

        // HACK: If we had less notifications last time we checked, send an alert if the
        // user wanted one. Technically, this may cause an alert to be missed if new ones
        // come in while you are reading old ones. Since the window is almost certainly open
        // in this edge case, that's probably a non-issue.
        // TODO: Replace this once we have proper pushrules support. This is a horrible hack
        if (prevNotificationCount < notificationCount) {
            if (userSettings_->hasAlertOnNotification())
                MainWindow::instance()->alert(0);
        }
        prevNotificationCount = notificationCount;

        // No need to check amounts for this section, as this function internally checks for
        // duplicates.
        if (notificationCount && userSettings_->hasNotifications())
            http::client()->notifications(
              5,
              "",
              "",
              [this](const mtx::responses::Notifications &res, mtx::http::RequestErr err) {
                  if (err) {
                      nhlog::net()->warn("failed to retrieve notifications: {}", err);
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

    connect(
      this, &ChatPage::newSyncResponse, this, &ChatPage::handleSyncResponse, Qt::QueuedConnection);

    connect(this, &ChatPage::dropToLoginPageCb, this, &ChatPage::dropToLoginPage);

    connect(
      this,
      &ChatPage::startRemoveFallbackKeyTimer,
      this,
      [this]() {
          QTimer::singleShot(std::chrono::minutes(5), this, &ChatPage::removeOldFallbackKey);
          disconnect(
            this, &ChatPage::newSyncResponse, this, &ChatPage::startRemoveFallbackKeyTimer);
      },
      Qt::QueuedConnection);

    connectCallMessage<mtx::events::voip::CallInvite>();
    connectCallMessage<mtx::events::voip::CallCandidates>();
    connectCallMessage<mtx::events::voip::CallAnswer>();
    connectCallMessage<mtx::events::voip::CallHangUp>();
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
    auto settings = UserSettings::instance()->qsettings();

    if (UserSettings::instance()->profile() != QLatin1String("")) {
        settings->beginGroup(QStringLiteral("profile"));
        settings->beginGroup(UserSettings::instance()->profile());
    }
    settings->beginGroup(QStringLiteral("auth"));
    settings->remove(QLatin1String(""));
    settings->endGroup(); // auth
    if (UserSettings::instance()->profile() != QLatin1String("")) {
        settings->endGroup(); // profilename
        settings->endGroup(); // profile
    }

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
        nhlog::ui()->critical("bootstrapped with invalid user_id: {}", userid.toStdString());
    }

    http::client()->set_server(homeserver.toStdString());
    http::client()->set_access_token(token.toStdString());
    http::client()->verify_certificates(!UserSettings::instance()->disableCertificateValidation());

    // The Olm client needs the user_id & device_id that will be included
    // in the generated payloads & keys.
    olm::client()->set_user_id(http::client()->user_id().to_string());
    olm::client()->set_device_id(http::client()->device_id());

    try {
        cache::init(userid);

        connect(cache::client(), &Cache::databaseReady, this, [this]() {
            nhlog::db()->info("database ready");

            const bool isInitialized = cache::isInitialized();
            const auto cacheVersion  = cache::formatVersion();

            try {
                if (!isInitialized) {
                    cache::setCurrentFormat();
                } else {
                    if (cacheVersion == cache::CacheVersion::Current) {
                        loadStateFromCache();
                        return;
                    } else if (cacheVersion == cache::CacheVersion::Older) {
                        if (!cache::runMigrations()) {
                            QMessageBox::critical(
                              nullptr,
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
                          nullptr,
                          tr("Incompatible cache version"),
                          tr("The cache on your disk is newer than this version of Nheko "
                             "supports. Please update Nheko or clear your cache."));
                        QCoreApplication::quit();
                        return;
                    }
                }

                // It's the first time syncing with this device
                // There isn't a saved olm account to restore.
                nhlog::crypto()->info("creating new olm account");
                olm::client()->create_new_account();
                cache::saveOlmAccount(olm::client()->save(cache::client()->pickleSecret()));
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
            getBackupVersion();
            tryInitialSync();
            callManager_->refreshTurnServer();
            emit MainWindow::instance()->reload();
        });

        connect(cache::client(),
                &Cache::newReadReceipts,
                view_manager_,
                &TimelineViewManager::updateReadReceipts);

        connect(cache::client(),
                &Cache::removeNotification,
                notificationsManager,
                &NotificationsManager::removeNotification);

    } catch (const lmdb::error &e) {
        nhlog::db()->critical("failure during boot: {}", e.what());
        emit dropToLoginPageCb(tr("Failed to open database, logging out!"));
    }
}

void
ChatPage::loadStateFromCache()
{
    nhlog::db()->info("restoring state from cache");

    try {
        olm::client()->load(cache::restoreOlmAccount(), cache::client()->pickleSecret());

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
    } catch (const nlohmann::json::exception &e) {
        nhlog::db()->critical("failed to parse cache data: {}", e.what());
        emit dropToLoginPageCb(tr("Failed to restore save data. Please login again."));
        return;
    } catch (const std::exception &e) {
        nhlog::db()->critical("failed to load cache data: {}", e.what());
        emit dropToLoginPageCb(tr("Failed to restore save data. Please login again."));
        return;
    }

    nhlog::crypto()->info("ed25519   : {}", olm::client()->identity_keys().ed25519);
    nhlog::crypto()->info("curve25519: {}", olm::client()->identity_keys().curve25519);

    getProfileInfo();
    getBackupVersion();
    verifyOneTimeKeyCountAfterStartup();
    callManager_->refreshTurnServer();

    emit contentLoaded();

    // Start receiving events.
    connect(this, &ChatPage::newSyncResponse, &ChatPage::startRemoveFallbackKeyTimer);
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

                // We should only send one notification per event.
                cache::markSentNotification(event_id);

                // Don't send a notification when the current room is opened.
                if (isRoomActive(room_id))
                    continue;

                if (userSettings_->hasDesktopNotifications()) {
                    auto info = cache::singleRoomInfo(item.room_id);

                    AvatarProvider::resolve(QString::fromStdString(info.avatar_url),
                                            96,
                                            this,
                                            [this, item](QPixmap image) {
                                                notificationsManager->postNotification(
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
    olm::client()->generate_one_time_keys(MAX_ONETIME_KEYS, true);

    http::client()->upload_keys(
      olm::client()->create_upload_keys_request(),
      [this](const mtx::responses::UploadKeys &res, mtx::http::RequestErr err) {
          if (err) {
              const int status_code = static_cast<int>(err->status_code);

              if (status_code == 404) {
                  nhlog::net()->warn("skipping key uploading. server doesn't provide /keys/upload");
                  return startInitialSync();
              }

              nhlog::crypto()->critical("failed to upload one time keys: {}", err);

              QString errorMsg(tr("Failed to setup encryption keys. Server response: "
                                  "%1 %2. Please try again later.")
                                 .arg(QString::fromStdString(err->matrix_error.error))
                                 .arg(status_code));

              emit dropToLoginPageCb(errorMsg);
              return;
          }

          olm::client()->forget_old_fallback_key();
          olm::mark_keys_as_published();

          for (const auto &entry : res.one_time_key_counts)
              nhlog::net()->info("uploaded {} {} one-time keys", entry.second, entry.first);

          cache::client()->markUserKeysOutOfDate({http::client()->user_id().to_string()});

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

    http::client()->sync(opts, [this](const mtx::responses::Sync &res, mtx::http::RequestErr err) {
        // TODO: Initial Sync should include mentions as well...

        if (err) {
            const auto error      = QString::fromStdString(err->matrix_error.error);
            const auto msg        = tr("Please try to login again: %1").arg(error);
            const auto err_code   = mtx::errors::to_string(err->matrix_error.errcode);
            const int status_code = static_cast<int>(err->status_code);

            nhlog::net()->error("initial sync error: {}", err);

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

        QTimer::singleShot(0, this, [this, res] {
            nhlog::net()->info("initial sync completed");
            try {
                cache::client()->saveState(res);

                olm::handle_to_device_messages(res.to_device.events);

                emit initializeViews(std::move(res));
                emit initializeMentions(cache::getTimelineMentions());

                cache::calculateRoomReadStatus();
            } catch (const lmdb::error &e) {
                nhlog::db()->error("failed to save state after initial sync: {}", e.what());
                startInitialSync();
                return;
            }

            emit trySyncCb();
            emit contentLoaded();
        });
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
        return;
    }

    nhlog::net()->debug("sync completed: {}", res.next_batch);

    // Ensure that we have enough one-time keys available.
    ensureOneTimeKeyCount(res.device_one_time_keys_count, res.device_unused_fallback_key_types);

    // TODO: fine grained error handling
    try {
        cache::client()->saveState(res);
        olm::handle_to_device_messages(res.to_device.events);

        auto updates = cache::getRoomInfo(cache::client()->roomsWithStateUpdates(res));

        emit syncUI(std::move(res));

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
      opts, [this, since = opts.since](const mtx::responses::Sync &res, mtx::http::RequestErr err) {
          if (err) {
              const auto error = QString::fromStdString(err->matrix_error.error);
              const auto msg   = tr("Please try to login again: %1").arg(error);

              if ((http::is_logged_in() &&
                   (err->matrix_error.errcode == mtx::errors::ErrorCode::M_UNKNOWN_TOKEN ||
                    err->matrix_error.errcode == mtx::errors::ErrorCode::M_MISSING_TOKEN)) ||
                  !http::is_logged_in()) {
                  emit dropToLoginPageCb(msg);
                  return;
              }

              nhlog::net()->error("sync error: {}", *err);
              emit tryDelayedSyncCb();
              return;
          }

          emit newSyncResponse(res, since);
      });
}

void
ChatPage::knockRoom(const QString &room,
                    const std::vector<std::string> &via,
                    QString reason,
                    bool failedJoin,
                    bool promptForConfirmation)
{
    const auto room_id = room.toStdString();
    bool confirmed     = false;
    if (promptForConfirmation) {
        reason = QInputDialog::getText(
          nullptr,
          tr("Knock on room"),
          // clang-format off
      failedJoin
        ? tr("You failed to join %1. You can try to knock, so that others can invite you in. Do you want to do so?\nYou may optionally provide a reason for others to accept your knock:").arg(room)
        : tr("Do you really want to knock on %1? You may optionally provide a reason for others to accept your knock:").arg(room),
          // clang-format on
          QLineEdit::Normal,
          reason,
          &confirmed);
        if (!confirmed) {
            return;
        }
    }

    http::client()->knock_room(
      room_id,
      via,
      [this, room_id](const mtx::responses::RoomId &, mtx::http::RequestErr err) {
          if (err) {
              emit showNotification(tr("Failed to knock room: %1")
                                      .arg(QString::fromStdString(err->matrix_error.error)));
              return;
          }
      },
      reason.toStdString());
}

void
ChatPage::joinRoom(const QString &room, const QString &reason)
{
    const auto room_id = room.toStdString();
    joinRoomVia(room_id, {}, false, reason);
}

void
ChatPage::joinRoomVia(const std::string &room_id,
                      const std::vector<std::string> &via,
                      bool promptForConfirmation,
                      const QString &reason)
{
    if (promptForConfirmation) {
        auto prompt = new RoomSummary(room_id, via, reason);
        QQmlEngine::setObjectOwnership(prompt, QQmlEngine::JavaScriptOwnership);
        emit showRoomJoinPrompt(prompt);
        return;
    }

    http::client()->join_room(
      room_id,
      via,
      [this, room_id, reason, via](const mtx::responses::RoomId &, mtx::http::RequestErr err) {
          if (err) {
              if (err->matrix_error.errcode == mtx::errors::ErrorCode::M_FORBIDDEN)
                  emit internalKnock(QString::fromStdString(room_id), via, reason, true, true);
              else
                  emit showNotification(tr("Failed to join room: %1")
                                          .arg(QString::fromStdString(err->matrix_error.error)));
              return;
          }

          // We remove any invites with the same room_id.
          try {
              cache::removeInvite(room_id);
          } catch (const lmdb::error &e) {
              emit showNotification(tr("Failed to remove invite: %1").arg(e.what()));
          }

          view_manager_->rooms()->setCurrentRoom(QString::fromStdString(room_id));
      },
      reason.toStdString());
}

void
ChatPage::createRoom(const mtx::requests::CreateRoom &req)
{
    if (req.room_alias_name.find(":") != std::string::npos ||
        req.room_alias_name.find("#") != std::string::npos) {
        nhlog::net()->warn("Failed to create room: Some characters are not allowed in alias");
        emit this->showNotification(tr("Room creation failed: Bad Alias"));
        return;
    }

    http::client()->create_room(
      req, [this](const mtx::responses::CreateRoom &res, mtx::http::RequestErr err) {
          if (err) {
              const auto err_code = mtx::errors::to_string(err->matrix_error.errcode);
              const auto error    = err->matrix_error.error;

              nhlog::net()->warn("failed to create room: {})", err);

              emit showNotification(
                tr("Room creation failed: %1").arg(QString::fromStdString(error)));
              return;
          }

          QString newRoomId = QString::fromStdString(res.room_id.to_string());
          emit showNotification(tr("Room %1 created.").arg(newRoomId));
          emit newRoom(newRoomId);
          emit changeToRoom(newRoomId);
      });
}

void
ChatPage::leaveRoom(const QString &room_id, const QString &reason)
{
    http::client()->leave_room(
      room_id.toStdString(),
      [this, room_id](const mtx::responses::Empty &, mtx::http::RequestErr err) {
          if (err) {
              emit showNotification(tr("Failed to leave room: %1")
                                      .arg(QString::fromStdString(err->matrix_error.error)));
              nhlog::net()->error("Failed to leave room '{}': {}", room_id.toStdString(), err);

              if (err->status_code == 404 &&
                  err->matrix_error.errcode == mtx::errors::ErrorCode::M_UNKNOWN) {
                  nhlog::db()->debug(
                    "Removing invite and room for {}, even though we couldn't leave.",
                    room_id.toStdString());
                  cache::client()->removeInvite(room_id.toStdString());
                  cache::client()->removeRoom(room_id.toStdString());
              }
              return;
          }

          emit leftRoom(room_id);
      },
      reason.toStdString());
}

void
ChatPage::changeRoom(const QString &room_id)
{
    view_manager_->rooms()->setCurrentRoom(room_id);
}

void
ChatPage::inviteUser(const QString &room, QString userid, QString reason)
{
    if (QMessageBox::question(nullptr,
                              tr("Confirm invite"),
                              tr("Do you really want to invite %1 (%2)?")
                                .arg(cache::displayName(room, userid), userid)) != QMessageBox::Yes)
        return;

    http::client()->invite_user(
      room.toStdString(),
      userid.toStdString(),
      [this, userid, room](const mtx::responses::Empty &, mtx::http::RequestErr err) {
          if (err) {
              nhlog::net()->error(
                "Failed to invite {} to {}: {}", userid.toStdString(), room.toStdString(), *err);
              emit showNotification(
                tr("Failed to invite %1 to %2: %3")
                  .arg(userid, room, QString::fromStdString(err->matrix_error.error)));
          } else
              emit showNotification(tr("Invited user: %1").arg(userid));
      },
      reason.trimmed().toStdString());
}
void
ChatPage::kickUser(const QString &room, QString userid, QString reason)
{
    bool confirmed;
    reason =
      QInputDialog::getText(nullptr,
                            tr("Reason for the kick"),
                            tr("Enter reason for kicking %1 (%2) or hit enter for no reason:")
                              .arg(cache::displayName(room, userid), userid),
                            QLineEdit::Normal,
                            reason,
                            &confirmed);
    if (!confirmed) {
        return;
    }

    http::client()->kick_user(
      room.toStdString(),
      userid.toStdString(),
      [this, userid, room](const mtx::responses::Empty &, mtx::http::RequestErr err) {
          if (err) {
              emit showNotification(
                tr("Failed to kick %1 from %2: %3")
                  .arg(userid, room, QString::fromStdString(err->matrix_error.error)));
          } else
              emit showNotification(tr("Kicked user: %1").arg(userid));
      },
      reason.trimmed().toStdString());
}
void
ChatPage::banUser(const QString &room, QString userid, QString reason)
{
    bool confirmed;
    reason =
      QInputDialog::getText(nullptr,
                            tr("Reason for the ban"),
                            tr("Enter reason for banning %1 (%2) or hit enter for no reason:")
                              .arg(cache::displayName(room, userid), userid),
                            QLineEdit::Normal,
                            reason,
                            &confirmed);
    if (!confirmed) {
        return;
    }

    http::client()->ban_user(
      room.toStdString(),
      userid.toStdString(),
      [this, userid, room](const mtx::responses::Empty &, mtx::http::RequestErr err) {
          if (err) {
              emit showNotification(
                tr("Failed to ban %1 in %2: %3")
                  .arg(userid, room, QString::fromStdString(err->matrix_error.error)));
          } else
              emit showNotification(tr("Banned user: %1").arg(userid));
      },
      reason.trimmed().toStdString());
}
void
ChatPage::unbanUser(const QString &room, QString userid, QString reason)
{
    if (QMessageBox::question(nullptr,
                              tr("Confirm unban"),
                              tr("Do you really want to unban %1 (%2)?")
                                .arg(cache::displayName(room, userid), userid)) != QMessageBox::Yes)
        return;

    http::client()->unban_user(
      room.toStdString(),
      userid.toStdString(),
      [this, userid, room](const mtx::responses::Empty &, mtx::http::RequestErr err) {
          if (err) {
              emit showNotification(
                tr("Failed to unban %1 in %2: %3")
                  .arg(userid, room, QString::fromStdString(err->matrix_error.error)));
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
    return QString::fromStdString(cache::presence(utils::localUser().toStdString()).status_msg);
}

void
ChatPage::setStatus(const QString &status)
{
    http::client()->put_presence_status(
      currentPresence(), status.toStdString(), [](mtx::http::RequestErr err) {
          if (err) {
              nhlog::net()->warn("failed to set presence status_msg: {}", err->matrix_error.error);
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
ChatPage::verifyOneTimeKeyCountAfterStartup()
{
    http::client()->upload_keys(
      olm::client()->create_upload_keys_request(),
      [this](const mtx::responses::UploadKeys &res, mtx::http::RequestErr err) {
          if (err) {
              nhlog::crypto()->warn("failed to update one-time keys: {}", err);

              if (err->status_code < 400 || err->status_code >= 500)
                  return;
          }

          std::map<std::string, uint16_t> key_counts;
          auto count = 0;
          if (auto c = res.one_time_key_counts.find(mtx::crypto::SIGNED_CURVE25519);
              c == res.one_time_key_counts.end()) {
              key_counts[mtx::crypto::SIGNED_CURVE25519] = 0;
          } else {
              key_counts[mtx::crypto::SIGNED_CURVE25519] = c->second;
              count                                      = c->second;
          }

          nhlog::crypto()->info(
            "Fetched server key count {} {}", count, mtx::crypto::SIGNED_CURVE25519);

          ensureOneTimeKeyCount(key_counts, std::nullopt);
      });
}

void
ChatPage::ensureOneTimeKeyCount(const std::map<std::string, uint16_t> &counts,
                                const std::optional<std::vector<std::string>> &unused_fallback_keys)
{
    if (auto count = counts.find(mtx::crypto::SIGNED_CURVE25519); count != counts.end()) {
        bool replace_fallback_key = false;
        if (unused_fallback_keys &&
            std::find(unused_fallback_keys->begin(),
                      unused_fallback_keys->end(),
                      mtx::crypto::SIGNED_CURVE25519) == unused_fallback_keys->end())
            replace_fallback_key = true;
        nhlog::crypto()->debug(
          "Updated server key count {} {}, fallback keys supported: {}, new fallback key: {}",
          count->second,
          mtx::crypto::SIGNED_CURVE25519,
          unused_fallback_keys.has_value(),
          replace_fallback_key);

        if (count->second < MAX_ONETIME_KEYS || replace_fallback_key) {
            const size_t nkeys =
              count->second < MAX_ONETIME_KEYS ? (MAX_ONETIME_KEYS - count->second) : 0;

            nhlog::crypto()->info("uploading {} {} keys", nkeys, mtx::crypto::SIGNED_CURVE25519);
            olm::client()->generate_one_time_keys(nkeys, replace_fallback_key);

            http::client()->upload_keys(
              olm::client()->create_upload_keys_request(),
              [replace_fallback_key, this](const mtx::responses::UploadKeys &,
                                           mtx::http::RequestErr err) {
                  if (err) {
                      nhlog::crypto()->warn("failed to update one-time keys: {}", err);

                      if (err->status_code < 400 || err->status_code >= 500)
                          return;
                  }

                  // mark as published anyway, otherwise we may end up in a loop.
                  olm::mark_keys_as_published();

                  if (replace_fallback_key) {
                      emit startRemoveFallbackKeyTimer();
                  }
              });
        } else if (count->second > 2 * MAX_ONETIME_KEYS) {
            nhlog::crypto()->warn("too many one-time keys, deleting 1");
            mtx::requests::ClaimKeys req;
            req.one_time_keys[http::client()->user_id().to_string()][http::client()->device_id()] =
              std::string(mtx::crypto::SIGNED_CURVE25519);
            http::client()->claim_keys(
              req, [](const mtx::responses::ClaimKeys &, mtx::http::RequestErr err) {
                  if (err)
                      nhlog::crypto()->warn("failed to clear 1 one-time key: {}", err);
                  else
                      nhlog::crypto()->info("cleared 1 one-time key");
              });
        }
    }
}

void
ChatPage::removeOldFallbackKey()
{
    olm::client()->forget_old_fallback_key();
    olm::mark_keys_as_published();
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
ChatPage::getBackupVersion()
{
    if (!UserSettings::instance()->useOnlineKeyBackup()) {
        nhlog::crypto()->info("Online key backup disabled.");
        return;
    }

    http::client()->backup_version(
      [this](const mtx::responses::backup::BackupVersion &res, mtx::http::RequestErr err) {
          if (err) {
              nhlog::net()->warn("Failed to retrieve backup version");
              if (err->status_code == 404)
                  cache::client()->deleteBackupVersion();
              return;
          }

          // switch to UI thread for secrets stuff
          QTimer::singleShot(0, this, [res] {
              auto auth_data = nlohmann::json::parse(res.auth_data);

              if (res.algorithm == "m.megolm_backup.v1.curve25519-aes-sha2") {
                  auto key = cache::secret(mtx::secret_storage::secrets::megolm_backup_v1);
                  if (!key) {
                      nhlog::crypto()->info("No key for online key backup.");
                      cache::client()->deleteBackupVersion();
                      return;
                  }

                  using namespace mtx::crypto;
                  auto pubkey = CURVE25519_public_key_from_private(to_binary_buf(base642bin(*key)));

                  if (auth_data["public_key"].get<std::string>() != pubkey) {
                      nhlog::crypto()->info("Our backup key {} does not match the one "
                                            "used in the online backup {}",
                                            pubkey,
                                            auth_data["public_key"]);
                      cache::client()->deleteBackupVersion();
                      return;
                  }

                  nhlog::crypto()->info("Using online key backup.");
                  OnlineBackupVersion data{};
                  data.algorithm = res.algorithm;
                  data.version   = res.version;
                  cache::client()->saveBackupVersion(data);
              } else {
                  nhlog::crypto()->info("Unsupported key backup algorithm: {}", res.algorithm);
                  cache::client()->deleteBackupVersion();
              }
          });
      });
}

void
ChatPage::initiateLogout()
{
    http::client()->logout([this](const mtx::responses::Logout &, mtx::http::RequestErr err) {
        if (err) {
            // TODO: handle special errors
            emit contentLoaded();
            nhlog::net()->warn("failed to logout: {}", err);
            return;
        }

        emit loggedOut();
    });
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
      nullptr,
      QCoreApplication::translate("CrossSigningSecrets", "Decrypt secrets"),
      keyDesc.name.empty()
        ? QCoreApplication::translate(
            "CrossSigningSecrets", "Enter your recovery key or passphrase to decrypt your secrets:")
        : QCoreApplication::translate(
            "CrossSigningSecrets",
            "Enter your recovery key or passphrase called %1 to decrypt your secrets:")
            .arg(QString::fromStdString(keyDesc.name)),
      QLineEdit::Password);

    if (text.isEmpty())
        return;

    // strip space chars from a recovery key. It can't contain those, but some clients insert them
    // to make them easier to read.
    QString stripped = text;
    stripped.remove(' ');
    stripped.remove('\n');
    stripped.remove('\t');

    auto decryptionKey = mtx::crypto::key_from_recoverykey(stripped.toStdString(), keyDesc);
    if (!decryptionKey && keyDesc.passphrase) {
        try {
            decryptionKey = mtx::crypto::key_from_passphrase(text.toStdString(), keyDesc);
        } catch (std::exception &e) {
            nhlog::crypto()->error("Failed to derive secret key from passphrase: {}", e.what());
        }
    }

    if (!decryptionKey) {
        QMessageBox::information(
          nullptr,
          QCoreApplication::translate("CrossSigningSecrets", "Decryption failed"),
          QCoreApplication::translate("CrossSigningSecrets",
                                      "Failed to decrypt secrets with the "
                                      "provided recovery key or passphrase"));
        return;
    }

    auto deviceKeys = cache::client()->userKeys(http::client()->user_id().to_string());
    mtx::requests::KeySignaturesUpload req;

    for (const auto &[secretName, encryptedSecret] : secrets) {
        auto decrypted = mtx::crypto::decrypt(encryptedSecret, *decryptionKey, secretName);
        nhlog::crypto()->debug("Secret {} decrypted: {}", secretName, !decrypted.empty());

        if (!decrypted.empty()) {
            cache::storeSecret(secretName, decrypted);

            if (deviceKeys && deviceKeys->device_keys.count(http::client()->device_id()) &&
                secretName == mtx::secret_storage::secrets::cross_signing_self_signing) {
                auto myKey = deviceKeys->device_keys.at(http::client()->device_id());
                if (myKey.user_id == http::client()->user_id().to_string() &&
                    myKey.device_id == http::client()->device_id() &&
                    myKey.keys["ed25519:" + http::client()->device_id()] ==
                      olm::client()->identity_keys().ed25519 &&
                    myKey.keys["curve25519:" + http::client()->device_id()] ==
                      olm::client()->identity_keys().curve25519) {
                    nlohmann::json j = myKey;
                    j.erase("signatures");
                    j.erase("unsigned");

                    auto ssk = mtx::crypto::PkSigning::from_seed(decrypted);
                    myKey.signatures[http::client()->user_id().to_string()]
                                    ["ed25519:" + ssk.public_key()] = ssk.sign(j.dump());
                    req.signatures[http::client()->user_id().to_string()]
                                  [http::client()->device_id()] = myKey;
                }
            } else if (deviceKeys &&
                       secretName == mtx::secret_storage::secrets::cross_signing_master) {
                auto mk = mtx::crypto::PkSigning::from_seed(decrypted);

                if (deviceKeys->master_keys.user_id == http::client()->user_id().to_string() &&
                    deviceKeys->master_keys.keys["ed25519:" + mk.public_key()] == mk.public_key()) {
                    nlohmann::json j = deviceKeys->master_keys;
                    j.erase("signatures");
                    j.erase("unsigned");
                    mtx::crypto::CrossSigningKeys master_key =
                      j.get<mtx::crypto::CrossSigningKeys>();
                    master_key.signatures[http::client()->user_id().to_string()]
                                         ["ed25519:" + http::client()->device_id()] =
                      olm::client()->sign_message(j.dump());
                    req.signatures[http::client()->user_id().to_string()][mk.public_key()] =
                      master_key;
                }
            }
        }
    }

    if (!req.signatures.empty()) {
        nhlog::crypto()->debug("Uploading new signatures: {}", nlohmann::json(req).dump(2));
        http::client()->keys_signatures_upload(
          req, [](const mtx::responses::KeySignaturesUpload &res, mtx::http::RequestErr err) {
              if (err) {
                  nhlog::net()->error("failed to upload signatures: {},{}",
                                      mtx::errors::to_string(err->matrix_error.errcode),
                                      static_cast<int>(err->status_code));
              }

              for (const auto &[user_id, tmp] : res.errors)
                  for (const auto &[key_id, e] : tmp)
                      nhlog::net()->error("signature error for user '{}' and key "
                                          "id {}: {} {}",
                                          user_id,
                                          key_id,
                                          mtx::errors::to_string(e.errcode),
                                          e.error);
          });
    }
}

void
ChatPage::startChat(QString userid, std::optional<bool> encryptionEnabled)
{
    auto joined_rooms = cache::joinedRooms();
    auto room_infos   = cache::getRoomInfo(joined_rooms);

    for (const std::string &room_id : joined_rooms) {
        if (const auto &info = room_infos[QString::fromStdString(room_id)];
            info.member_count == 2 && !info.is_space) {
            auto room_members = cache::roomMembers(room_id);
            if (std::find(room_members.begin(), room_members.end(), (userid).toStdString()) !=
                room_members.end()) {
                view_manager_->rooms()->setCurrentRoom(QString::fromStdString(room_id));
                return;
            }
        }
    }

    if (QMessageBox::Yes !=
        QMessageBox::question(
          nullptr,
          tr("Confirm invite"),
          tr("Do you really want to start a private chat with %1?").arg(userid)))
        return;

    mtx::requests::CreateRoom req;
    req.preset     = mtx::requests::Preset::TrustedPrivateChat;
    req.visibility = mtx::common::RoomVisibility::Private;

    if (!encryptionEnabled.has_value()) {
        if (auto keys = cache::client()->userKeys(userid.toStdString()))
            encryptionEnabled = !keys->device_keys.empty();
    }

    if (encryptionEnabled.value_or(false)) {
        mtx::events::StrippedEvent<mtx::events::state::Encryption> enc;
        enc.type              = mtx::events::EventType::RoomEncryption;
        enc.content.algorithm = mtx::crypto::MEGOLM_ALGO;
        req.initial_state.emplace_back(std::move(enc));
    }

    if (utils::localUser() != userid) {
        req.invite    = {userid.toStdString()};
        req.is_direct = true;
    }
    emit ChatPage::instance()->createRoom(req);
}

static QString
mxidFromSegments(QStringView sigil, QStringView mxid)
{
    if (mxid.isEmpty())
        return QString();

    auto mxid_ = QUrl::fromPercentEncoding(mxid.toUtf8());

    if (sigil == u"u") {
        return "@" + mxid_;
    } else if (sigil == u"roomid") {
        return "!" + mxid_;
    } else if (sigil == u"r") {
        return "#" + mxid_;
        //} else if (sigil == "group") {
        //        return "+" + mxid_;
    } else {
        return QString();
    }
}

bool
ChatPage::handleMatrixUri(QString uri)
{
    nhlog::ui()->info("Received uri! {}", uri.toStdString());
    QUrl uri_{uri};

    // Convert matrix.to URIs to proper format
    if (uri_.scheme() == QLatin1String("https") && uri_.host() == QLatin1String("matrix.to")) {
        QString p = uri_.fragment(QUrl::FullyEncoded);
        if (p.startsWith(QLatin1String("/")))
            p.remove(0, 1);

        auto temp = p.split(QStringLiteral("?"));
        QString query;
        if (temp.size() >= 2)
            query = QUrl::fromPercentEncoding(temp.takeAt(1).toUtf8());

        temp            = temp.first().split(QStringLiteral("/"));
        auto identifier = QUrl::fromPercentEncoding(temp.takeFirst().toUtf8());
        QString eventId = QUrl::fromPercentEncoding(temp.join('/').toUtf8());
        if (!identifier.isEmpty()) {
            if (identifier.startsWith(QLatin1String("@"))) {
                QByteArray newUri = "matrix:u/" + QUrl::toPercentEncoding(identifier.remove(0, 1));
                if (!query.isEmpty())
                    newUri.append("?" + query.toUtf8());
                return handleMatrixUri(QUrl::fromEncoded(newUri));
            } else if (identifier.startsWith(QLatin1String("#"))) {
                QByteArray newUri = "matrix:r/" + QUrl::toPercentEncoding(identifier.remove(0, 1));
                if (!eventId.isEmpty())
                    newUri.append("/e/" + QUrl::toPercentEncoding(eventId.remove(0, 1)));
                if (!query.isEmpty())
                    newUri.append("?" + query.toUtf8());
                return handleMatrixUri(QUrl::fromEncoded(newUri));
            } else if (identifier.startsWith(QLatin1String("!"))) {
                QByteArray newUri =
                  "matrix:roomid/" + QUrl::toPercentEncoding(identifier.remove(0, 1));
                if (!eventId.isEmpty())
                    newUri.append("/e/" + QUrl::toPercentEncoding(eventId.remove(0, 1)));
                if (!query.isEmpty())
                    newUri.append("?" + query.toUtf8());
                return handleMatrixUri(QUrl::fromEncoded(newUri));
            }
        }
    }

    // non-matrix URIs are not handled by us, return false
    if (uri_.scheme() != QLatin1String("matrix"))
        return false;

    auto tempPath = uri_.path(QUrl::ComponentFormattingOption::FullyEncoded);
    if (tempPath.startsWith('/'))
        tempPath.remove(0, 1);
    auto segments = QStringView(tempPath).split('/');

    if (segments.size() != 2 && segments.size() != 4)
        return false;

    auto sigil1 = segments[0];
    auto mxid1  = mxidFromSegments(sigil1, segments[1]);
    if (mxid1.isEmpty())
        return false;

    QString mxid2;
    if (segments.size() == 4 && segments[2] == QStringView(u"e")) {
        if (segments[3].isEmpty())
            return false;
        else
            mxid2 = "$" + QUrl::fromPercentEncoding(segments[3].toUtf8());
    }

    std::vector<std::string> vias;
    QString action;

    auto items =
      uri_.query(QUrl::ComponentFormattingOption::FullyEncoded).split('&', Qt::SkipEmptyParts);
    for (QString item : qAsConst(items)) {
        nhlog::ui()->info("item: {}", item.toStdString());

        if (item.startsWith(QLatin1String("action="))) {
            action = item.remove(QStringLiteral("action="));
        } else if (item.startsWith(QLatin1String("via="))) {
            vias.push_back(QUrl::fromPercentEncoding(item.remove(QStringLiteral("via=")).toUtf8())
                             .toStdString());
        }
    }

    if (sigil1 == u"u") {
        if (action.isEmpty()) {
            auto t = MainWindow::instance()->focusedRoom();
            if (!t.isEmpty() && cache::isRoomMember(mxid1.toStdString(), t.toStdString())) {
                auto rm = view_manager_->rooms()->getRoomById(t);
                if (rm)
                    rm->openUserProfile(mxid1);
                return true;
            }
            emit view_manager_->openGlobalUserProfile(mxid1);
        } else if (action == u"chat") {
            this->startChat(mxid1);
        }
        return true;
    } else if (sigil1 == u"roomid") {
        auto joined_rooms = cache::joinedRooms();
        auto targetRoomId = mxid1.toStdString();

        for (const auto &roomid : joined_rooms) {
            if (roomid == targetRoomId) {
                view_manager_->rooms()->setCurrentRoom(mxid1);
                if (!mxid2.isEmpty())
                    view_manager_->showEvent(mxid1, mxid2);
                return true;
            }
        }

        if (action == u"join" || action.isEmpty()) {
            joinRoomVia(targetRoomId, vias);
            return true;
        } else if (action == u"knock" || action.isEmpty()) {
            knockRoom(mxid1, vias);
            return true;
        }
        return false;
    } else if (sigil1 == u"r") {
        auto joined_rooms    = cache::joinedRooms();
        auto targetRoomAlias = mxid1.toStdString();

        for (const auto &roomid : joined_rooms) {
            auto aliases =
              cache::client()->getStateEvent<mtx::events::state::CanonicalAlias>(roomid);
            if (aliases) {
                if (aliases->content.alias == targetRoomAlias) {
                    view_manager_->rooms()->setCurrentRoom(QString::fromStdString(roomid));
                    if (!mxid2.isEmpty())
                        view_manager_->showEvent(QString::fromStdString(roomid), mxid2);
                    return true;
                }
            }
        }

        if (action == u"join" || action.isEmpty()) {
            joinRoomVia(mxid1.toStdString(), vias);
            return true;
        } else if (action == u"knock" || action.isEmpty()) {
            knockRoom(mxid1, vias);
            return true;
        }
        return false;
    }
    return false;
}

bool
ChatPage::handleMatrixUri(const QUrl &uri)
{
    return handleMatrixUri(uri.toString(QUrl::ComponentFormattingOption::FullyEncoded).toUtf8());
}

bool
ChatPage::isRoomActive(const QString &room_id)
{
    return QGuiApplication::focusWindow() && QGuiApplication::focusWindow()->isActive() &&
           MainWindow::instance()->windowForRoom(room_id) == QGuiApplication::focusWindow();
}
