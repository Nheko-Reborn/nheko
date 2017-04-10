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
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QWidget>

#include "ChatPage.h"
#include "Sync.h"
#include "UserInfoWidget.h"

ChatPage::ChatPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ChatPage)
    , sync_interval_(2000)
{
	ui->setupUi(this);
	matrix_client_ = new MatrixClient("matrix.org", parent);
	content_downloader_ = new QNetworkAccessManager(parent);

	room_list_ = new RoomList(this);

	top_bar_ = new TopRoomBar(this);
	ui->topBarLayout->addWidget(top_bar_);

	view_manager_ = new HistoryViewManager(this);
	ui->mainContentLayout->addWidget(view_manager_);

	text_input_ = new TextInputWidget(this);
	ui->contentLayout->addWidget(text_input_);

	user_info_widget_ = new UserInfoWidget(ui->sideBarTopWidget);

	sync_timer_ = new QTimer(this);
	connect(sync_timer_, SIGNAL(timeout()), this, SLOT(startSync()));

	connect(user_info_widget_, SIGNAL(logout()), matrix_client_, SLOT(logout()));
	connect(matrix_client_, SIGNAL(loggedOut()), this, SLOT(logout()));

	connect(room_list_,
		SIGNAL(roomChanged(const RoomInfo &)),
		this,
		SLOT(changeTopRoomInfo(const RoomInfo &)));

	connect(room_list_,
		SIGNAL(roomChanged(const RoomInfo &)),
		view_manager_,
		SLOT(setHistoryView(const RoomInfo &)));

	connect(room_list_,
		SIGNAL(fetchRoomAvatar(const QString &, const QUrl &)),
		this,
		SLOT(fetchRoomAvatar(const QString &, const QUrl &)));

	connect(text_input_,
		SIGNAL(sendTextMessage(const QString &)),
		this,
		SLOT(sendTextMessage(const QString &)));

	ui->sideBarTopUserInfoLayout->addWidget(user_info_widget_);
	ui->sideBarMainLayout->addWidget(room_list_);

	connect(matrix_client_,
		SIGNAL(initialSyncCompleted(SyncResponse)),
		this,
		SLOT(initialSyncCompleted(SyncResponse)));
	connect(matrix_client_,
		SIGNAL(syncCompleted(SyncResponse)),
		this,
		SLOT(syncCompleted(SyncResponse)));
	connect(matrix_client_,
		SIGNAL(getOwnProfileResponse(QUrl, QString)),
		this,
		SLOT(updateOwnProfileInfo(QUrl, QString)));
	connect(matrix_client_,
		SIGNAL(messageSent(QString, int)),
		this,
		SLOT(messageSent(QString, int)));
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
	matrix_client_->reset();

	room_avatars_.clear();

	emit close();
}

void ChatPage::messageSent(QString event_id, int txn_id)
{
	Q_UNUSED(event_id);

	QSettings settings;
	settings.setValue("client/transaction_id", txn_id + 1);
}

void ChatPage::sendTextMessage(const QString &msg)
{
	auto room = current_room_;
	matrix_client_->sendTextMessage(current_room_.id(), msg);
}

void ChatPage::bootstrap(QString userid, QString homeserver, QString token)
{
	Q_UNUSED(userid);

	matrix_client_->setServer(homeserver);
	matrix_client_->setAccessToken(token);

	matrix_client_->getOwnProfile();
	matrix_client_->initialSync();
}

void ChatPage::startSync()
{
	matrix_client_->sync();
}

void ChatPage::setOwnAvatar(const QByteArray &img)
{
	if (img.size() == 0)
		return;

	QPixmap pixmap;
	pixmap.loadFromData(img);
	user_info_widget_->setAvatar(pixmap.toImage());
}

void ChatPage::syncCompleted(const SyncResponse &response)
{
	matrix_client_->setNextBatchToken(response.nextBatch());

	/* room_list_->sync(response.rooms()); */
	view_manager_->sync(response.rooms());
}

