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
#include <QDebug>
#include <QSettings>
#include <QtConcurrent>

#include "AvatarProvider.h"
#include "Cache.h"
#include "ChatPage.h"
#include "MainWindow.h"
#include "MatrixClient.h"
#include "OverlayModal.h"
#include "QuickSwitcher.h"
#include "RoomList.h"
#include "SideBarActions.h"
#include "Splitter.h"
#include "TextInputWidget.h"
#include "Theme.h"
#include "TopRoomBar.h"
#include "TypingDisplay.h"
#include "UserInfoWidget.h"
#include "UserSettingsPage.h"

#include "dialogs/ReadReceipts.h"
#include "timeline/TimelineViewManager.h"

constexpr int SYNC_RETRY_TIMEOUT         = 40 * 1000;
constexpr int INITIAL_SYNC_RETRY_TIMEOUT = 240 * 1000;

ChatPage *ChatPage::instance_ = nullptr;

ChatPage::ChatPage(QSharedPointer<MatrixClient> client,
                   QSharedPointer<UserSettings> userSettings,
                   QWidget *parent)
  : QWidget(parent)
  , client_(client)
  , userSettings_{userSettings}
{
        setObjectName("chatPage");

        topLayout_ = new QHBoxLayout(this);
        topLayout_->setSpacing(0);
        topLayout_->setMargin(0);

        communitiesSideBar_ = new QWidget(this);
        communitiesSideBar_->setFixedWidth(ui::sidebar::CommunitiesSidebarSize);
        communitiesSideBarLayout_ = new QVBoxLayout(communitiesSideBar_);
        communitiesSideBarLayout_->setSpacing(0);
        communitiesSideBarLayout_->setMargin(0);

        communitiesList_ = new CommunitiesList(client, this);
        communitiesSideBarLayout_->addWidget(communitiesList_);
        // communitiesSideBarLayout_->addStretch(1);
        topLayout_->addWidget(communitiesSideBar_);

        auto splitter = new Splitter(this);
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
        connect(
          sidebarActions_, &SideBarActions::joinRoom, client_.data(), &MatrixClient::joinRoom);
        connect(
          sidebarActions_, &SideBarActions::createRoom, client_.data(), &MatrixClient::createRoom);

        user_info_widget_ = new UserInfoWidget(sideBar_);
        room_list_        = new RoomList(client, userSettings_, sideBar_);

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
        view_manager_ = new TimelineViewManager(client, this);

        contentLayout_->addWidget(top_bar_);
        contentLayout_->addWidget(view_manager_);

        // Splitter
        splitter->addWidget(sideBar_);
        splitter->addWidget(content_);
        splitter->setSizes({ui::sidebar::NormalSize, parent->width() - ui::sidebar::NormalSize});

        text_input_    = new TextInputWidget(this);
        typingDisplay_ = new TypingDisplay(this);
        contentLayout_->addWidget(typingDisplay_);
        contentLayout_->addWidget(text_input_);

        typingRefresher_ = new QTimer(this);
        typingRefresher_->setInterval(TYPING_REFRESH_TIMEOUT);

        connect(user_info_widget_, &UserInfoWidget::logout, this, [this]() {
                client_->logout();
                emit showOverlayProgressBar();
        });
        connect(client_.data(), &MatrixClient::loggedOut, this, &ChatPage::logout);

        connect(top_bar_, &TopRoomBar::inviteUsers, this, [this](QStringList users) {
                for (int ii = 0; ii < users.size(); ++ii) {
                        QTimer::singleShot(ii * 1000, this, [this, ii, users]() {
                                client_->inviteUser(current_room_, users.at(ii));
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
        connect(room_list_, &RoomList::roomChanged, text_input_, &TextInputWidget::focusLineEdit);
        connect(
          room_list_, &RoomList::roomChanged, view_manager_, &TimelineViewManager::setHistoryView);

        connect(room_list_, &RoomList::acceptInvite, this, [this](const QString &room_id) {
                view_manager_->addRoom(room_id);
                client_->joinRoom(room_id);
                room_list_->removeRoom(room_id, currentRoom() == room_id);
        });

        connect(room_list_, &RoomList::declineInvite, this, [this](const QString &room_id) {
                client_->leaveRoom(room_id);
                room_list_->removeRoom(room_id, currentRoom() == room_id);
        });

        connect(text_input_, &TextInputWidget::startedTyping, this, [this]() {
                if (!userSettings_->isTypingNotificationsEnabled())
                        return;

                typingRefresher_->start();
                client_->sendTypingNotification(current_room_);
        });

        connect(text_input_, &TextInputWidget::stoppedTyping, this, [this]() {
                if (!userSettings_->isTypingNotificationsEnabled())
                        return;

                typingRefresher_->stop();
                client_->removeTypingNotification(current_room_);
        });

        connect(typingRefresher_, &QTimer::timeout, this, [this]() {
                if (!userSettings_->isTypingNotificationsEnabled())
                        return;

                client_->sendTypingNotification(current_room_);
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

        connect(text_input_,
                &TextInputWidget::sendJoinRoomRequest,
                client_.data(),
                &MatrixClient::joinRoom);

        connect(text_input_,
                &TextInputWidget::uploadImage,
                this,
                [this](QSharedPointer<QIODevice> data, const QString &fn) {
                        client_->uploadImage(current_room_, fn, data);
                });

        connect(text_input_,
                &TextInputWidget::uploadFile,
                this,
                [this](QSharedPointer<QIODevice> data, const QString &fn) {
                        client_->uploadFile(current_room_, fn, data);
                });

        connect(text_input_,
                &TextInputWidget::uploadAudio,
                this,
                [this](QSharedPointer<QIODevice> data, const QString &fn) {
                        client_->uploadAudio(current_room_, fn, data);
                });
        connect(text_input_,
                &TextInputWidget::uploadVideo,
                this,
                [this](QSharedPointer<QIODevice> data, const QString &fn) {
                        client_->uploadVideo(current_room_, fn, data);
                });

        connect(
          client_.data(), &MatrixClient::roomCreationFailed, this, &ChatPage::showNotification);
        connect(client_.data(), &MatrixClient::joinFailed, this, &ChatPage::showNotification);
        connect(client_.data(), &MatrixClient::uploadFailed, this, [this](int, const QString &msg) {
                text_input_->hideUploadSpinner();
                emit showNotification(msg);
        });
        connect(
          client_.data(),
          &MatrixClient::imageUploaded,
          this,
          [this](QString roomid, QString filename, QString url, QString mime, uint64_t dsize) {
                  text_input_->hideUploadSpinner();
                  view_manager_->queueImageMessage(roomid, filename, url, mime, dsize);
          });
        connect(
          client_.data(),
          &MatrixClient::fileUploaded,
          this,
          [this](QString roomid, QString filename, QString url, QString mime, uint64_t dsize) {
                  text_input_->hideUploadSpinner();
                  view_manager_->queueFileMessage(roomid, filename, url, mime, dsize);
          });
        connect(
          client_.data(),
          &MatrixClient::audioUploaded,
          this,
          [this](QString roomid, QString filename, QString url, QString mime, uint64_t dsize) {
                  text_input_->hideUploadSpinner();
                  view_manager_->queueAudioMessage(roomid, filename, url, mime, dsize);
          });
        connect(
          client_.data(),
          &MatrixClient::videoUploaded,
          this,
          [this](QString roomid, QString filename, QString url, QString mime, uint64_t dsize) {
                  text_input_->hideUploadSpinner();
                  view_manager_->queueVideoMessage(roomid, filename, url, mime, dsize);
          });

        connect(room_list_, &RoomList::roomAvatarChanged, this, &ChatPage::updateTopBarAvatar);

        connect(client_.data(),
                &MatrixClient::initialSyncCompleted,
                this,
                &ChatPage::initialSyncCompleted);
        connect(
          client_.data(), &MatrixClient::initialSyncFailed, this, &ChatPage::retryInitialSync);
        connect(client_.data(), &MatrixClient::syncCompleted, this, &ChatPage::syncCompleted);
        connect(client_.data(),
                &MatrixClient::getOwnProfileResponse,
                this,
                &ChatPage::updateOwnProfileInfo);
        connect(client_.data(),
                SIGNAL(getOwnCommunitiesResponse(QList<QString>)),
                this,
                SLOT(updateOwnCommunitiesInfo(QList<QString>)));
        connect(client_.data(),
                &MatrixClient::communityProfileRetrieved,
                this,
                [this](QString communityId, QJsonObject profile) {
                        communities_[communityId]->parseProfile(profile);
                });
        connect(client_.data(),
                &MatrixClient::communityRoomsRetrieved,
                this,
                [this](QString communityId, QJsonObject rooms) {
                        communities_[communityId]->parseRooms(rooms);

                        if (communityId == current_community_) {
                                if (communityId == "world") {
                                        room_list_->setFilterRooms(false);
                                } else {
                                        room_list_->setRoomFilter(
                                          communities_[communityId]->getRoomList());
                                }
                        }
                });

        connect(client_.data(), &MatrixClient::joinedRoom, this, [this](const QString &room_id) {
                emit showNotification("You joined the room.");

                // We remove any invites with the same room_id.
                try {
                        cache_->removeInvite(room_id.toStdString());
                } catch (const lmdb::error &e) {
                        emit showNotification(QString("Failed to remove invite: %1")
                                                .arg(QString::fromStdString(e.what())));
                }
        });
        connect(client_.data(), &MatrixClient::leftRoom, this, &ChatPage::removeRoom);
        connect(client_.data(), &MatrixClient::invitedUser, this, [this](QString, QString user) {
                emit showNotification(QString("Invited user %1").arg(user));
        });
        connect(client_.data(), &MatrixClient::roomCreated, this, [this](QString room_id) {
                emit showNotification(QString("Room %1 created").arg(room_id));
        });
        connect(client_.data(), &MatrixClient::redactionFailed, this, [this](const QString &error) {
                emit showNotification(QString("Message redaction failed: %1").arg(error));
        });

        showContentTimer_ = new QTimer(this);
        showContentTimer_->setSingleShot(true);
        connect(showContentTimer_, &QTimer::timeout, this, [this]() {
                consensusTimer_->stop();
                emit contentLoaded();
        });

        consensusTimer_ = new QTimer(this);
        connect(consensusTimer_, &QTimer::timeout, this, [this]() {
                if (view_manager_->hasLoaded()) {
                        // Remove the spinner overlay.
                        emit contentLoaded();
                        showContentTimer_->stop();
                        consensusTimer_->stop();
                }
        });

        initialSyncTimer_ = new QTimer(this);
        connect(initialSyncTimer_, &QTimer::timeout, this, [this]() { retryInitialSync(); });

        syncTimeoutTimer_ = new QTimer(this);
        connect(syncTimeoutTimer_, &QTimer::timeout, this, [this]() {
                if (client_->getHomeServer().isEmpty()) {
                        syncTimeoutTimer_->stop();
                        return;
                }

                qDebug() << "Sync took too long. Retrying...";
                client_->sync();
        });

        connect(communitiesList_,
                &CommunitiesList::communityChanged,
                this,
                [this](const QString &communityId) {
                        current_community_ = communityId;

                        if (communityId == "world")
                                room_list_->setFilterRooms(false);
                        else
                                room_list_->setRoomFilter(communities_[communityId]->getRoomList());
                });

        setGroupViewState(userSettings_->isGroupViewEnabled());

        connect(userSettings_.data(),
                &UserSettings::groupViewStateChanged,
                this,
                &ChatPage::setGroupViewState);

        connect(this, &ChatPage::continueSync, this, [this](const QString &next_batch) {
                syncTimeoutTimer_->start(SYNC_RETRY_TIMEOUT);
                client_->setNextBatchToken(next_batch);
                client_->sync();
        });

        connect(this, &ChatPage::startConsesusTimer, this, [this]() {
                consensusTimer_->start(CONSENSUS_TIMEOUT);
                showContentTimer_->start(SHOW_CONTENT_TIMEOUT);
        });
        connect(this, &ChatPage::initializeRoomList, room_list_, &RoomList::initialize);
        connect(this,
                &ChatPage::initializeViews,
                view_manager_,
                [this](const mtx::responses::Rooms &rooms) { view_manager_->initialize(rooms); });
        connect(
          this,
          &ChatPage::initializeEmptyViews,
          this,
          [this](const std::vector<std::string> &rooms) { view_manager_->initialize(rooms); });
        connect(this, &ChatPage::syncUI, this, [this](const mtx::responses::Rooms &rooms) {
                try {
                        room_list_->cleanupInvites(cache_->invites());
                } catch (const lmdb::error &e) {
                        qWarning() << "failed to retrieve invites" << e.what();
                }

                view_manager_->initialize(rooms);
                removeLeftRooms(rooms.leave);

                for (const auto &room : rooms.join) {
                        auto room_id = QString::fromStdString(room.first);

                        updateTypingUsers(room_id, room.second.ephemeral.typing);
                        updateRoomNotificationCount(
                          room_id, room.second.unread_notifications.notification_count);
                }
        });
        connect(this, &ChatPage::syncRoomlist, room_list_, &RoomList::sync);

        instance_ = this;

        qRegisterMetaType<std::map<QString, RoomInfo>>();
        qRegisterMetaType<QMap<QString, RoomInfo>>();
        qRegisterMetaType<mtx::responses::Rooms>();
        qRegisterMetaType<std::vector<std::string>>();
}

void
ChatPage::logout()
{
        deleteConfigs();

        resetUI();

        emit closing();
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

        cache_->deleteData();

        client_->reset();
}

void
ChatPage::bootstrap(QString userid, QString homeserver, QString token)
{
        client_->setServer(homeserver);
        client_->setAccessToken(token);
        client_->getOwnProfile();
        client_->getOwnCommunities();

        cache_ = QSharedPointer<Cache>(new Cache(userid));
        room_list_->setCache(cache_);
        text_input_->setCache(cache_);

        AvatarProvider::init(client_, cache_);

        try {
                cache_->setup();

                if (!cache_->isFormatValid()) {
                        cache_->deleteData();
                        cache_->setup();
                        cache_->setCurrentFormat();
                }

                if (cache_->isInitialized()) {
                        loadStateFromCache();
                        return;
                }
        } catch (const lmdb::error &e) {
                qCritical() << "Cache failure" << e.what();
                cache_->deleteData();
                qInfo() << "Falling back to initial sync ...";
        }

        client_->initialSync();

        initialSyncTimer_->start(INITIAL_SYNC_RETRY_TIMEOUT);
}

void
ChatPage::syncCompleted(const mtx::responses::Sync &response)
{
        syncTimeoutTimer_->stop();

        QtConcurrent::run([this, res = std::move(response)]() {
                try {
                        cache_->saveState(res);
                        emit syncUI(res.rooms);
                        emit syncRoomlist(cache_->roomUpdates(res));
                } catch (const lmdb::error &e) {
                        std::cout << "save cache error:" << e.what() << '\n';
                        // TODO: retry sync.
                        return;
                }

                emit continueSync(cache_->nextBatchToken());
        });
}

void
ChatPage::initialSyncCompleted(const mtx::responses::Sync &response)
{
        initialSyncTimer_->stop();

        qDebug() << "initial sync completed";

        QtConcurrent::run([this, res = std::move(response)]() {
                try {
                        cache_->saveState(res);
                        emit initializeViews(std::move(res.rooms));
                        emit initializeRoomList(cache_->roomInfo());
                } catch (const lmdb::error &e) {
                        qWarning() << "cache error:" << QString::fromStdString(e.what());
                        emit retryInitialSync();
                        return;
                }

                emit continueSync(cache_->nextBatchToken());
                emit contentLoaded();
        });
}

void
ChatPage::updateTopBarAvatar(const QString &roomid, const QPixmap &img)
{
        if (current_room_ != roomid)
                return;

        top_bar_->updateRoomAvatar(img.toImage());
}

void
ChatPage::updateOwnProfileInfo(const QUrl &avatar_url, const QString &display_name)
{
        QSettings settings;
        auto userid = settings.value("auth/user_id").toString();

        user_info_widget_->setUserId(userid);
        user_info_widget_->setDisplayName(display_name);

        if (!avatar_url.isValid())
                return;

        if (!cache_.isNull()) {
                auto data = cache_->image(avatar_url.toString());
                if (!data.isNull()) {
                        user_info_widget_->setAvatar(QImage::fromData(data));
                        return;
                }
        }

        auto proxy = client_->fetchUserAvatar(avatar_url);

        if (proxy.isNull())
                return;

        proxy->setParent(this);
        connect(proxy.data(),
                &DownloadMediaProxy::avatarDownloaded,
                this,
                [this, proxy](const QImage &img) {
                        proxy->deleteLater();
                        user_info_widget_->setAvatar(img);
                });
}

void
ChatPage::updateOwnCommunitiesInfo(const QList<QString> &own_communities)
{
        for (int i = 0; i < own_communities.size(); i++) {
                QSharedPointer<Community> community = QSharedPointer<Community>(new Community());

                communities_[own_communities[i]] = community;
        }

        communitiesList_->setCommunities(communities_);
}

void
ChatPage::changeTopRoomInfo(const QString &room_id)
{
        if (room_id.isEmpty()) {
                qWarning() << "can't switch to empty room_id";
                return;
        }

        try {
                auto room_info = cache_->getRoomInfo({room_id.toStdString()});

                if (room_info.find(room_id) == room_info.end())
                        return;

                const auto name       = QString::fromStdString(room_info[room_id].name);
                const auto avatar_url = QString::fromStdString(room_info[room_id].avatar_url);

                top_bar_->updateRoomName(name);
                top_bar_->updateRoomTopic(QString::fromStdString(room_info[room_id].topic));

                auto img = cache_->getRoomAvatar(room_id);

                if (img.isNull())
                        top_bar_->updateRoomAvatarFromName(name);
                else
                        top_bar_->updateRoomAvatar(img);

        } catch (const lmdb::error &e) {
                qWarning() << "failed to change top bar room info" << e.what();
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
        qDebug() << "restoring state from cache";

        QtConcurrent::run([this]() {
                try {
                        cache_->populateMembers();

                        emit initializeEmptyViews(cache_->joinedRooms());
                        emit initializeRoomList(cache_->roomInfo());
                } catch (const lmdb::error &e) {
                        std::cout << "load cache error:" << e.what() << '\n';
                        // TODO Clear cache and restart.
                        return;
                }

                // Start receiving events.
                emit continueSync(cache_->nextBatchToken());

                // Check periodically if the timelines have been loaded.
                emit startConsesusTimer();
        });
}

void
ChatPage::showQuickSwitcher()
{
        if (quickSwitcher_.isNull()) {
                quickSwitcher_ = QSharedPointer<QuickSwitcher>(
                  new QuickSwitcher(cache_, this),
                  [](QuickSwitcher *switcher) { switcher->deleteLater(); });

                connect(quickSwitcher_.data(),
                        &QuickSwitcher::roomSelected,
                        room_list_,
                        &RoomList::highlightSelectedRoom);

                connect(quickSwitcher_.data(), &QuickSwitcher::closing, this, [this]() {
                        if (!quickSwitcherModal_.isNull())
                                quickSwitcherModal_->hide();
                        text_input_->setFocus(Qt::FocusReason::PopupFocusReason);
                });
        }

        if (quickSwitcherModal_.isNull()) {
                quickSwitcherModal_ = QSharedPointer<OverlayModal>(
                  new OverlayModal(MainWindow::instance(), quickSwitcher_.data()),
                  [](OverlayModal *modal) { modal->deleteLater(); });
                quickSwitcherModal_->setColor(QColor(30, 30, 30, 170));
        }

        quickSwitcherModal_->show();
}

void
ChatPage::removeRoom(const QString &room_id)
{
        try {
                cache_->removeRoom(room_id);
                cache_->removeInvite(room_id.toStdString());
        } catch (const lmdb::error &e) {
                qCritical() << "The cache couldn't be updated: " << e.what();
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

        QSettings settings;
        QString local_user = settings.value("auth/user_id").toString();

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
ChatPage::showReadReceipts(const QString &event_id)
{
        if (receiptsDialog_.isNull()) {
                receiptsDialog_ = QSharedPointer<dialogs::ReadReceipts>(
                  new dialogs::ReadReceipts(this),
                  [](dialogs::ReadReceipts *dialog) { dialog->deleteLater(); });
        }

        if (receiptsModal_.isNull()) {
                receiptsModal_ = QSharedPointer<OverlayModal>(
                  new OverlayModal(MainWindow::instance(), receiptsDialog_.data()),
                  [](OverlayModal *modal) { modal->deleteLater(); });
                receiptsModal_->setColor(QColor(30, 30, 30, 170));
        }

        receiptsDialog_->addUsers(cache_->readReceipts(event_id, current_room_));
        receiptsModal_->show();
}

void
ChatPage::setGroupViewState(bool isEnabled)
{
        if (!isEnabled) {
                communitiesList_->communityChanged("world");
                communitiesSideBar_->hide();

                return;
        }

        communitiesSideBar_->show();
}

void
ChatPage::retryInitialSync(int status_code)
{
        initialSyncTimer_->stop();

        if (client_->getHomeServer().isEmpty()) {
                deleteConfigs();
                resetUI();
                emit showLoginPage("Sync error. Please try again.");
                return;
        }

        // Retry on Bad-Gateway & Gateway-Timeout errors
        if (status_code == -1 || status_code == 504 || status_code == 502 || status_code == 524) {
                qWarning() << "retrying initial sync";

                client_->initialSync();
                initialSyncTimer_->start(INITIAL_SYNC_RETRY_TIMEOUT);
        } else {
                // Drop into the login screen.
                deleteConfigs();
                resetUI();

                emit showLoginPage(QString("Sync error %1. Please try again.").arg(status_code));
        }
}

void
ChatPage::updateRoomNotificationCount(const QString &room_id, uint16_t notification_count)
{
        room_list_->updateUnreadMessageCount(room_id, notification_count);
}
