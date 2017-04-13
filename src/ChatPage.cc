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

#include "ui_ChatPage.h"

#include <QDebug>
#include <QLabel>
#include <QSettings>

#include "ChatPage.h"
#include "Sync.h"
#include "UserInfoWidget.h"

ChatPage::ChatPage(QSharedPointer<MatrixClient> client, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ChatPage)
    , sync_interval_(2000)
    , client_(client)
{
	ui->setupUi(this);

	room_list_ = new RoomList(client, this);
	ui->sideBarMainLayout->addWidget(room_list_);

	top_bar_ = new TopRoomBar(this);
	ui->topBarLayout->addWidget(top_bar_);

	view_manager_ = new HistoryViewManager(client, this);
	ui->mainContentLayout->addWidget(view_manager_);

	text_input_ = new TextInputWidget(this);
	ui->contentLayout->addWidget(text_input_);

	user_info_widget_ = new UserInfoWidget(ui->sideBarTopWidget);
	ui->sideBarTopUserInfoLayout->addWidget(user_info_widget_);

	sync_timer_ = new QTimer(this);
	sync_timer_->setSingleShot(true);
	connect(sync_timer_, SIGNAL(timeout()), this, SLOT(startSync()));

	connect(user_info_widget_, SIGNAL(logout()), client_.data(), SLOT(logout()));
	connect(client_.data(), SIGNAL(loggedOut()), this, SLOT(logout()));

	connect(room_list_,
		SIGNAL(roomChanged(const RoomInfo &)),
		this,
		SLOT(changeTopRoomInfo(const RoomInfo &)));
	connect(room_list_,
		SIGNAL(roomChanged(const RoomInfo &)),
		view_manager_,
		SLOT(setHistoryView(const RoomInfo &)));

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
}

void ChatPage::logout()
{
	sync_timer_->stop();

	QSettings settings;
	settings.remove("auth/access_token");
	settings.remove("auth/home_server");
	settings.remove("auth/user_id");
	settings.remove("client/transaction_id");

	// Clear the environment.
	room_list_->clear();
	view_manager_->clearAll();

	top_bar_->reset();
	user_info_widget_->reset();
	client_->reset();

	room_avatars_.clear();

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

void ChatPage::syncCompleted(const SyncResponse &response)
{
	client_->setNextBatchToken(response.nextBatch());

	/* room_list_->sync(response.rooms()); */
	view_manager_->sync(response.rooms());

	sync_timer_->start(sync_interval_);
}

void ChatPage::initialSyncCompleted(const SyncResponse &response)
{
	if (!response.nextBatch().isEmpty())
		client_->setNextBatchToken(response.nextBatch());

	view_manager_->initialize(response.rooms());
	room_list_->setInitialRooms(response.rooms());

	sync_timer_->start(sync_interval_);
}

void ChatPage::updateTopBarAvatar(const QString &roomid, const QPixmap &img)
{
	room_avatars_.insert(roomid, img);

	if (current_room_.id() != roomid)
		return;

	top_bar_->updateRoomAvatar(img.toImage());
}

void ChatPage::updateOwnProfileInfo(const QUrl &avatar_url, const QString &display_name)
{
	QSettings settings;
	auto userid = settings.value("auth/user_id").toString();

	user_info_widget_->setUserId(userid);
	user_info_widget_->setDisplayName(display_name);

	client_->fetchOwnAvatar(avatar_url);
}

void ChatPage::changeTopRoomInfo(const RoomInfo &info)
{
	top_bar_->updateRoomName(info.name());
	top_bar_->updateRoomTopic(info.topic());

	if (room_avatars_.contains(info.id()))
		top_bar_->updateRoomAvatar(room_avatars_.value(info.id()).toImage());
	else
		top_bar_->updateRoomAvatarFromName(info.name());

	current_room_ = info;
}

ChatPage::~ChatPage()
{
	sync_timer_->stop();
	delete ui;
}
