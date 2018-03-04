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
#include "RoomSettings.h"
#include "RoomState.h"
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

        sideBarTopLayout_ = new QVBoxLayout();
        sideBarTopLayout_->setSpacing(0);
        sideBarTopLayout_->setMargin(0);
        sideBarMainLayout_ = new QVBoxLayout();
        sideBarMainLayout_->setSpacing(0);
        sideBarMainLayout_->setMargin(0);

        sideBarLayout_->addLayout(sideBarTopLayout_);
        sideBarLayout_->addLayout(sideBarMainLayout_);

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

        connect(room_list_, &RoomList::acceptInvite, client_.data(), &MatrixClient::joinRoom);
        connect(room_list_, &RoomList::declineInvite, client_.data(), &MatrixClient::leaveRoom);

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
                removeInvite(room_id);
        });
        connect(client_.data(), &MatrixClient::invitedUser, this, [this](QString, QString user) {
                emit showNotification(QString("Invited user %1").arg(user));
        });
        connect(client_.data(), &MatrixClient::roomCreated, this, [this](QString room_id) {
                emit showNotification(QString("Room %1 created").arg(room_id));
        });
        connect(client_.data(), &MatrixClient::leftRoom, this, &ChatPage::removeRoom);

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

        AvatarProvider::init(client);

        instance_ = this;
}

void
ChatPage::logout()
{
        deleteConfigs();

        resetUI();

        emit close();
}

void
ChatPage::resetUI()
{
        roomAvatars_.clear();
        room_list_->clear();
        roomSettings_.clear();
        roomStates_.clear();
        top_bar_->reset();
        user_info_widget_->reset();
        view_manager_->clearAll();
        AvatarProvider::clear();

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
                cache_->unmount();
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

        updateJoinedRooms(response.rooms.join);
        removeLeftRooms(response.rooms.leave);

        const auto nextBatchToken = QString::fromStdString(response.next_batch);

        auto stateDiff = generateMembershipDifference(response.rooms.join, roomStates_);
        QtConcurrent::run(cache_.data(), &Cache::setState, nextBatchToken, stateDiff);
        QtConcurrent::run(cache_.data(), &Cache::setInvites, response.rooms.invite);

        room_list_->sync(roomStates_, roomSettings_);
        room_list_->syncInvites(response.rooms.invite);

        view_manager_->sync(response.rooms);

        client_->setNextBatchToken(nextBatchToken);
        client_->sync();

        syncTimeoutTimer_->start(SYNC_RETRY_TIMEOUT);
}

void
ChatPage::initialSyncCompleted(const mtx::responses::Sync &response)
{
        initialSyncTimer_->stop();

        auto joined = response.rooms.join;

        for (auto it = joined.cbegin(); it != joined.cend(); ++it) {
                auto roomState = QSharedPointer<RoomState>(new RoomState);

                // Build the current state from the timeline and state events.
                roomState->updateFromEvents(it->second.state.events);
                roomState->updateFromEvents(it->second.timeline.events);

                // Remove redundant memberships.
                roomState->removeLeaveMemberships();

                // Resolve room name and avatar. e.g in case of one-to-one chats.
                roomState->resolveName();
                roomState->resolveAvatar();

                const auto room_id = QString::fromStdString(it->first);

                roomStates_.emplace(room_id, roomState);
                roomSettings_.emplace(room_id,
                                      QSharedPointer<RoomSettings>(new RoomSettings(room_id)));

                for (const auto &membership : roomState->memberships) {
                        updateUserDisplayName(membership.second);
                        updateUserAvatarUrl(membership.second);
                }

                QApplication::processEvents();
        }

        QtConcurrent::run(cache_.data(),
                          &Cache::setState,
                          QString::fromStdString(response.next_batch),
                          roomStates_);
        QtConcurrent::run(cache_.data(), &Cache::setInvites, response.rooms.invite);

        // Create timelines
        view_manager_->initialize(response.rooms);

        // Initialize room list.
        room_list_->setInitialRooms(roomSettings_, roomStates_);
        room_list_->syncInvites(response.rooms.invite);

        client_->setNextBatchToken(QString::fromStdString(response.next_batch));
        client_->sync();

        // Add messages
        view_manager_->sync(response.rooms);

        emit contentLoaded();
}

