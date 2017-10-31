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
#include "Sync.h"
#include "TextInputWidget.h"
#include "Theme.h"
#include "TimelineViewManager.h"
#include "TopRoomBar.h"
#include "TypingDisplay.h"
#include "UserInfoWidget.h"

constexpr int MAX_INITIAL_SYNC_FAILURES = 5;
constexpr int SYNC_RETRY_TIMEOUT        = 10000;

namespace events = matrix::events;

ChatPage::ChatPage(QSharedPointer<MatrixClient> client, QWidget *parent)
  : QWidget(parent)
  , client_(client)
{
        setStyleSheet("background-color: #fff;");

        topLayout_ = new QHBoxLayout(this);
        topLayout_->setSpacing(0);
        topLayout_->setMargin(0);

        auto splitter = new Splitter(this);
        splitter->setHandleWidth(0);

        topLayout_->addWidget(splitter);

        // SideBar
        sideBar_ = new QWidget(this);
        sideBar_->setMinimumSize(QSize(ui::sidebar::NormalSize, 0));
        sideBarLayout_ = new QVBoxLayout(sideBar_);
        sideBarLayout_->setSpacing(0);
        sideBarLayout_->setMargin(0);

        sideBarTopLayout_ = new QVBoxLayout();
        sideBarTopLayout_->setSpacing(0);
        sideBarTopLayout_->setMargin(0);
        sideBarMainLayout_ = new QVBoxLayout();
        sideBarMainLayout_->setSpacing(0);
        sideBarMainLayout_->setMargin(0);

        sidebarActions_ = new SideBarActions(this);

        sideBarLayout_->addLayout(sideBarTopLayout_);
        sideBarLayout_->addLayout(sideBarMainLayout_);
        sideBarLayout_->addWidget(sidebarActions_);

        sideBarTopWidget_ = new QWidget(sideBar_);
        sideBarTopWidget_->setStyleSheet("background-color: #d6dde3; color: #ebebeb;");

        sideBarTopLayout_->addWidget(sideBarTopWidget_);

        sideBarTopWidgetLayout_ = new QVBoxLayout(sideBarTopWidget_);
        sideBarTopWidgetLayout_->setSpacing(0);
        sideBarTopWidgetLayout_->setMargin(0);

        // Content
        content_       = new QWidget(this);
        contentLayout_ = new QVBoxLayout(content_);
        contentLayout_->setSpacing(0);
        contentLayout_->setMargin(0);

        topBarLayout_ = new QHBoxLayout();
        topBarLayout_->setSpacing(0);
        mainContentLayout_ = new QVBoxLayout();
        mainContentLayout_->setSpacing(0);
        mainContentLayout_->setMargin(0);

        contentLayout_->addLayout(topBarLayout_);
        contentLayout_->addLayout(mainContentLayout_);

        // Splitter
        splitter->addWidget(sideBar_);
        splitter->addWidget(content_);

        room_list_ = new RoomList(client, sideBar_);
        sideBarMainLayout_->addWidget(room_list_);

        top_bar_ = new TopRoomBar(this);
        topBarLayout_->addWidget(top_bar_);

        view_manager_ = new TimelineViewManager(client, this);
        mainContentLayout_->addWidget(view_manager_);

        text_input_    = new TextInputWidget(this);
        typingDisplay_ = new TypingDisplay(this);
        contentLayout_->addWidget(typingDisplay_);
        contentLayout_->addWidget(text_input_);

        typingRefresher_ = new QTimer(this);
        typingRefresher_->setInterval(TYPING_REFRESH_TIMEOUT);

        user_info_widget_ = new UserInfoWidget(sideBarTopWidget_);
        sideBarTopWidgetLayout_->addWidget(user_info_widget_);

        connect(user_info_widget_, SIGNAL(logout()), client_.data(), SLOT(logout()));
        connect(client_.data(), SIGNAL(loggedOut()), this, SLOT(logout()));

        connect(
          top_bar_, &TopRoomBar::leaveRoom, this, [=]() { client_->leaveRoom(current_room_); });

        connect(room_list_, &RoomList::roomChanged, this, [=](const QString &roomid) {
                QStringList users;

                if (typingUsers_.contains(roomid))
                        users = typingUsers_[roomid];

                typingDisplay_->setUsers(users);
        });
        connect(room_list_, &RoomList::roomChanged, text_input_, &TextInputWidget::stopTyping);

        connect(room_list_, &RoomList::roomChanged, this, &ChatPage::changeTopRoomInfo);
        connect(room_list_, &RoomList::roomChanged, text_input_, &TextInputWidget::focusLineEdit);
        connect(
          room_list_, &RoomList::roomChanged, view_manager_, &TimelineViewManager::setHistoryView);

        connect(view_manager_,
                &TimelineViewManager::unreadMessages,
                this,
                [=](const QString &roomid, int count) {
                        if (!settingsManager_.contains(roomid)) {
                                qWarning() << "RoomId does not have settings" << roomid;
                                room_list_->updateUnreadMessageCount(roomid, count);
                                return;
                        }

                        if (settingsManager_[roomid]->isNotificationsEnabled())
                                room_list_->updateUnreadMessageCount(roomid, count);
                });

        connect(text_input_, &TextInputWidget::startedTyping, this, [=]() {
                typingRefresher_->start();
                client_->sendTypingNotification(current_room_);
        });

        connect(text_input_, &TextInputWidget::stoppedTyping, this, [=]() {
                typingRefresher_->stop();
                client_->removeTypingNotification(current_room_);
        });

        connect(typingRefresher_, &QTimer::timeout, this, [=]() {
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
                SLOT(sendTextMessage(const QString &)));

        connect(text_input_,
                SIGNAL(sendEmoteMessage(const QString &)),
                view_manager_,
                SLOT(sendEmoteMessage(const QString &)));

        connect(text_input_,
                &TextInputWidget::sendJoinRoomRequest,
                client_.data(),
                &MatrixClient::joinRoom);

        connect(text_input_, &TextInputWidget::uploadImage, this, [=](QString filename) {
                client_->uploadImage(current_room_, filename);
        });

        connect(client_.data(), &MatrixClient::joinFailed, this, &ChatPage::showNotification);
        connect(client_.data(),
                &MatrixClient::imageUploaded,
                this,
                [=](QString roomid, QString filename, QString url) {
                        text_input_->hideUploadSpinner();
                        view_manager_->sendImageMessage(roomid, filename, url);
                });

        connect(client_.data(),
                SIGNAL(roomAvatarRetrieved(const QString &, const QPixmap &)),
                this,
                SLOT(updateTopBarAvatar(const QString &, const QPixmap &)));

        connect(client_.data(),
                SIGNAL(initialSyncCompleted(const SyncResponse &)),
                this,
                SLOT(initialSyncCompleted(const SyncResponse &)));
        connect(client_.data(), &MatrixClient::initialSyncFailed, this, [=](const QString &msg) {
                if (client_->getHomeServer().isEmpty()) {
                        deleteConfigs();
                        return;
                }

                initialSyncFailures += 1;

                if (initialSyncFailures >= MAX_INITIAL_SYNC_FAILURES) {
                        initialSyncFailures = 0;

                        deleteConfigs();

                        emit showLoginPage(msg);
                        emit contentLoaded();
                        return;
                }

                qWarning() << msg;
                qWarning() << "Retrying initial sync";

                client_->initialSync();
        });
        connect(client_.data(),
                SIGNAL(syncCompleted(const SyncResponse &)),
                this,
                SLOT(syncCompleted(const SyncResponse &)));
        connect(client_.data(),
                SIGNAL(syncFailed(const QString &)),
                this,
                SLOT(syncFailed(const QString &)));
        connect(client_.data(),
                SIGNAL(getOwnProfileResponse(const QUrl &, const QString &)),
                this,
                SLOT(updateOwnProfileInfo(const QUrl &, const QString &)));
        connect(client_.data(),
                SIGNAL(ownAvatarRetrieved(const QPixmap &)),
                this,
                SLOT(setOwnAvatar(const QPixmap &)));
        connect(client_.data(), &MatrixClient::joinedRoom, this, [=]() {
                emit showNotification("You joined the room.");
        });
        connect(client_.data(),
                SIGNAL(leftRoom(const QString &)),
                this,
                SLOT(removeRoom(const QString &)));

        showContentTimer_ = new QTimer(this);
        showContentTimer_->setSingleShot(true);
        connect(showContentTimer_, &QTimer::timeout, this, [=]() {
                consensusTimer_->stop();
                emit contentLoaded();
        });

        consensusTimer_ = new QTimer(this);
        connect(consensusTimer_, &QTimer::timeout, this, [=]() {
                if (view_manager_->hasLoaded()) {
                        // Remove the spinner overlay.
                        emit contentLoaded();
                        showContentTimer_->stop();
                        consensusTimer_->stop();
                }
        });

        AvatarProvider::init(client);
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
        room_avatars_.clear();
        room_list_->clear();
        settingsManager_.clear();
        state_manager_.clear();
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

        cache_ = QSharedPointer<Cache>(new Cache(userid));

        try {
                cache_->setup();

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
}

void
ChatPage::setOwnAvatar(const QPixmap &img)
{
        user_info_widget_->setAvatar(img.toImage());
}

void
ChatPage::syncFailed(const QString &msg)
{
        // Stop if sync is not active. e.g user is logged out.
        if (client_->getHomeServer().isEmpty())
                return;

        qWarning() << "Sync error:" << msg;
        QTimer::singleShot(SYNC_RETRY_TIMEOUT, this, [=]() { client_->sync(); });
}

void
ChatPage::syncCompleted(const SyncResponse &response)
{
        updateJoinedRooms(response.rooms().join());
        removeLeftRooms(response.rooms().leave());

        auto stateDiff = generateMembershipDifference(response.rooms().join(), state_manager_);
        QtConcurrent::run(cache_.data(), &Cache::setState, response.nextBatch(), stateDiff);

        room_list_->sync(state_manager_);
        view_manager_->sync(response.rooms());

        client_->setNextBatchToken(response.nextBatch());
        client_->sync();
}

void
ChatPage::initialSyncCompleted(const SyncResponse &response)
{
        auto joined = response.rooms().join();

        for (auto it = joined.constBegin(); it != joined.constEnd(); ++it) {
                RoomState room_state;

                // Build the current state from the timeline and state events.
                room_state.updateFromEvents(it.value().state().events());
                room_state.updateFromEvents(it.value().timeline().events());

                // Remove redundant memberships.
                room_state.removeLeaveMemberships();

                // Resolve room name and avatar. e.g in case of one-to-one chats.
                room_state.resolveName();
                room_state.resolveAvatar();

                state_manager_.insert(it.key(), room_state);
                settingsManager_.insert(it.key(),
                                        QSharedPointer<RoomSettings>(new RoomSettings(it.key())));

                for (const auto membership : room_state.memberships) {
                        updateUserDisplayName(membership);
                        updateUserAvatarUrl(membership);
                }

                QApplication::processEvents();
        }

        QtConcurrent::run(cache_.data(), &Cache::setState, response.nextBatch(), state_manager_);

        // Populate timelines with messages.
        view_manager_->initialize(response.rooms());

        // Initialize room list.
        room_list_->setInitialRooms(settingsManager_, state_manager_);

        client_->setNextBatchToken(response.nextBatch());
        client_->sync();

        emit contentLoaded();
}

void
ChatPage::updateTopBarAvatar(const QString &roomid, const QPixmap &img)
{
        room_avatars_.insert(roomid, img);

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
                client_->fetchOwnAvatar(avatar_url);
}

void
ChatPage::changeTopRoomInfo(const QString &room_id)
{
        if (!state_manager_.contains(room_id))
                return;

        auto state = state_manager_[room_id];

        top_bar_->updateRoomName(state.getName());
        top_bar_->updateRoomTopic(state.getTopic());
        top_bar_->setRoomSettings(settingsManager_[room_id]);

        if (room_avatars_.contains(room_id))
                top_bar_->updateRoomAvatar(room_avatars_.value(room_id).toImage());
        else
                top_bar_->updateRoomAvatarFromName(state.getName());

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

        // Fetch all the joined room's state.
        auto rooms = cache_->states();

        for (auto it = rooms.constBegin(); it != rooms.constEnd(); ++it) {
                RoomState room_state = it.value();

                // Clean up and prepare state for use.
                room_state.removeLeaveMemberships();
                room_state.resolveName();
                room_state.resolveAvatar();

                // Save the current room state.
                state_manager_.insert(it.key(), room_state);

                // Create or restore the settings for this room.
                settingsManager_.insert(it.key(),
                                        QSharedPointer<RoomSettings>(new RoomSettings(it.key())));

                // Resolve user avatars.
                for (const auto membership : room_state.memberships) {
                        updateUserDisplayName(membership);
                        updateUserAvatarUrl(membership);
                }
        }

        // Initializing empty timelines.
        view_manager_->initialize(rooms.keys());

        // Initialize room list from the restored state and settings.
        room_list_->setInitialRooms(settingsManager_, state_manager_);

        // Check periodically if the timelines have been loaded.
        consensusTimer_->start(CONSENSUS_TIMEOUT);

        // Show the content if consensus can't be achieved.
        showContentTimer_->start(SHOW_CONTENT_TIMEOUT);

        // Start receiving events.
        client_->sync();
}

void
ChatPage::showQuickSwitcher()
{
        if (quickSwitcher_.isNull()) {
                quickSwitcher_ = QSharedPointer<QuickSwitcher>(
                  new QuickSwitcher(this),
                  [=](QuickSwitcher *switcher) { switcher->deleteLater(); });

                connect(quickSwitcher_.data(),
                        &QuickSwitcher::roomSelected,
                        room_list_,
                        &RoomList::highlightSelectedRoom);

                connect(quickSwitcher_.data(), &QuickSwitcher::closing, this, [=]() {
                        if (!this->quickSwitcherModal_.isNull())
                                this->quickSwitcherModal_->fadeOut();
                });
        }

        if (quickSwitcherModal_.isNull()) {
                quickSwitcherModal_ = QSharedPointer<OverlayModal>(
                  new OverlayModal(MainWindow::instance(), quickSwitcher_.data()),
                  [=](OverlayModal *modal) { modal->deleteLater(); });
                quickSwitcherModal_->setDuration(0);
                quickSwitcherModal_->setColor(QColor(30, 30, 30, 170));
        }

        QMap<QString, QString> rooms;

        for (auto it = state_manager_.constBegin(); it != state_manager_.constEnd(); ++it)
                rooms.insert(it.value().getName(), it.key());

        quickSwitcher_->setRoomList(rooms);
        quickSwitcherModal_->fadeIn();
}

void
ChatPage::addRoom(const QString &room_id)
{
        if (!state_manager_.contains(room_id)) {
                RoomState room_state;

                state_manager_.insert(room_id, room_state);
                settingsManager_.insert(room_id,
                                        QSharedPointer<RoomSettings>(new RoomSettings(room_id)));

                room_list_->addRoom(settingsManager_[room_id], state_manager_[room_id], room_id);
                room_list_->highlightSelectedRoom(room_id);

                changeTopRoomInfo(room_id);
        }
}

void
ChatPage::removeRoom(const QString &room_id)
{
        state_manager_.remove(room_id);
        settingsManager_.remove(room_id);
        try {
                cache_->removeRoom(room_id);
        } catch (const lmdb::error &e) {
                qCritical() << "The cache couldn't be updated: " << e.what();
                // TODO: Notify the user.
                cache_->unmount();
                cache_->deleteData();
        }
        room_list_->removeRoom(room_id, room_id == current_room_);
}

void
ChatPage::updateTypingUsers(const QString &roomid, const QList<QString> &user_ids)
{
        QStringList users;

        QSettings settings;
        QString user_id = settings.value("auth/user_id").toString();

        for (const auto uid : user_ids) {
                if (uid == user_id)
                        continue;
                users.append(TimelineViewManager::displayName(uid));
        }

        users.sort();

        if (current_room_ == roomid) {
                typingDisplay_->setUsers(users);
        }

        typingUsers_.insert(roomid, users);
}

void
ChatPage::updateUserMetadata(const QJsonArray &events)
{
        events::EventType ty;

        for (const auto &event : events) {
                try {
                        ty = events::extractEventType(event.toObject());
                } catch (const DeserializationException &e) {
                        qWarning() << e.what() << event;
                        continue;
                }

                if (!events::isStateEvent(ty))
                        continue;

                try {
                        switch (ty) {
                        case events::EventType::RoomMember: {
                                events::StateEvent<events::MemberEventContent> member;
                                member.deserialize(event);

                                updateUserAvatarUrl(member);
                                updateUserDisplayName(member);

                                break;
                        }
                        default: {
                                continue;
                        }
                        }
                } catch (const DeserializationException &e) {
                        qWarning() << e.what() << event;
                        continue;
                }
        }
}

void
ChatPage::updateUserAvatarUrl(const events::StateEvent<events::MemberEventContent> &membership)
{
        auto uid = membership.sender();
        auto url = membership.content().avatarUrl();

        if (!url.toString().isEmpty())
                AvatarProvider::setAvatarUrl(uid, url);
}

void
ChatPage::updateUserDisplayName(const events::StateEvent<events::MemberEventContent> &membership)
{
        auto displayName = membership.content().displayName();

        if (!displayName.isEmpty())
                TimelineViewManager::DISPLAY_NAMES.insert(membership.stateKey(), displayName);
}

void
ChatPage::removeLeftRooms(const QMap<QString, LeftRoom> &rooms)
{
        for (auto it = rooms.constBegin(); it != rooms.constEnd(); ++it) {
                if (state_manager_.contains(it.key()))
                        removeRoom(it.key());
        }
}

void
ChatPage::updateJoinedRooms(const QMap<QString, JoinedRoom> &rooms)
{
        for (auto it = rooms.constBegin(); it != rooms.constEnd(); ++it) {
                updateTypingUsers(it.key(), it.value().typingUserIDs());

                const auto newStateEvents    = it.value().state().events();
                const auto newTimelineEvents = it.value().timeline().events();

                // Merge the new updates for rooms that we are tracking.
                if (state_manager_.contains(it.key())) {
                        auto oldState = &state_manager_[it.key()];
                        oldState->updateFromEvents(newStateEvents);
                        oldState->updateFromEvents(newTimelineEvents);
                        oldState->resolveName();
                        oldState->resolveAvatar();
                } else {
                        // Build the current state from the timeline and state events.
                        RoomState room_state;
                        room_state.updateFromEvents(newStateEvents);
                        room_state.updateFromEvents(newTimelineEvents);

                        // Resolve room name and avatar. e.g in case of one-to-one chats.
                        room_state.resolveName();
                        room_state.resolveAvatar();

                        state_manager_.insert(it.key(), room_state);

                        // TODO Doesn't work on the sidebar.
                        settingsManager_.insert(
                          it.key(), QSharedPointer<RoomSettings>(new RoomSettings(it.key())));

                        view_manager_->addRoom(it.value(), it.key());
                }

                updateUserMetadata(newStateEvents);
                updateUserMetadata(newTimelineEvents);

                if (it.key() == current_room_)
                        changeTopRoomInfo(it.key());

                QApplication::processEvents();
        }
}

QMap<QString, RoomState>
ChatPage::generateMembershipDifference(const QMap<QString, JoinedRoom> &rooms,
                                       const QMap<QString, RoomState> &states) const
{
        QMap<QString, RoomState> stateDiff;

        for (auto it = rooms.constBegin(); it != rooms.constEnd(); ++it) {
                if (!states.contains(it.key()))
                        continue;

                auto events = it.value().state().events();

                for (auto event : it.value().timeline().events())
                        events.append(event);

                RoomState local;
                local.aliases            = states[it.key()].aliases;
                local.avatar             = states[it.key()].avatar;
                local.canonical_alias    = states[it.key()].canonical_alias;
                local.history_visibility = states[it.key()].history_visibility;
                local.join_rules         = states[it.key()].join_rules;
                local.name               = states[it.key()].name;
                local.power_levels       = states[it.key()].power_levels;
                local.topic              = states[it.key()].topic;
                local.memberships        = getMemberships(events);

                stateDiff.insert(it.key(), local);
        }

        return stateDiff;
}

using Memberships = QMap<QString, matrix::events::StateEvent<events::MemberEventContent>>;

Memberships
ChatPage::getMemberships(const QJsonArray &events) const
{
        Memberships memberships;

        events::EventType ty;

        for (const auto &event : events) {
                try {
                        ty = events::extractEventType(event.toObject());
                } catch (const DeserializationException &e) {
                        qWarning() << e.what() << event;
                        continue;
                }

                if (!events::isStateEvent(ty))
                        continue;

                try {
                        switch (ty) {
                        case events::EventType::RoomMember: {
                                events::StateEvent<events::MemberEventContent> member;
                                member.deserialize(event);
                                memberships.insert(member.stateKey(), member);
                                break;
                        }
                        default: {
                                continue;
                        }
                        }
                } catch (const DeserializationException &e) {
                        qWarning() << e.what() << event;
                        continue;
                }
        }

        return memberships;
}

ChatPage::~ChatPage() {}
