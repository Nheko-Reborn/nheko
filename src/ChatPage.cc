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

#include <QDebug>
#include <QSettings>

#include "ChatPage.h"
#include "Splitter.h"
#include "Sync.h"
#include "Theme.h"
#include "TimelineViewManager.h"
#include "UserInfoWidget.h"

#include "AliasesEventContent.h"
#include "AvatarEventContent.h"
#include "AvatarProvider.h"
#include "CanonicalAliasEventContent.h"
#include "CreateEventContent.h"
#include "HistoryVisibilityEventContent.h"
#include "JoinRulesEventContent.h"
#include "MemberEventContent.h"
#include "NameEventContent.h"
#include "PowerLevelsEventContent.h"
#include "TopicEventContent.h"

#include "StateEvent.h"

namespace events = matrix::events;

ChatPage::ChatPage(QSharedPointer<MatrixClient> client, QWidget *parent)
    : QWidget(parent)
    , sync_interval_(2000)
    , client_(client)
{
	resize(798, 519);
	setStyleSheet("background-color: #f8fbfe;");

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

	sideBarLayout_->addLayout(sideBarTopLayout_);
	sideBarLayout_->addLayout(sideBarMainLayout_);

	sideBarTopWidget_ = new QWidget(sideBar_);
	sideBarTopWidget_->setStyleSheet("background-color: #d6dde3; color: #ebebeb;");

	sideBarTopLayout_->addWidget(sideBarTopWidget_);

	sideBarTopWidgetLayout_ = new QVBoxLayout(sideBarTopWidget_);
	sideBarTopWidgetLayout_->setSpacing(0);
	sideBarTopWidgetLayout_->setMargin(0);

	// Content
	content_ = new QWidget(this);
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

	room_list_ = new RoomList(client, this);
	sideBarMainLayout_->addWidget(room_list_);

	top_bar_ = new TopRoomBar(this);
	topBarLayout_->addWidget(top_bar_);

	view_manager_ = new TimelineViewManager(client, this);
	mainContentLayout_->addWidget(view_manager_);

	text_input_ = new TextInputWidget(this);
	contentLayout_->addWidget(text_input_);

	user_info_widget_ = new UserInfoWidget(sideBarTopWidget_);
	sideBarTopWidgetLayout_->addWidget(user_info_widget_);

	sync_timer_ = new QTimer(this);
	sync_timer_->setSingleShot(true);
	connect(sync_timer_, SIGNAL(timeout()), this, SLOT(startSync()));

	connect(user_info_widget_, SIGNAL(logout()), client_.data(), SLOT(logout()));
	connect(client_.data(), SIGNAL(loggedOut()), this, SLOT(logout()));

	connect(room_list_, &RoomList::roomChanged, this, &ChatPage::changeTopRoomInfo);
	connect(room_list_, &RoomList::roomChanged, view_manager_, &TimelineViewManager::setHistoryView);

	connect(view_manager_, &TimelineViewManager::unreadMessages, this, [=](const QString &roomid, int count) {
		if (!settingsManager_.contains(roomid)) {
			qWarning() << "RoomId does not have settings" << roomid;
			room_list_->updateUnreadMessageCount(roomid, count);
			return;
		}

		if (settingsManager_[roomid]->isNotificationsEnabled())
			room_list_->updateUnreadMessageCount(roomid, count);
	});

	connect(room_list_,
		SIGNAL(totalUnreadMessageCountUpdated(int)),
		this,
		SLOT(showUnreadMessageNotification(int)));

	connect(text_input_,
		SIGNAL(sendTextMessage(const QString &)),
		view_manager_,
		SLOT(sendTextMessage(const QString &)));

	connect(client_.data(),
		SIGNAL(roomAvatarRetrieved(const QString &, const QPixmap &)),
		this,
		SLOT(updateTopBarAvatar(const QString &, const QPixmap &)));

	connect(client_.data(),
		SIGNAL(initialSyncCompleted(const SyncResponse &)),
		this,
		SLOT(initialSyncCompleted(const SyncResponse &)));
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

	AvatarProvider::init(client);
}

void ChatPage::logout()
{
	sync_timer_->stop();

	// Delete all config parameters.
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

	// Clear the environment.
	room_list_->clear();
	view_manager_->clearAll();

	top_bar_->reset();
	user_info_widget_->reset();
	client_->reset();

	state_manager_.clear();
	settingsManager_.clear();
	room_avatars_.clear();

	AvatarProvider::clear();

	emit close();
}

void ChatPage::bootstrap(QString userid, QString homeserver, QString token)
{
	Q_UNUSED(userid);

	client_->setServer(homeserver);
	client_->setAccessToken(token);

	client_->getOwnProfile();
	client_->initialSync();
}

void ChatPage::startSync()
{
	client_->sync();
}

void ChatPage::setOwnAvatar(const QPixmap &img)
{
	user_info_widget_->setAvatar(img.toImage());
}

void ChatPage::syncFailed(const QString &msg)
{
	qWarning() << "Sync error:" << msg;
	sync_timer_->start(sync_interval_ * 5);
}

// TODO: Should be moved in another class that manages this global list.
void ChatPage::updateDisplayNames(const RoomState &state)
{
	for (const auto member : state.memberships) {
		auto displayName = member.content().displayName();

		if (!displayName.isEmpty())
			TimelineViewManager::DISPLAY_NAMES.insert(member.stateKey(), displayName);
	}
}