void
ChatPage::updateTopBarAvatar(const QString &roomid, const QPixmap &img)
{
        roomAvatars_.emplace(roomid, img);

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

        if (avatar_url.isValid())
                client_->fetchUserAvatar(
                  avatar_url,
                  [this](QImage img) { user_info_widget_->setAvatar(img); },
                  [](QString error) { qWarning() << error << ": failed to fetch own avatar"; });
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
        if (roomStates_.find(room_id) == roomStates_.end())
                return;

        auto state = roomStates_[room_id];

        top_bar_->updateRoomName(state->getName());
        top_bar_->updateRoomTopic(state->getTopic());
        top_bar_->setRoomSettings(roomSettings_[room_id]);

        if (roomAvatars_.find(room_id) != roomAvatars_.end())
                top_bar_->updateRoomAvatar(roomAvatars_[room_id].toImage());
        else
                top_bar_->updateRoomAvatarFromName(state->getName());

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
        qDebug() << "Restoring state from cache";

        qDebug() << "Restored nextBatchToken" << cache_->nextBatchToken();
        client_->setNextBatchToken(cache_->nextBatchToken());

        qRegisterMetaType<std::map<QString, RoomState>>();

        QtConcurrent::run(cache_.data(), &Cache::states);

        connect(
          cache_.data(), &Cache::statesLoaded, this, [this](std::map<QString, RoomState> rooms) {
                  qDebug() << "Cache data loaded";

                  std::vector<QString> roomKeys;

                  for (auto const &room : rooms) {
                          auto roomState = QSharedPointer<RoomState>(new RoomState(room.second));

                          // Clean up and prepare state for use.
                          roomState->removeLeaveMemberships();
                          roomState->resolveName();
                          roomState->resolveAvatar();

                          // Save the current room state.
                          roomStates_.emplace(room.first, roomState);

                          // Create or restore the settings for this room.
                          roomSettings_.emplace(
                            room.first, QSharedPointer<RoomSettings>(new RoomSettings(room.first)));

                          // Resolve user avatars.
                          for (auto const &membership : roomState->memberships) {
                                  updateUserDisplayName(membership.second);
                                  updateUserAvatarUrl(membership.second);
                          }

                          roomKeys.emplace_back(room.first);
                  }

                  // Initializing empty timelines.
                  view_manager_->initialize(roomKeys);

                  // Initialize room list from the restored state and settings.
                  room_list_->setInitialRooms(roomSettings_, roomStates_);
                  room_list_->syncInvites(cache_->invites());

                  // Check periodically if the timelines have been loaded.
                  consensusTimer_->start(CONSENSUS_TIMEOUT);

                  // Show the content if consensus can't be achieved.
                  showContentTimer_->start(SHOW_CONTENT_TIMEOUT);

                  // Start receiving events.
                  client_->sync();
          });
}