void ChatPage::initialSyncCompleted(const SyncResponse &response)
{
	if (!response.nextBatch().isEmpty())
		matrix_client_->setNextBatchToken(response.nextBatch());

	view_manager_->initialize(response.rooms());
	room_list_->setInitialRooms(response.rooms());

	sync_timer_->start(sync_interval_);
}

// TODO: This function should be part of the matrix client for generic media retrieval.
void ChatPage::fetchRoomAvatar(const QString &roomid, const QUrl &avatar_url)
{
	// TODO: move this into a Utils function
	QList<QString> url_parts = avatar_url.toString().split("mxc://");

	if (url_parts.size() != 2) {
		qDebug() << "Invalid format for room avatar " << avatar_url.toString();
		return;
	}

	QString media_params = url_parts[1];
	QString media_url = QString("%1/_matrix/media/r0/download/%2")
				    .arg(matrix_client_->getHomeServer(), media_params);

	QNetworkRequest avatar_request(media_url);
	QNetworkReply *reply = content_downloader_->get(avatar_request);
	reply->setProperty("media_params", media_params);

	connect(reply, &QNetworkReply::finished, [this, media_params, roomid, reply]() {
		reply->deleteLater();

		auto media = reply->property("media_params").toString();

		if (media != media_params)
			return;

		int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

		if (status == 0) {
			qDebug() << reply->errorString();
			return;
		}

		if (status >= 400) {
			qWarning() << "Request " << reply->request().url() << " returned " << status;
			return;
		}

		auto img = reply->readAll();

		if (img.size() == 0)
			return;

		QPixmap pixmap;
		pixmap.loadFromData(img);
		room_avatars_.insert(roomid, pixmap);

		this->room_list_->updateRoomAvatar(roomid, pixmap.toImage());

		if (current_room_.id() == roomid) {
			QIcon icon(pixmap);
			this->top_bar_->updateRoomAvatar(icon);
		}
	});
}

void ChatPage::updateOwnProfileInfo(const QUrl &avatar_url, const QString &display_name)
{
	QSettings settings;
	auto userid = settings.value("auth/user_id").toString();

	user_info_widget_->setUserId(userid);
	user_info_widget_->setDisplayName(display_name);

	// TODO: move this into a Utils function
	QList<QString> url_parts = avatar_url.toString().split("mxc://");

	if (url_parts.size() != 2) {
		qDebug() << "Invalid format for media " << avatar_url.toString();
		return;
	}

	QString media_params = url_parts[1];
	QString media_url = QString("%1/_matrix/media/r0/download/%2")
				    .arg(matrix_client_->getHomeServer(), media_params);

	QNetworkRequest avatar_request(media_url);
	QNetworkReply *reply = content_downloader_->get(avatar_request);
	reply->setProperty("media_params", media_params);

	connect(reply, &QNetworkReply::finished, [this, media_params, reply]() {
		reply->deleteLater();

		auto media = reply->property("media_params").toString();

		if (media != media_params)
			return;

		int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

		if (status == 0) {
			qDebug() << reply->errorString();
			return;
		}

		if (status >= 400) {
			qWarning() << "Request " << reply->request().url() << " returned " << status;
			return;
		}

		setOwnAvatar(reply->readAll());
	});
}

void ChatPage::changeTopRoomInfo(const RoomInfo &info)
{
	top_bar_->updateRoomName(info.name());
	top_bar_->updateRoomTopic(info.topic());

	if (room_avatars_.contains(info.id())) {
		QIcon icon(room_avatars_.value(info.id()));
		top_bar_->updateRoomAvatar(icon);
	} else {
		top_bar_->updateRoomAvatarFromName(info.name());
	}

	current_room_ = info;
}

ChatPage::~ChatPage()
{
	sync_timer_->stop();
	delete ui;
}