void ChatPage::syncCompleted(const SyncResponse &response)
{
	client_->setNextBatchToken(response.nextBatch());

	auto joined = response.rooms().join();

	for (auto it = joined.constBegin(); it != joined.constEnd(); it++) {
		RoomState room_state;

		if (state_manager_.contains(it.key()))
			room_state = state_manager_[it.key()];

		updateRoomState(room_state, it.value().state().events());
		updateRoomState(room_state, it.value().timeline().events());
		updateDisplayNames(room_state);

		if (state_manager_.contains(it.key())) {
			// TODO: Use pointers instead of copying.
			auto oldState = state_manager_[it.key()];
			oldState.update(room_state);
			state_manager_.insert(it.key(), oldState);
		} else {
			qWarning() << "New rooms cannot be added after initial sync, yet.";
		}

		if (it.key() == current_room_)
			changeTopRoomInfo(it.key());
	}

	room_list_->sync(state_manager_);
	view_manager_->sync(response.rooms());

	sync_timer_->start(sync_interval_);
}

void ChatPage::initialSyncCompleted(const SyncResponse &response)
{
	if (!response.nextBatch().isEmpty())
		client_->setNextBatchToken(response.nextBatch());

	auto joined = response.rooms().join();

	for (auto it = joined.constBegin(); it != joined.constEnd(); it++) {
		RoomState room_state;

		updateRoomState(room_state, it.value().state().events());
		updateRoomState(room_state, it.value().timeline().events());

		room_state.removeLeaveMemberships();
		room_state.resolveName();
		room_state.resolveAvatar();

		updateDisplayNames(room_state);

		state_manager_.insert(it.key(), room_state);
		settingsManager_.insert(it.key(), QSharedPointer<RoomSettings>(new RoomSettings(it.key())));

		for (const auto membership : room_state.memberships) {
			auto uid = membership.sender();
			auto url = membership.content().avatarUrl();

			if (!url.toString().isEmpty())
				AvatarProvider::setAvatarUrl(uid, url);
		}
	}

	view_manager_->initialize(response.rooms());
	room_list_->setInitialRooms(settingsManager_, state_manager_);

	sync_timer_->start(sync_interval_);
}

void ChatPage::updateTopBarAvatar(const QString &roomid, const QPixmap &img)
{
	room_avatars_.insert(roomid, img);

	if (current_room_ != roomid)
		return;

	top_bar_->updateRoomAvatar(img.toImage());
}

void ChatPage::updateOwnProfileInfo(const QUrl &avatar_url, const QString &display_name)
{
	QSettings settings;
	auto userid = settings.value("auth/user_id").toString();

	user_info_widget_->setUserId(userid);
	user_info_widget_->setDisplayName(display_name);

	if (avatar_url.isValid())
		client_->fetchOwnAvatar(avatar_url);
}

void ChatPage::changeTopRoomInfo(const QString &room_id)
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

void ChatPage::showUnreadMessageNotification(int count)
{
	emit unreadMessages(count);

	// TODO: Make the default title a const.
	if (count == 0)
		emit changeWindowTitle("nheko");
	else
		emit changeWindowTitle(QString("nheko (%1)").arg(count));
}

void ChatPage::updateRoomState(RoomState &room_state, const QJsonArray &events)
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
			case events::EventType::RoomAliases: {
				events::StateEvent<events::AliasesEventContent> aliases;
				aliases.deserialize(event);
				room_state.aliases = aliases;
				break;
			}
			case events::EventType::RoomAvatar: {
				events::StateEvent<events::AvatarEventContent> avatar;
				avatar.deserialize(event);
				room_state.avatar = avatar;
				break;
			}
			case events::EventType::RoomCanonicalAlias: {
				events::StateEvent<events::CanonicalAliasEventContent> canonical_alias;
				canonical_alias.deserialize(event);
				room_state.canonical_alias = canonical_alias;
				break;
			}
			case events::EventType::RoomCreate: {
				events::StateEvent<events::CreateEventContent> create;
				create.deserialize(event);
				room_state.create = create;
				break;
			}
			case events::EventType::RoomHistoryVisibility: {
				events::StateEvent<events::HistoryVisibilityEventContent> history_visibility;
				history_visibility.deserialize(event);
				room_state.history_visibility = history_visibility;
				break;
			}
			case events::EventType::RoomJoinRules: {
				events::StateEvent<events::JoinRulesEventContent> join_rules;
				join_rules.deserialize(event);
				room_state.join_rules = join_rules;
				break;
			}
			case events::EventType::RoomName: {
				events::StateEvent<events::NameEventContent> name;
				name.deserialize(event);
				room_state.name = name;
				break;
			}
			case events::EventType::RoomMember: {
				events::StateEvent<events::MemberEventContent> member;
				member.deserialize(event);

				room_state.memberships.insert(member.stateKey(), member);

				break;
			}
			case events::EventType::RoomPowerLevels: {
				events::StateEvent<events::PowerLevelsEventContent> power_levels;
				power_levels.deserialize(event);
				room_state.power_levels = power_levels;
				break;
			}
			case events::EventType::RoomTopic: {
				events::StateEvent<events::TopicEventContent> topic;
				topic.deserialize(event);
				room_state.topic = topic;
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

ChatPage::~ChatPage()
{
	sync_timer_->stop();
}
