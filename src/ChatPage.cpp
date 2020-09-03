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

#include <QApplication>
#include <QImageReader>
#include <QMessageBox>
#include <QSettings>
#include <QShortcut>
#include <QtConcurrent>

#include "ActiveCallBar.h"
#include "AvatarProvider.h"
#include "Cache.h"
#include "Cache_p.h"
#include "ChatPage.h"
#include "EventAccessors.h"
#include "Logging.h"
#include "MainWindow.h"
#include "MatrixClient.h"
#include "Olm.h"
#include "QuickSwitcher.h"
#include "RoomList.h"
#include "SideBarActions.h"
#include "Splitter.h"
#include "TextInputWidget.h"
#include "UserInfoWidget.h"
#include "UserSettingsPage.h"
#include "Utils.h"
#include "WebRTCSession.h"
#include "ui/OverlayModal.h"
#include "ui/Theme.h"

#include "notifications/Manager.h"

#include "dialogs/PlaceCall.h"
#include "dialogs/ReadReceipts.h"
#include "popups/UserMentions.h"
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

ChatPage::ChatPage(QSharedPointer<UserSettings> userSettings, QWidget *parent)
  : QWidget(parent)
  , isConnected_(true)
  , userSettings_{userSettings}
  , notificationsManager(this)
  , callManager_(userSettings)
{
        setObjectName("chatPage");

        qRegisterMetaType<std::optional<mtx::crypto::EncryptedFile>>();
        qRegisterMetaType<std::optional<RelatedInfo>>();
        qRegisterMetaType<mtx::presence::PresenceState>();

        topLayout_ = new QHBoxLayout(this);
        topLayout_->setSpacing(0);
        topLayout_->setMargin(0);

        communitiesList_ = new CommunitiesList(this);
        topLayout_->addWidget(communitiesList_);

        splitter = new Splitter(this);
        splitter->setHandleWidth(0);

        topLayout_->addWidget(splitter);

        // SideBar
        sideBar_ = new QFrame(this);
        sideBar_->setObjectName("sideBar");
        sideBar_->setMinimumWidth(::splitter::calculateSidebarSizes(QFont{}).normal);
        sideBarLayout_ = new QVBoxLayout(sideBar_);
        sideBarLayout_->setSpacing(0);
        sideBarLayout_->setMargin(0);

        sideBarTopWidget_ = new QWidget(sideBar_);
        sidebarActions_   = new SideBarActions(this);
        connect(
          sidebarActions_, &SideBarActions::showSettings, this, &ChatPage::showUserSettingsPage);
        connect(sidebarActions_, &SideBarActions::joinRoom, this, &ChatPage::joinRoom);
        connect(sidebarActions_, &SideBarActions::createRoom, this, &ChatPage::createRoom);

        user_info_widget_    = new UserInfoWidget(sideBar_);
        user_mentions_popup_ = new popups::UserMentions();
        room_list_           = new RoomList(userSettings, sideBar_);
        connect(room_list_, &RoomList::joinRoom, this, &ChatPage::joinRoom);

        sideBarLayout_->addWidget(user_info_widget_);
        sideBarLayout_->addWidget(room_list_);
        sideBarLayout_->addWidget(sidebarActions_);

        sideBarTopWidgetLayout_ = new QVBoxLayout(sideBarTopWidget_);
        sideBarTopWidgetLayout_->setSpacing(0);
        sideBarTopWidgetLayout_->setMargin(0);

        // Content
        content_ = new QFrame(this);
        content_->setObjectName("mainContent");
        contentLayout_ = new QVBoxLayout(content_);
        contentLayout_->setSpacing(0);
        contentLayout_->setMargin(0);

        view_manager_ = new TimelineViewManager(userSettings_, &callManager_, this);

        contentLayout_->addWidget(view_manager_->getWidget());

        activeCallBar_ = new ActiveCallBar(this);
        contentLayout_->addWidget(activeCallBar_);
        activeCallBar_->hide();
        connect(
          &callManager_, &CallManager::newCallParty, activeCallBar_, &ActiveCallBar::setCallParty);

        // Splitter
        splitter->addWidget(sideBar_);
        splitter->addWidget(content_);
        splitter->restoreSizes(parent->width());

        text_input_ = new TextInputWidget(this);
        contentLayout_->addWidget(text_input_);

        typingRefresher_ = new QTimer(this);
        typingRefresher_->setInterval(TYPING_REFRESH_TIMEOUT);

        connect(this, &ChatPage::connectionLost, this, [this]() {
                nhlog::net()->info("connectivity lost");
                isConnected_ = false;
                http::client()->shutdown();
                text_input_->disableInput();
        });
        connect(this, &ChatPage::connectionRestored, this, [this]() {
                nhlog::net()->info("trying to re-connect");
                text_input_->enableInput();
                isConnected_ = true;

                // Drop all pending connections.
                http::client()->shutdown();
                trySync();
        });

        connect(text_input_,
                &TextInputWidget::clearRoomTimeline,
                view_manager_,
                &TimelineViewManager::clearCurrentRoomTimeline);

        connect(
          new QShortcut(QKeySequence("Ctrl+Down"), this), &QShortcut::activated, this, [this]() {
                  if (isVisible())
                          room_list_->nextRoom();
          });
        connect(
          new QShortcut(QKeySequence("Ctrl+Up"), this), &QShortcut::activated, this, [this]() {
                  if (isVisible())
                          room_list_->previousRoom();
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

        connect(
          view_manager_, &TimelineViewManager::showRoomList, splitter, &Splitter::showFullRoomList);
        connect(view_manager_, &TimelineViewManager::inviteUsers, this, [this](QStringList users) {
                const auto room_id = current_room_.toStdString();

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

        connect(room_list_, &RoomList::roomChanged, this, [this](QString room_id) {
                this->current_room_ = room_id;
        });
        connect(room_list_, &RoomList::roomChanged, text_input_, &TextInputWidget::stopTyping);
        connect(room_list_, &RoomList::roomChanged, splitter, &Splitter::showChatView);
        connect(room_list_, &RoomList::roomChanged, text_input_, &TextInputWidget::focusLineEdit);
        connect(
          room_list_, &RoomList::roomChanged, view_manager_, &TimelineViewManager::setHistoryView);

        connect(room_list_, &RoomList::acceptInvite, this, [this](const QString &room_id) {
                joinRoom(room_id);
                room_list_->removeRoom(room_id, currentRoom() == room_id);
        });

        connect(room_list_, &RoomList::declineInvite, this, [this](const QString &room_id) {
                leaveRoom(room_id);
                room_list_->removeRoom(room_id, currentRoom() == room_id);
        });

        connect(
          text_input_, &TextInputWidget::startedTyping, this, &ChatPage::sendTypingNotifications);
        connect(typingRefresher_, &QTimer::timeout, this, &ChatPage::sendTypingNotifications);
        connect(text_input_, &TextInputWidget::stoppedTyping, this, [this]() {
                if (!userSettings_->typingNotifications())
                        return;

                typingRefresher_->stop();

                if (current_room_.isEmpty())
                        return;

                http::client()->stop_typing(
                  current_room_.toStdString(), [](mtx::http::RequestErr err) {
                          if (err) {
                                  nhlog::net()->warn("failed to stop typing notifications: {}",
                                                     err->matrix_error.error);
                          }
                  });
        });

        connect(view_manager_,
                &TimelineViewManager::updateRoomsLastMessage,
                room_list_,
                &RoomList::updateRoomDescription);

        connect(room_list_,
                SIGNAL(totalUnreadMessageCountUpdated(int)),
                this,
                SLOT(showUnreadMessageNotification(int)));

        connect(text_input_,
                &TextInputWidget::sendTextMessage,
                view_manager_,
                &TimelineViewManager::queueTextMessage);

        connect(text_input_,
                &TextInputWidget::sendEmoteMessage,
                view_manager_,
                &TimelineViewManager::queueEmoteMessage);

        connect(text_input_, &TextInputWidget::sendJoinRoomRequest, this, &ChatPage::joinRoom);

        // invites and bans via quick command
        connect(text_input_, &TextInputWidget::sendInviteRoomRequest, this, &ChatPage::inviteUser);
        connect(text_input_, &TextInputWidget::sendKickRoomRequest, this, &ChatPage::kickUser);
        connect(text_input_, &TextInputWidget::sendBanRoomRequest, this, &ChatPage::banUser);
        connect(text_input_, &TextInputWidget::sendUnbanRoomRequest, this, &ChatPage::unbanUser);

        connect(
          text_input_, &TextInputWidget::changeRoomNick, this, [this](const QString &displayName) {
                  mtx::events::state::Member member;
                  member.display_name = displayName.toStdString();
                  member.avatar_url =
                    cache::avatarUrl(currentRoom(),
                                     QString::fromStdString(http::client()->user_id().to_string()))
                      .toStdString();
                  member.membership = mtx::events::state::Membership::Join;

                  http::client()->send_state_event(
                    currentRoom().toStdString(),
                    http::client()->user_id().to_string(),
                    member,
                    [](mtx::responses::EventId, mtx::http::RequestErr err) {
                            if (err)
                                    nhlog::net()->error("Failed to set room displayname: {}",
                                                        err->matrix_error.error);
                    });
          });

        connect(
          text_input_,
          &TextInputWidget::uploadMedia,
          this,
          [this](QSharedPointer<QIODevice> dev, QString mimeClass, const QString &fn) {
                  if (!dev->open(QIODevice::ReadOnly)) {
                          emit uploadFailed(
                            QString("Error while reading media: %1").arg(dev->errorString()));
                          return;
                  }

                  auto bin = dev->readAll();
                  QMimeDatabase db;
                  QMimeType mime = db.mimeTypeForData(bin);

                  auto payload = std::string(bin.data(), bin.size());
                  std::optional<mtx::crypto::EncryptedFile> encryptedFile;
                  if (cache::isRoomEncrypted(current_room_.toStdString())) {
                          mtx::crypto::BinaryBuf buf;
                          std::tie(buf, encryptedFile) = mtx::crypto::encrypt_file(payload);
                          payload                      = mtx::crypto::to_string(buf);
                  }

                  QSize dimensions;
                  QString blurhash;
                  if (mimeClass == "image") {
                          QImage img = utils::readImage(&bin);

                          dimensions = img.size();
                          if (img.height() > 200 && img.width() > 360)
                                  img = img.scaled(360, 200, Qt::KeepAspectRatioByExpanding);
                          std::vector<unsigned char> data;
                          for (int y = 0; y < img.height(); y++) {
                                  for (int x = 0; x < img.width(); x++) {
                                          auto p = img.pixel(x, y);
                                          data.push_back(static_cast<unsigned char>(qRed(p)));
                                          data.push_back(static_cast<unsigned char>(qGreen(p)));
                                          data.push_back(static_cast<unsigned char>(qBlue(p)));
                                  }
                          }
                          blurhash = QString::fromStdString(
                            blurhash::encode(data.data(), img.width(), img.height(), 4, 3));
                  }

                  http::client()->upload(
                    payload,
                    encryptedFile ? "application/octet-stream" : mime.name().toStdString(),
                    QFileInfo(fn).fileName().toStdString(),
                    [this,
                     room_id  = current_room_,
                     filename = fn,
                     encryptedFile,
                     mimeClass,
                     mime = mime.name(),
                     size = payload.size(),
                     dimensions,
                     blurhash](const mtx::responses::ContentURI &res, mtx::http::RequestErr err) {
                            if (err) {
                                    emit uploadFailed(
                                      tr("Failed to upload media. Please try again."));
                                    nhlog::net()->warn("failed to upload media: {} {} ({})",
                                                       err->matrix_error.error,
                                                       to_string(err->matrix_error.errcode),
                                                       static_cast<int>(err->status_code));
                                    return;
                            }

                            emit mediaUploaded(room_id,
                                               filename,
                                               encryptedFile,
                                               QString::fromStdString(res.content_uri),
                                               mimeClass,
                                               mime,
                                               size,
                                               dimensions,
                                               blurhash);
                    });
          });

        connect(this, &ChatPage::uploadFailed, this, [this](const QString &msg) {
                text_input_->hideUploadSpinner();
                emit showNotification(msg);
        });
        connect(this,
                &ChatPage::mediaUploaded,
                this,
                [this](QString roomid,
                       QString filename,
                       std::optional<mtx::crypto::EncryptedFile> encryptedFile,
                       QString url,
                       QString mimeClass,
                       QString mime,
                       qint64 dsize,
                       QSize dimensions,
                       QString blurhash) {
                        text_input_->hideUploadSpinner();

                        if (encryptedFile)
                                encryptedFile->url = url.toStdString();

                        if (mimeClass == "image")
                                view_manager_->queueImageMessage(roomid,
                                                                 filename,
                                                                 encryptedFile,
                                                                 url,
                                                                 mime,
                                                                 dsize,
                                                                 dimensions,
                                                                 blurhash);
                        else if (mimeClass == "audio")
                                view_manager_->queueAudioMessage(
                                  roomid, filename, encryptedFile, url, mime, dsize);
                        else if (mimeClass == "video")
                                view_manager_->queueVideoMessage(
                                  roomid, filename, encryptedFile, url, mime, dsize);
                        else
                                view_manager_->queueFileMessage(
                                  roomid, filename, encryptedFile, url, mime, dsize);
                });

        connect(text_input_, &TextInputWidget::callButtonPress, this, [this]() {
                if (callManager_.onActiveCall()) {
                        callManager_.hangUp();
                } else {
                        if (auto roomInfo = cache::singleRoomInfo(current_room_.toStdString());
                            roomInfo.member_count != 2) {
                                showNotification("Voice calls are limited to 1:1 rooms.");
                        } else {
                                std::vector<RoomMember> members(
                                  cache::getMembers(current_room_.toStdString()));
                                const RoomMember &callee =
                                  members.front().user_id == utils::localUser() ? members.back()
                                                                                : members.front();
                                auto dialog = new dialogs::PlaceCall(
                                  callee.user_id,
                                  callee.display_name,
                                  QString::fromStdString(roomInfo.name),
                                  QString::fromStdString(roomInfo.avatar_url),
                                  userSettings_,
                                  MainWindow::instance());
                                connect(dialog, &dialogs::PlaceCall::voice, this, [this]() {
                                        callManager_.sendInvite(current_room_);
                                });
                                utils::centerWidget(dialog, MainWindow::instance());
                                dialog->show();
                        }
                }
        });

        connect(
          this, &ChatPage::updateGroupsInfo, communitiesList_, &CommunitiesList::setCommunities);

        connect(this, &ChatPage::leftRoom, this, &ChatPage::removeRoom);
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

        connect(communitiesList_,
                &CommunitiesList::communityChanged,
                this,
                [this](const QString &groupId) {
                        current_community_ = groupId;

                        if (groupId == "world")
                                room_list_->removeFilter();
                        else
                                room_list_->applyFilter(communitiesList_->roomList(groupId));
                });

        connect(&notificationsManager,
                &NotificationsManager::notificationClicked,
                this,
                [this](const QString &roomid, const QString &eventid) {
                        Q_UNUSED(eventid)
                        room_list_->highlightSelectedRoom(roomid);
                        activateWindow();
                });

        setGroupViewState(userSettings_->groupView());

        connect(userSettings_.data(),
                &UserSettings::groupViewStateChanged,
                this,
                &ChatPage::setGroupViewState);

        connect(this, &ChatPage::initializeRoomList, room_list_, &RoomList::initialize);
        connect(this,
                &ChatPage::initializeViews,
                view_manager_,
                [this](const mtx::responses::Rooms &rooms) { view_manager_->sync(rooms); });
        connect(this,
                &ChatPage::initializeEmptyViews,
                view_manager_,
                &TimelineViewManager::initWithMessages);
        connect(this,
                &ChatPage::initializeMentions,
                user_mentions_popup_,
                &popups::UserMentions::initializeMentions);
        connect(this, &ChatPage::syncUI, this, [this](const mtx::responses::Rooms &rooms) {
                try {
                        room_list_->cleanupInvites(cache::invites());
                } catch (const lmdb::error &e) {
                        nhlog::db()->error("failed to retrieve invites: {}", e.what());
                }

                view_manager_->sync(rooms);
                removeLeftRooms(rooms.leave);

                bool hasNotifications = false;
                for (const auto &room : rooms.join) {
                        auto room_id = QString::fromStdString(room.first);
                        updateRoomNotificationCount(
                          room_id,
                          room.second.unread_notifications.notification_count,
                          room.second.unread_notifications.highlight_count);

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
        connect(this, &ChatPage::syncRoomlist, room_list_, &RoomList::sync);
        connect(this, &ChatPage::syncTags, communitiesList_, &CommunitiesList::syncTags);

        // Callbacks to update the user info (top left corner of the page).
        connect(this, &ChatPage::setUserAvatar, user_info_widget_, &UserInfoWidget::setAvatar);
        connect(this, &ChatPage::setUserDisplayName, this, [this](const QString &name) {
                auto userid = utils::localUser();
                user_info_widget_->setUserId(userid);
                user_info_widget_->setDisplayName(name);
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

        instance_ = this;
}

void
ChatPage::logout()
{
        deleteConfigs();

        resetUI();

        emit closing();
        connectivityTimer_.stop();
}

void
ChatPage::dropToLoginPage(const QString &msg)
{
        nhlog::ui()->info("dropping to the login page: {}", msg.toStdString());

        deleteConfigs();
        resetUI();

        http::client()->shutdown();
        connectivityTimer_.stop();

        emit showLoginPage(msg);
}

void
ChatPage::resetUI()
{
        room_list_->clear();
        user_info_widget_->reset();
        view_manager_->clearAll();

        showUnreadMessageNotification(0);
}

void
ChatPage::focusMessageInput()
{
        this->text_input_->focusLineEdit();
}

void
ChatPage::deleteConfigs()
{
        QSettings settings;
        settings.beginGroup("auth");
        settings.remove("");
        settings.endGroup();
        settings.beginGroup("client");
        settings.remove("");
        settings.endGroup();
        settings.beginGroup("notifications");
        settings.remove("");
        settings.endGroup();

        cache::deleteData();
        http::client()->clear();
}

void
ChatPage::bootstrap(QString userid, QString homeserver, QString token)
{
        using namespace mtx::identifiers;

        try {
                http::client()->set_user(parse<User>(userid.toStdString()));
        } catch (const std::invalid_argument &e) {
                nhlog::ui()->critical("bootstrapped with invalid user_id: {}",
                                      userid.toStdString());
        }

        http::client()->set_server(homeserver.toStdString());
        http::client()->set_access_token(token.toStdString());

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

                connect(
                  cache::client(), &Cache::roomReadStatus, room_list_, &RoomList::updateReadStatus);

                connect(cache::client(),
                        &Cache::removeNotification,
                        &notificationsManager,
                        &NotificationsManager::removeNotification);

                const bool isInitialized = cache::isInitialized();
                const auto cacheVersion  = cache::formatVersion();

                callManager_.refreshTurnServer();

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
ChatPage::showUnreadMessageNotification(int count)
{
        emit unreadMessages(count);

        // TODO: Make the default title a const.
        if (count == 0)
                emit changeWindowTitle("nheko");
        else
                emit changeWindowTitle(QString("nheko (%1)").arg(count));
}

void
ChatPage::loadStateFromCache()
{
        emit contentLoaded();

        nhlog::db()->info("restoring state from cache");

        try {
                cache::restoreSessions();
                olm::client()->load(cache::restoreOlmAccount(), STORAGE_SECRET_KEY);

                cache::populateMembers();

                emit initializeEmptyViews(cache::roomMessages());
                emit initializeRoomList(cache::roomInfo());
                emit initializeMentions(cache::getTimelineMentions());
                emit syncTags(cache::roomInfo().toStdMap());

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

        // Start receiving events.
        emit trySyncCb();
}

void
ChatPage::showQuickSwitcher()
{
        auto dialog = new QuickSwitcher(this);

        connect(dialog, &QuickSwitcher::roomSelected, room_list_, &RoomList::highlightSelectedRoom);
        connect(dialog, &QuickSwitcher::closing, this, [this]() {
                MainWindow::instance()->hideOverlay();
                text_input_->setFocus(Qt::FocusReason::PopupFocusReason);
        });

        MainWindow::instance()->showTransparentOverlayModal(dialog);
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

        room_list_->removeRoom(room_id, room_id == current_room_);
}

void
ChatPage::removeLeftRooms(const std::map<std::string, mtx::responses::LeftRoom> &rooms)
{
        for (auto it = rooms.cbegin(); it != rooms.cend(); ++it) {
                const auto room_id = QString::fromStdString(it->first);
                room_list_->removeRoom(room_id, room_id == current_room_);
        }
}

void
ChatPage::setGroupViewState(bool isEnabled)
{
        if (!isEnabled) {
                communitiesList_->communityChanged("world");
                communitiesList_->hide();

                return;
        }

        communitiesList_->show();
}

void
ChatPage::updateRoomNotificationCount(const QString &room_id,
                                      uint16_t notification_count,
                                      uint16_t highlight_count)
{
        room_list_->updateUnreadMessageCount(room_id, notification_count, highlight_count);
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
                                const auto user_id =
                                  QString::fromStdString(mtx::accessors::sender(item.event));

                                // We should only sent one notification per event.
                                cache::markSentNotification(event_id);

                                // Don't send a notification when the current room is opened.
                                if (isRoomActive(room_id))
                                        continue;

                                if (userSettings_->hasAlertOnNotification()) {
                                        QApplication::alert(this);
                                }

                                if (userSettings_->hasDesktopNotifications()) {
                                        notificationsManager.postNotification(
                                          room_id,
                                          QString::fromStdString(event_id),
                                          QString::fromStdString(
                                            cache::singleRoomInfo(item.room_id).name),
                                          cache::displayName(room_id, user_id),
                                          utils::event_body(item.event),
                                          cache::getRoomAvatar(room_id));
                                }
                        }
                } catch (const lmdb::error &e) {
                        nhlog::db()->warn("error while sending notification: {}", e.what());
                }
        }
}

void
ChatPage::showNotificationsDialog(const QPoint &widgetPos)
{
        auto notifDialog = user_mentions_popup_;

        notifDialog->setGeometry(
          widgetPos.x() - (width() / 10), widgetPos.y() + 25, width() / 5, height() / 2);

        notifDialog->raise();
        notifDialog->showPopup();
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
          opts,
          std::bind(
            &ChatPage::initialSyncHandler, this, std::placeholders::_1, std::placeholders::_2));
}

void
ChatPage::handleSyncResponse(mtx::responses::Sync res)
{
        nhlog::net()->debug("sync completed: {}", res.next_batch);

        // Ensure that we have enough one-time keys available.
        ensureOneTimeKeyCount(res.device_one_time_keys_count);

        // TODO: fine grained error handling
        try {
                cache::saveState(res);
                olm::handle_to_device_messages(res.to_device.events);

                auto updates = cache::roomUpdates(res);

                emit syncRoomlist(updates);

                emit syncUI(res.rooms);

                emit syncTags(cache::roomTagUpdates(res));

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
          [this, since = cache::nextBatchToken()](const mtx::responses::Sync &res,
                                                  mtx::http::RequestErr err) {
                  if (since != cache::nextBatchToken()) {
                          nhlog::net()->warn("Duplicate sync, dropping");
                          return;
                  }

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

                          nhlog::net()->error("sync error: {} {}", status_code, err_code);
                          emit tryDelayedSyncCb();
                          return;
                  }

                  emit newSyncResponse(res);
          });
}

void
ChatPage::joinRoom(const QString &room)
{
        const auto room_id = room.toStdString();

        http::client()->join_room(
          room_id, [this, room_id](const nlohmann::json &, mtx::http::RequestErr err) {
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

                  emit showNotification(
                    tr("Room %1 created.").arg(QString::fromStdString(res.room_id.to_string())));
          });
}

void
ChatPage::leaveRoom(const QString &room_id)
{
        http::client()->leave_room(
          room_id.toStdString(), [this, room_id](const json &, mtx::http::RequestErr err) {
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
ChatPage::inviteUser(QString userid, QString reason)
{
        auto room = current_room_;

        if (QMessageBox::question(this,
                                  tr("Confirm invite"),
                                  tr("Do you really want to invite %1 (%2)?")
                                    .arg(cache::displayName(current_room_, userid))
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
        auto room = current_room_;

        if (QMessageBox::question(this,
                                  tr("Confirm kick"),
                                  tr("Do you really want to kick %1 (%2)?")
                                    .arg(cache::displayName(current_room_, userid))
                                    .arg(userid)) != QMessageBox::Yes)
                return;

        http::client()->kick_user(
          room.toStdString(),
          userid.toStdString(),
          [this, userid, room](const mtx::responses::Empty &, mtx::http::RequestErr err) {
                  if (err) {
                          emit showNotification(
                            tr("Failed to kick %1 to %2: %3")
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
        auto room = current_room_;

        if (QMessageBox::question(this,
                                  tr("Confirm ban"),
                                  tr("Do you really want to ban %1 (%2)?")
                                    .arg(cache::displayName(current_room_, userid))
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
        auto room = current_room_;

        if (QMessageBox::question(this,
                                  tr("Confirm unban"),
                                  tr("Do you really want to unban %1 (%2)?")
                                    .arg(cache::displayName(current_room_, userid))
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
ChatPage::sendTypingNotifications()
{
        if (!userSettings_->typingNotifications())
                return;

        http::client()->start_typing(
          current_room_.toStdString(), 10'000, [](mtx::http::RequestErr err) {
                  if (err) {
                          nhlog::net()->warn("failed to send typing notification: {}",
                                             err->matrix_error.error);
                  }
          });
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
ChatPage::initialSyncHandler(const mtx::responses::Sync &res, mtx::http::RequestErr err)
{
        // TODO: Initial Sync should include mentions as well...

        if (err) {
                const auto error      = QString::fromStdString(err->matrix_error.error);
                const auto msg        = tr("Please try to login again: %1").arg(error);
                const auto err_code   = mtx::errors::to_string(err->matrix_error.errcode);
                const int status_code = static_cast<int>(err->status_code);

                nhlog::net()->error("initial sync error: {} {}", status_code, err_code);

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
                cache::saveState(res);

                olm::handle_to_device_messages(res.to_device.events);

                emit initializeViews(std::move(res.rooms));
                emit initializeRoomList(cache::roomInfo());
                emit initializeMentions(cache::getTimelineMentions());

                cache::calculateRoomReadStatus();
                emit syncTags(cache::roomInfo().toStdMap());
        } catch (const lmdb::error &e) {
                nhlog::db()->error("failed to save state after initial sync: {}", e.what());
                startInitialSync();
                return;
        }

        emit trySyncCb();
        emit contentLoaded();
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

        http::client()->joined_groups(
          [this](const mtx::responses::JoinedGroups &res, mtx::http::RequestErr err) {
                  if (err) {
                          nhlog::net()->critical("failed to retrieve joined groups: {} {}",
                                                 static_cast<int>(err->status_code),
                                                 err->matrix_error.error);
                          return;
                  }

                  emit updateGroupsInfo(res);
          });
}

void
ChatPage::hideSideBars()
{
        // Don't hide side bar, if we are currently only showing the side bar!
        if (view_manager_->getWidget()->isVisible()) {
                communitiesList_->hide();
                sideBar_->hide();
        }
        view_manager_->enableBackButton();
}

void
ChatPage::showSideBars()
{
        if (userSettings_->groupView())
                communitiesList_->show();

        sideBar_->show();
        view_manager_->disableBackButton();
        content_->show();
}

uint64_t
ChatPage::timelineWidth()
{
        int sidebarWidth = sideBar_->minimumSize().width();
        sidebarWidth += communitiesList_->minimumSize().width();
        nhlog::ui()->info("timelineWidth: {}", size().width() - sidebarWidth);

        return size().width() - sidebarWidth;
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
        connect(&callManager_,
                qOverload<const QString &, const T &>(&CallManager::newMessage),
                view_manager_,
                qOverload<const QString &, const T &>(&TimelineViewManager::queueCallMessage));
}