void
ChatPage::showQuickSwitcher()
{
        if (quickSwitcher_.isNull()) {
                quickSwitcher_ = QSharedPointer<QuickSwitcher>(
                  new QuickSwitcher(this),
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

        std::map<QString, QString> rooms;

        for (auto const &state : roomStates_) {
                QString deambiguator =
                  QString::fromStdString(state.second->canonical_alias.content.alias);
                if (deambiguator == "")
                        deambiguator = state.first;
                rooms.emplace(state.second->getName() + " (" + deambiguator + ")", state.first);
        }

        quickSwitcher_->setRoomList(rooms);
        quickSwitcherModal_->show();
}

void
ChatPage::addRoom(const QString &room_id)
{
        if (roomStates_.find(room_id) == roomStates_.end()) {
                auto room_state = QSharedPointer<RoomState>(new RoomState);

                roomStates_.emplace(room_id, room_state);
                roomSettings_.emplace(room_id,
                                      QSharedPointer<RoomSettings>(new RoomSettings(room_id)));

                room_list_->addRoom(roomSettings_[room_id], roomStates_[room_id], room_id);
                room_list_->highlightSelectedRoom(room_id);

                changeTopRoomInfo(room_id);
        }
}

void
ChatPage::removeRoom(const QString &room_id)
{
        roomStates_.erase(room_id);
        roomSettings_.erase(room_id);

        try {
                cache_->removeRoom(room_id);
                cache_->removeInvite(room_id);
        } catch (const lmdb::error &e) {
                qCritical() << "The cache couldn't be updated: " << e.what();
                // TODO: Notify the user.
                cache_->unmount();
                cache_->deleteData();
        }
        room_list_->removeRoom(room_id, room_id == current_room_);
}

void
ChatPage::removeInvite(const QString &room_id)
{
        try {
                cache_->removeInvite(room_id);
        } catch (const lmdb::error &e) {
                qCritical() << "The cache couldn't be updated: " << e.what();
                // TODO: Notify the user.
                cache_->unmount();
                cache_->deleteData();
        }

        room_list_->removeRoom(room_id, room_id == current_room_);
}

void
ChatPage::updateTypingUsers(const QString &roomid, const std::vector<std::string> &user_ids)
{
        if (!userSettings_->isTypingNotificationsEnabled())
                return;

        QStringList users;

        QSettings settings;
        QString user_id = settings.value("auth/user_id").toString();

        for (const auto &uid : user_ids) {
                auto user = QString::fromStdString(uid);

                if (user == user_id)
                        continue;

                users.append(TimelineViewManager::displayName(user));
        }

        users.sort();

        if (current_room_ == roomid) {
                typingDisplay_->setUsers(users);
        }

        typingUsers_.emplace(roomid, users);
}

void
ChatPage::updateUserAvatarUrl(const mtx::events::StateEvent<mtx::events::state::Member> &membership)
{
        auto uid = QString::fromStdString(membership.state_key);
        auto url = QString::fromStdString(membership.content.avatar_url);

        if (!url.isEmpty())
                AvatarProvider::setAvatarUrl(uid, url);
}

void
ChatPage::updateUserDisplayName(
  const mtx::events::StateEvent<mtx::events::state::Member> &membership)
{
        auto displayName = QString::fromStdString(membership.content.display_name);
        auto stateKey    = QString::fromStdString(membership.state_key);

        if (!displayName.isEmpty())
                TimelineViewManager::DISPLAY_NAMES.emplace(stateKey, displayName);
}

void
ChatPage::removeLeftRooms(const std::map<std::string, mtx::responses::LeftRoom> &rooms)
{
        for (auto it = rooms.cbegin(); it != rooms.cend(); ++it) {
                const auto room_id = QString::fromStdString(it->first);

                if (roomStates_.find(room_id) != roomStates_.end())
                        removeRoom(room_id);
        }
}

void
ChatPage::updateJoinedRooms(const std::map<std::string, mtx::responses::JoinedRoom> &rooms)
{
        for (auto it = rooms.cbegin(); it != rooms.cend(); ++it) {
                const auto roomid = QString::fromStdString(it->first);

                updateTypingUsers(roomid, it->second.ephemeral.typing);
                updateRoomNotificationCount(roomid,
                                            it->second.unread_notifications.notification_count);

                if (it->second.ephemeral.receipts.size() > 0)
                        QtConcurrent::run(cache_.data(),
                                          &Cache::updateReadReceipt,
                                          it->first,
                                          it->second.ephemeral.receipts);

                const auto newStateEvents    = it->second.state;
                const auto newTimelineEvents = it->second.timeline;

                // Merge the new updates for rooms that we are tracking.
                if (roomStates_.find(roomid) != roomStates_.end()) {
                        auto oldState = roomStates_[roomid];
                        oldState->updateFromEvents(newStateEvents.events);
                        oldState->updateFromEvents(newTimelineEvents.events);
                        oldState->resolveName();
                        oldState->resolveAvatar();
                } else {
                        // Build the current state from the timeline and state events.
                        auto roomState = QSharedPointer<RoomState>(new RoomState);
                        roomState->updateFromEvents(newStateEvents.events);
                        roomState->updateFromEvents(newTimelineEvents.events);

                        // Resolve room name and avatar. e.g in case of one-to-one chats.
                        roomState->resolveName();
                        roomState->resolveAvatar();

                        roomStates_.emplace(roomid, roomState);

                        roomSettings_.emplace(
                          roomid, QSharedPointer<RoomSettings>(new RoomSettings(roomid)));

                        view_manager_->addRoom(it->second, roomid);
                }

                updateUserMetadata(newStateEvents.events);
                updateUserMetadata(newTimelineEvents.events);

                if (roomid == current_room_)
                        changeTopRoomInfo(roomid);

                QApplication::processEvents();
        }
}

std::map<QString, QSharedPointer<RoomState>>
ChatPage::generateMembershipDifference(
  const std::map<std::string, mtx::responses::JoinedRoom> &rooms,
  const std::map<QString, QSharedPointer<RoomState>> &states) const
{
        std::map<QString, QSharedPointer<RoomState>> stateDiff;

        for (auto it = rooms.cbegin(); it != rooms.cend(); ++it) {
                const auto room_id = QString::fromStdString(it->first);

                if (states.find(room_id) == states.end())
                        continue;

                auto all_memberships     = getMemberships(it->second.state.events);
                auto timelineMemberships = getMemberships(it->second.timeline.events);

                // We have to process first the state events and then the timeline.
                for (auto mm = timelineMemberships.cbegin(); mm != timelineMemberships.cend(); ++mm)
                        all_memberships.emplace(mm->first, mm->second);

                auto local                = QSharedPointer<RoomState>(new RoomState);
                local->aliases            = states.at(room_id)->aliases;
                local->avatar             = states.at(room_id)->avatar;
                local->canonical_alias    = states.at(room_id)->canonical_alias;
                local->history_visibility = states.at(room_id)->history_visibility;
                local->join_rules         = states.at(room_id)->join_rules;
                local->name               = states.at(room_id)->name;
                local->power_levels       = states.at(room_id)->power_levels;
                local->topic              = states.at(room_id)->topic;
                local->memberships        = all_memberships;

                stateDiff.emplace(room_id, local);
        }

        return stateDiff;
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
