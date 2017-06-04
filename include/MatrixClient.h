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

#pragma once

#include <QtNetwork/QNetworkAccessManager>

#include "Profile.h"
#include "RoomMessages.h"
#include "Sync.h"

/*
 * MatrixClient provides the high level API to communicate with
 * a Matrix homeserver. All the responses are returned through signals.
 */
class MatrixClient : public QNetworkAccessManager
{
	Q_OBJECT
public:
	MatrixClient(QString server, QObject *parent = 0);

	// Client API.
	void initialSync() noexcept;
	void sync() noexcept;
	void sendTextMessage(const QString &roomid, const QString &msg) noexcept;
	void login(const QString &username, const QString &password) noexcept;
	void registerUser(const QString &username, const QString &password, const QString &server) noexcept;
	void versions() noexcept;
	void fetchRoomAvatar(const QString &roomid, const QUrl &avatar_url);
	void fetchUserAvatar(const QString &userId, const QUrl &avatarUrl);
	void fetchOwnAvatar(const QUrl &avatar_url);
	void downloadImage(const QString &event_id, const QUrl &url);
	void messages(const QString &room_id, const QString &from_token) noexcept;

	inline QUrl getHomeServer();
	inline int transactionId();
	inline void incrementTransactionId();

	void reset() noexcept;

public slots:
	void getOwnProfile() noexcept;
	void logout() noexcept;

	inline void setServer(const QString &server);
	inline void setAccessToken(const QString &token);
	inline void setNextBatchToken(const QString &next_batch);

signals:
	void loginError(const QString &error);
	void registerError(const QString &error);

	void loggedOut();

	void loginSuccess(const QString &userid, const QString &homeserver, const QString &token);
	void registerSuccess(const QString &userid, const QString &homeserver, const QString &token);

	void roomAvatarRetrieved(const QString &roomid, const QPixmap &img);
	void userAvatarRetrieved(const QString &userId, const QImage &img);
	void ownAvatarRetrieved(const QPixmap &img);
	void imageDownloaded(const QString &event_id, const QPixmap &img);

	// Returned profile data for the user's account.
	void getOwnProfileResponse(const QUrl &avatar_url, const QString &display_name);
	void initialSyncCompleted(const SyncResponse &response);
	void syncCompleted(const SyncResponse &response);
	void syncFailed(const QString &msg);
	void messageSent(const QString &event_id, const QString &roomid, const int txn_id);
	void messagesRetrieved(const QString &room_id, const RoomMessages &msgs);

private slots:
	void onResponse(QNetworkReply *reply);

private:
	enum class Endpoint {
		GetOwnAvatar,
		GetOwnProfile,
		GetProfile,
		Image,
		InitialSync,
		Login,
		Logout,
		Messages,
		Register,
		RoomAvatar,
		UserAvatar,
		SendTextMessage,
		Sync,
		Versions,
	};

	// Response handlers.
	void onLoginResponse(QNetworkReply *reply);
	void onLogoutResponse(QNetworkReply *reply);
	void onRegisterResponse(QNetworkReply *reply);
	void onVersionsResponse(QNetworkReply *reply);
	void onGetOwnProfileResponse(QNetworkReply *reply);
	void onGetOwnAvatarResponse(QNetworkReply *reply);
	void onSendTextMessageResponse(QNetworkReply *reply);
	void onInitialSyncResponse(QNetworkReply *reply);
	void onSyncResponse(QNetworkReply *reply);
	void onRoomAvatarResponse(QNetworkReply *reply);
	void onUserAvatarResponse(QNetworkReply *reply);
	void onImageResponse(QNetworkReply *reply);
	void onMessagesResponse(QNetworkReply *reply);

	// Client API prefix.
	QString api_url_;

	// The Matrix server used for communication.
	QUrl server_;

	// The access token used for authentication.
	QString token_;

	// Increasing transaction ID.
	int txn_id_;

	// Token to be used for the next sync.
	QString next_batch_;
};

inline QUrl MatrixClient::getHomeServer()
{
	return server_;
}

inline int MatrixClient::transactionId()
{
	return txn_id_;
}

inline void MatrixClient::setServer(const QString &server)
{
	server_ = QUrl(QString("https://%1").arg(server));
}

inline void MatrixClient::setAccessToken(const QString &token)
{
	token_ = token;
}

inline void MatrixClient::setNextBatchToken(const QString &next_batch)
{
	next_batch_ = next_batch;
}

inline void MatrixClient::incrementTransactionId()
{
	txn_id_ += 1;
}
