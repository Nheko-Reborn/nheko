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
#include <QSettings>
#include <QtConcurrent>

#include "AvatarProvider.h"
#include "Cache.h"
#include "ChatPage.h"
#include "Logging.h"
#include "MainWindow.h"
#include "MatrixClient.h"
#include "Olm.h"
#include "QuickSwitcher.h"
#include "RoomList.h"
#include "SideBarActions.h"
#include "Splitter.h"
#include "TextInputWidget.h"
#include "TopRoomBar.h"
#include "TypingDisplay.h"
#include "UserInfoWidget.h"
#include "UserSettingsPage.h"
#include "Utils.h"
#include "ui/OverlayModal.h"
#include "ui/Theme.h"

#include "notifications/Manager.h"

#include "dialogs/ReadReceipts.h"
#include "timeline/TimelineViewManager.h"

// TODO: Needs to be updated with an actual secret.
static const std::string STORAGE_SECRET_KEY("secret");

ChatPage *ChatPage::instance_             = nullptr;
constexpr int CHECK_CONNECTIVITY_INTERVAL = 15'000;
constexpr int RETRY_TIMEOUT               = 5'000;
constexpr size_t MAX_ONETIME_KEYS         = 50;

ChatPage::ChatPage(QSharedPointer<UserSettings> userSettings, QWidget *parent)
  : QWidget(parent)
  , isConnected_(true)
  , userSettings_{userSettings}
  , notificationsManager(this)
{
        setObjectName("chatPage");

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
        sideBar_->setMinimumWidth(ui::sidebar::NormalSize);
        sideBarLayout_ = new QVBoxLayout(sideBar_);
        sideBarLayout_->setSpacing(0);
        sideBarLayout_->setMargin(0);

        sideBarTopWidget_ = new QWidget(sideBar_);
        sidebarActions_   = new SideBarActions(this);
        connect(
          sidebarActions_, &SideBarActions::showSettings, this, &ChatPage::showUserSettingsPage);
        connect(sidebarActions_, &SideBarActions::joinRoom, this, &ChatPage::joinRoom);
        connect(sidebarActions_, &SideBarActions::createRoom, this, &ChatPage::createRoom);

        user_info_widget_ = new UserInfoWidget(sideBar_);
        room_list_        = new RoomList(userSettings_, sideBar_);
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

        top_bar_      = new TopRoomBar(this);
        view_manager_ = new TimelineViewManager(this);

        contentLayout_->addWidget(top_bar_);
        contentLayout_->addWidget(view_manager_);

        connect(this,
                &ChatPage::removeTimelineEvent,
                view_manager_,
                &TimelineViewManager::removeTimelineEvent);

        // Splitter
        splitter->addWidget(sideBar_);
        splitter->addWidget(content_);
        splitter->restoreSizes(parent->width());

        text_input_ = new TextInputWidget(this);
        contentLayout_->addWidget(text_input_);

        typingDisplay_ = new TypingDisplay(content_);
        typingDisplay_->hide();
        connect(
          text_input_, &TextInputWidget::heightChanged, typingDisplay_, &TypingDisplay::setOffset);

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

        connect(top_bar_, &TopRoomBar::showRoomList, splitter, &Splitter::showFullRoomList);
        connect(top_bar_, &TopRoomBar::inviteUsers, this, [this](QStringList users) {
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
                                                    QString("Failed to invite user: %1").arg(user));
                                                  return;
                                          }

                                          emit showNotification(
                                            QString("Invited user: %1").arg(user));
                                  });
                        });
                }
        });

        connect(room_list_, &RoomList::roomChanged, this, [this](const QString &roomid) {
                QStringList users;

                if (!userSettings_->isTypingNotificationsEnabled()) {
                        typingDisplay_->setUsers(users);
                        return;
                }

                if (typingUsers_.find(roomid) != typingUsers_.end())
                        users = typingUsers_[roomid];

                typingDisplay_->setUsers(users);
        });
        connect(room_list_, &RoomList::roomChanged, text_input_, &TextInputWidget::stopTyping);
        connect(room_list_, &RoomList::roomChanged, this, &ChatPage::changeTopRoomInfo);
        connect(room_list_, &RoomList::roomChanged, splitter, &Splitter::showChatView);
        connect(room_list_, &RoomList::roomChanged, text_input_, &TextInputWidget::focusLineEdit);
        connect(
          room_list_, &RoomList::roomChanged, view_manager_, &TimelineViewManager::setHistoryView);

        connect(room_list_, &RoomList::acceptInvite, this, [this](const QString &room_id) {
                view_manager_->addRoom(room_id);
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
                if (!userSettings_->isTypingNotificationsEnabled())
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
                SIGNAL(sendTextMessage(const QString &)),
                view_manager_,
                SLOT(queueTextMessage(const QString &)));

        connect(text_input_,
                SIGNAL(sendEmoteMessage(const QString &)),
                view_manager_,
                SLOT(queueEmoteMessage(const QString &)));

        connect(text_input_, &TextInputWidget::sendJoinRoomRequest, this, &ChatPage::joinRoom);

        connect(
          text_input_,
          &TextInputWidget::uploadImage,
          this,
          [this](QSharedPointer<QIODevice> dev, const QString &fn) {
                  QMimeDatabase db;
                  QMimeType mime = db.mimeTypeForData(dev.data());

                  if (!dev->open(QIODevice::ReadOnly)) {
                          emit uploadFailed(
                            QString("Error while reading media: %1").arg(dev->errorString()));
                          return;
                  }

                  auto bin        = dev->peek(dev->size());
                  auto payload    = std::string(bin.data(), bin.size());
                  auto dimensions = QImageReader(dev.data()).size();

                  http::client()->upload(
                    payload,
                    mime.name().toStdString(),
                    QFileInfo(fn).fileName().toStdString(),
                    [this,
                     room_id  = current_room_,
                     filename = fn,
                     mime     = mime.name(),
                     size     = payload.size(),
                     dimensions](const mtx::responses::ContentURI &res, mtx::http::RequestErr err) {
                            if (err) {
                                    emit uploadFailed(
                                      tr("Failed to upload image. Please try again."));
                                    nhlog::net()->warn("failed to upload image: {} {} ({})",
                                                       err->matrix_error.error,
                                                       to_string(err->matrix_error.errcode),
                                                       static_cast<int>(err->status_code));
                                    return;
                            }

                            emit imageUploaded(room_id,
                                               filename,
                                               QString::fromStdString(res.content_uri),
                                               mime,
                                               size,
                                               dimensions);
                    });
          });

        connect(text_input_,
                &TextInputWidget::uploadFile,
                this,
                [this](QSharedPointer<QIODevice> dev, const QString &fn) {
                        QMimeDatabase db;
                        QMimeType mime = db.mimeTypeForData(dev.data());

                        if (!dev->open(QIODevice::ReadOnly)) {
                                emit uploadFailed(
                                  QString("Error while reading media: %1").arg(dev->errorString()));
                                return;
                        }

                        auto bin     = dev->readAll();
                        auto payload = std::string(bin.data(), bin.size());

                        http::client()->upload(
                          payload,
                          mime.name().toStdString(),
                          QFileInfo(fn).fileName().toStdString(),
                          [this,
                           room_id  = current_room_,
                           filename = fn,
                           mime     = mime.name(),
                           size     = payload.size()](const mtx::responses::ContentURI &res,
                                                  mtx::http::RequestErr err) {
                                  if (err) {
                                          emit uploadFailed(
                                            tr("Failed to upload file. Please try again."));
                                          nhlog::net()->warn("failed to upload file: {} ({})",
                                                             err->matrix_error.error,
                                                             static_cast<int>(err->status_code));
                                          return;
                                  }

                                  emit fileUploaded(room_id,
                                                    filename,
                                                    QString::fromStdString(res.content_uri),
                                                    mime,
                                                    size);
                          });
                });

        connect(text_input_,
                &TextInputWidget::uploadAudio,
                this,
                [this](QSharedPointer<QIODevice> dev, const QString &fn) {
                        QMimeDatabase db;
                        QMimeType mime = db.mimeTypeForData(dev.data());

                        if (!dev->open(QIODevice::ReadOnly)) {
                                emit uploadFailed(
                                  QString("Error while reading media: %1").arg(dev->errorString()));
                                return;
                        }

                        auto bin     = dev->readAll();
                        auto payload = std::string(bin.data(), bin.size());

                        http::client()->upload(
                          payload,
                          mime.name().toStdString(),
                          QFileInfo(fn).fileName().toStdString(),
                          [this,
                           room_id  = current_room_,
                           filename = fn,
                           mime     = mime.name(),
                           size     = payload.size()](const mtx::responses::ContentURI &res,
                                                  mtx::http::RequestErr err) {
                                  if (err) {
                                          emit uploadFailed(
                                            tr("Failed to upload audio. Please try again."));
                                          nhlog::net()->warn("failed to upload audio: {} ({})",
                                                             err->matrix_error.error,
                                                             static_cast<int>(err->status_code));
                                          return;
                                  }

                                  emit audioUploaded(room_id,
                                                     filename,
                                                     QString::fromStdString(res.content_uri),
                                                     mime,
                                                     size);
                          });
                });
        connect(text_input_,
                &TextInputWidget::uploadVideo,
                this,
                [this](QSharedPointer<QIODevice> dev, const QString &fn) {
                        QMimeDatabase db;
                        QMimeType mime = db.mimeTypeForData(dev.data());

                        if (!dev->open(QIODevice::ReadOnly)) {
                                emit uploadFailed(
                                  QString("Error while reading media: %1").arg(dev->errorString()));
                                return;
                        }

                        auto bin     = dev->readAll();
                        auto payload = std::string(bin.data(), bin.size());

                        http::client()->upload(
                          payload,
                          mime.name().toStdString(),
                          QFileInfo(fn).fileName().toStdString(),
                          [this,
                           room_id  = current_room_,
                           filename = fn,
                           mime     = mime.name(),
                           size     = payload.size()](const mtx::responses::ContentURI &res,
                                                  mtx::http::RequestErr err) {
                                  if (err) {
                                          emit uploadFailed(
                                            tr("Failed to upload video. Please try again."));
                                          nhlog::net()->warn("failed to upload video: {} ({})",
                                                             err->matrix_error.error,
                                                             static_cast<int>(err->status_code));
                                          return;
                                  }

                                  emit videoUploaded(room_id,
                                                     filename,
                                                     QString::fromStdString(res.content_uri),
                                                     mime,
                                                     size);
                          });
                });

        connect(this, &ChatPage::uploadFailed, this, [this](const QString &msg) {
                text_input_->hideUploadSpinner();
                emit showNotification(msg);
        });
        connect(this,
                &ChatPage::imageUploaded,
                this,
                [this](QString roomid,
                       QString filename,
                       QString url,
                       QString mime,
                       qint64 dsize,
                       QSize dimensions) {
                        text_input_->hideUploadSpinner();
                        view_manager_->queueImageMessage(
                          roomid, filename, url, mime, dsize, dimensions);
                });
        connect(this,
                &ChatPage::fileUploaded,
                this,
                [this](QString roomid, QString filename, QString url, QString mime, qint64 dsize) {
                        text_input_->hideUploadSpinner();
                        view_manager_->queueFileMessage(roomid, filename, url, mime, dsize);
                });
        connect(this,
                &ChatPage::audioUploaded,
                this,
                [this](QString roomid, QString filename, QString url, QString mime, qint64 dsize) {
                        text_input_->hideUploadSpinner();
                        view_manager_->queueAudioMessage(roomid, filename, url, mime, dsize);
                });
        connect(this,
                &ChatPage::videoUploaded,
                this,
                [this](QString roomid, QString filename, QString url, QString mime, qint64 dsize) {
                        text_input_->hideUploadSpinner();
                        view_manager_->queueVideoMessage(roomid, filename, url, mime, dsize);
                });

        connect(room_list_, &RoomList::roomAvatarChanged, this, &ChatPage::updateTopBarAvatar);

        connect(
          this, &ChatPage::updateGroupsInfo, communitiesList_, &CommunitiesList::setCommunities);

        connect(this, &ChatPage::leftRoom, this, &ChatPage::removeRoom);
        connect(this, &ChatPage::notificationsRetrieved, this, &ChatPage::sendDesktopNotifications);

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

        setGroupViewState(userSettings_->isGroupViewEnabled());

        connect(userSettings_.data(),
                &UserSettings::groupViewStateChanged,
                this,
                &ChatPage::setGroupViewState);

        connect(this, &ChatPage::initializeRoomList, room_list_, &RoomList::initialize);
        connect(this,
                &ChatPage::initializeViews,
                view_manager_,
                [this](const mtx::responses::Rooms &rooms) { view_manager_->initialize(rooms); });
        connect(this,
                &ChatPage::initializeEmptyViews,
                view_manager_,
                &TimelineViewManager::initWithMessages);
        connect(this, &ChatPage::syncUI, this, [this](const mtx::responses::Rooms &rooms) {
                try {
                        room_list_->cleanupInvites(cache::client()->invites());
                } catch (const lmdb::error &e) {
                        nhlog::db()->error("failed to retrieve invites: {}", e.what());
                }

                view_manager_->initialize(rooms);
                removeLeftRooms(rooms.leave);

                bool hasNotifications = false;
                for (const auto &room : rooms.join) {
                        auto room_id = QString::fromStdString(room.first);

                        updateTypingUsers(room_id, room.second.ephemeral.typing);
                        updateRoomNotificationCount(
                          room_id, room.second.unread_notifications.notification_count);

                        if (room.second.unread_notifications.notification_count > 0)
                                hasNotifications = true;
                }

                if (hasNotifications && userSettings_->hasDesktopNotifications())
                        http::client()->notifications(
                          5,
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
        connect(
          this, &ChatPage::syncTopBar, this, [this](const std::map<QString, RoomInfo> &updates) {
                  if (updates.find(currentRoom()) != updates.end())
                          changeTopRoomInfo(currentRoom());
          });

        // Callbacks to update the user info (top left corner of the page).
        connect(this, &ChatPage::setUserAvatar, user_info_widget_, &UserInfoWidget::setAvatar);
        connect(this, &ChatPage::setUserDisplayName, this, [this](const QString &name) {
                auto userid = utils::localUser();
                user_info_widget_->setUserId(userid);
                user_info_widget_->setDisplayName(name);
        });

        connect(this, &ChatPage::tryInitialSyncCb, this, &ChatPage::tryInitialSync);
        connect(this, &ChatPage::trySyncCb, this, &ChatPage::trySync);
        connect(this, &ChatPage::tryDelayedSyncCb, this, [this]() {
                QTimer::singleShot(RETRY_TIMEOUT, this, &ChatPage::trySync);
        });

        connect(this, &ChatPage::dropToLoginPageCb, this, &ChatPage::dropToLoginPage);
        connect(this, &ChatPage::messageReply, text_input_, &TextInputWidget::addReply);

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
        top_bar_->reset();
        user_info_widget_->reset();
        view_manager_->clearAll();

        showUnreadMessageNotification(0);
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

        cache::client()->deleteData();
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

                const bool isInitialized = cache::client()->isInitialized();
                const bool isValid       = cache::client()->isFormatValid();

                if (!isInitialized) {
                        cache::client()->setCurrentFormat();
                } else if (isInitialized && !isValid) {
                        // TODO: Deleting session data but keep using the
                        //	 same device doesn't work.
                        cache::client()->deleteData();

                        cache::init(userid);
                        cache::client()->setCurrentFormat();
                } else if (isInitialized) {
                        loadStateFromCache();
                        return;
                }

        } catch (const lmdb::error &e) {
                nhlog::db()->critical("failure during boot: {}", e.what());
                cache::client()->deleteData();
                nhlog::net()->info("falling back to initial sync");
        }

        try {
                // It's the first time syncing with this device
                // There isn't a saved olm account to restore.
                nhlog::crypto()->info("creating new olm account");
                olm::client()->create_new_account();
                cache::client()->saveOlmAccount(olm::client()->save(STORAGE_SECRET_KEY));
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
ChatPage::updateTopBarAvatar(const QString &roomid, const QPixmap &img)
{
        if (current_room_ != roomid)
                return;

        top_bar_->updateRoomAvatar(img.toImage());
}

void
ChatPage::changeTopRoomInfo(const QString &room_id)
{
        if (room_id.isEmpty()) {
                nhlog::ui()->warn("cannot switch to empty room_id");
                return;
        }

        try {
                auto room_info = cache::client()->getRoomInfo({room_id.toStdString()});

                if (room_info.find(room_id) == room_info.end())
                        return;

                const auto name       = QString::fromStdString(room_info[room_id].name);
                const auto avatar_url = QString::fromStdString(room_info[room_id].avatar_url);

                top_bar_->updateRoomName(name);
                top_bar_->updateRoomTopic(QString::fromStdString(room_info[room_id].topic));

                auto img = cache::client()->getRoomAvatar(room_id);

                if (img.isNull())
                        top_bar_->updateRoomAvatarFromName(name);
                else
                        top_bar_->updateRoomAvatar(img);

        } catch (const lmdb::error &e) {
                nhlog::ui()->error("failed to change top bar room info: {}", e.what());
        }

        current_room_ = room_id;
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

        getProfileInfo();

        QtConcurrent::run([this]() {
                try {
                        cache::client()->restoreSessions();
                        olm::client()->load(cache::client()->restoreOlmAccount(),
                                            STORAGE_SECRET_KEY);

                        cache::client()->populateMembers();

                        emit initializeEmptyViews(cache::client()->roomMessages());
                        emit initializeRoomList(cache::client()->roomInfo());
                        emit syncTags(cache::client()->roomInfo().toStdMap());

                        cache::client()->calculateRoomReadStatus();

                } catch (const mtx::crypto::olm_exception &e) {
                        nhlog::crypto()->critical("failed to restore olm account: {}", e.what());
                        emit dropToLoginPageCb(
                          tr("Failed to restore OLM account. Please login again."));
                        return;
                } catch (const lmdb::error &e) {
                        nhlog::db()->critical("failed to restore cache: {}", e.what());
                        emit dropToLoginPageCb(
                          tr("Failed to restore save data. Please login again."));
                        return;
                } catch (const json::exception &e) {
                        nhlog::db()->critical("failed to parse cache data: {}", e.what());
                        return;
                }

                nhlog::crypto()->info("ed25519   : {}", olm::client()->identity_keys().ed25519);
                nhlog::crypto()->info("curve25519: {}", olm::client()->identity_keys().curve25519);

                // Start receiving events.
                emit trySyncCb();
        });
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
                cache::client()->removeRoom(room_id);
                cache::client()->removeInvite(room_id.toStdString());
        } catch (const lmdb::error &e) {
                nhlog::db()->critical("failure while removing room: {}", e.what());
                // TODO: Notify the user.
        }

        room_list_->removeRoom(room_id, room_id == current_room_);
}

void
ChatPage::updateTypingUsers(const QString &roomid, const std::vector<std::string> &user_ids)
{
        if (!userSettings_->isTypingNotificationsEnabled())
                return;

        typingUsers_[roomid] = generateTypingUsers(roomid, user_ids);

        if (current_room_ == roomid)
                typingDisplay_->setUsers(typingUsers_[roomid]);
}

QStringList
ChatPage::generateTypingUsers(const QString &room_id, const std::vector<std::string> &typing_users)
{
        QStringList users;
        auto local_user = utils::localUser();

        for (const auto &uid : typing_users) {
                const auto remote_user = QString::fromStdString(uid);

                if (remote_user == local_user)
                        continue;

                users.append(Cache::displayName(room_id, remote_user));
        }

        users.sort();

        return users;
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
ChatPage::updateRoomNotificationCount(const QString &room_id, uint16_t notification_count)
{
        room_list_->updateUnreadMessageCount(room_id, notification_count);
}

void
ChatPage::sendDesktopNotifications(const mtx::responses::Notifications &res)
{
        for (const auto &item : res.notifications) {
                const auto event_id = utils::event_id(item.event);

                try {
                        if (item.read) {
                                cache::client()->removeReadNotification(event_id);
                                continue;
                        }

                        if (!cache::client()->isNotificationSent(event_id)) {
                                const auto room_id = QString::fromStdString(item.room_id);
                                const auto user_id = utils::event_sender(item.event);

                                // We should only sent one notification per event.
                                cache::client()->markSentNotification(event_id);

                                // Don't send a notification when the current room is opened.
                                if (isRoomActive(room_id))
                                        continue;

                                notificationsManager.postNotification(
                                  room_id,
                                  QString::fromStdString(event_id),
                                  QString::fromStdString(
                                    cache::client()->singleRoomInfo(item.room_id).name),
                                  Cache::displayName(room_id, user_id),
                                  utils::event_body(item.event),
                                  cache::client()->getRoomAvatar(room_id));
                        }
                } catch (const lmdb::error &e) {
                        nhlog::db()->warn("error while sending desktop notification: {}", e.what());
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
        opts.timeout = 0;
        http::client()->sync(
          opts,
          std::bind(
            &ChatPage::initialSyncHandler, this, std::placeholders::_1, std::placeholders::_2));
}

void
ChatPage::trySync()
{
        mtx::http::SyncOpts opts;

        if (!connectivityTimer_.isActive())
                connectivityTimer_.start();

        try {
                opts.since = cache::client()->nextBatchToken();
        } catch (const lmdb::error &e) {
                nhlog::db()->error("failed to retrieve next batch token: {}", e.what());
                return;
        }

        http::client()->sync(
          opts, [this](const mtx::responses::Sync &res, mtx::http::RequestErr err) {
                  if (err) {
                          const auto error      = QString::fromStdString(err->matrix_error.error);
                          const auto msg        = tr("Please try to login again: %1").arg(error);
                          const auto err_code   = mtx::errors::to_string(err->matrix_error.errcode);
                          const int status_code = static_cast<int>(err->status_code);

                          if (status_code <= 0 || status_code >= 600) {
                                  if (!http::is_logged_in())
                                          return;

                                  emit tryDelayedSyncCb();
                                  return;
                          }

                          nhlog::net()->error("sync error: {} {}", status_code, err_code);

                          switch (status_code) {
                          case 502:
                          case 504:
                          case 524: {
                                  emit trySyncCb();
                                  return;
                          }
                          default: {
                                  if (!http::is_logged_in())
                                          return;

                                  if (err->matrix_error.errcode ==
                                      mtx::errors::ErrorCode::M_UNKNOWN_TOKEN)
                                          emit dropToLoginPageCb(msg);
                                  else
                                          emit tryDelayedSyncCb();

                                  return;
                          }
                          }
                  }

                  nhlog::net()->debug("sync completed: {}", res.next_batch);

                  // Ensure that we have enough one-time keys available.
                  ensureOneTimeKeyCount(res.device_one_time_keys_count);

                  // TODO: fine grained error handling
                  try {
                          cache::client()->saveState(res);
                          olm::handle_to_device_messages(res.to_device);

                          emit syncUI(res.rooms);

                          auto updates = cache::client()->roomUpdates(res);

                          emit syncTopBar(updates);
                          emit syncRoomlist(updates);

                          emit syncTags(cache::client()->roomTagUpdates(res));

                          cache::client()->deleteOldData();
                  } catch (const lmdb::map_full_error &e) {
                          nhlog::db()->error("lmdb is full: {}", e.what());
                          cache::client()->deleteOldData();
                  } catch (const lmdb::error &e) {
                          nhlog::db()->error("saving sync response: {}", e.what());
                  }

                  emit trySyncCb();
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
                            QString("Failed to join room: %1")
                              .arg(QString::fromStdString(err->matrix_error.error)));
                          return;
                  }

                  emit showNotification("You joined the room");

                  // We remove any invites with the same room_id.
                  try {
                          cache::client()->removeInvite(room_id);
                  } catch (const lmdb::error &e) {
                          emit showNotification(
                            QString("Failed to remove invite: %1").arg(e.what()));
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

                  emit showNotification(QString("Room %1 created")
                                          .arg(QString::fromStdString(res.room_id.to_string())));
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
ChatPage::sendTypingNotifications()
{
        if (!userSettings_->isTypingNotificationsEnabled())
                return;

        http::client()->start_typing(
          current_room_.toStdString(), 10'000, [](mtx::http::RequestErr err) {
                  if (err) {
                          nhlog::net()->warn("failed to send typing notification: {}",
                                             err->matrix_error.error);
                  }
          });
}

void
ChatPage::initialSyncHandler(const mtx::responses::Sync &res, mtx::http::RequestErr err)
{
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
                cache::client()->saveState(res);

                olm::handle_to_device_messages(res.to_device);

                emit initializeViews(std::move(res.rooms));
                emit initializeRoomList(cache::client()->roomInfo());

                cache::client()->calculateRoomReadStatus();
                emit syncTags(cache::client()->roomInfo().toStdMap());
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

                  if (cache::client()) {
                          auto data = cache::client()->image(res.avatar_url);
                          if (!data.isNull()) {
                                  emit setUserAvatar(QImage::fromData(data));
                                  return;
                          }
                  }

                  if (res.avatar_url.empty())
                          return;

                  http::client()->download(
                    res.avatar_url,
                    [this, res](const std::string &data,
                                const std::string &,
                                const std::string &,
                                mtx::http::RequestErr err) {
                            if (err) {
                                    nhlog::net()->warn(
                                      "failed to download user avatar: {} - {}",
                                      mtx::errors::to_string(err->matrix_error.errcode),
                                      err->matrix_error.error);
                                    return;
                            }

                            if (cache::client())
                                    cache::client()->saveImage(res.avatar_url, data);

                            emit setUserAvatar(
                              QImage::fromData(QByteArray(data.data(), data.size())));
                    });
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
        communitiesList_->hide();
        sideBar_->hide();
        top_bar_->enableBackButton();
}

void
ChatPage::showSideBars()
{
        if (userSettings_->isGroupViewEnabled())
                communitiesList_->show();

        sideBar_->show();
        top_bar_->disableBackButton();
}

uint64_t
ChatPage::timelineWidth()
{
        int sidebarWidth = sideBar_->size().width();
        sidebarWidth += communitiesList_->size().width();

        return size().width() - sidebarWidth;
}
bool
ChatPage::isSideBarExpanded()
{
        return sideBar_->size().width() > ui::sidebar::NormalSize;
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
