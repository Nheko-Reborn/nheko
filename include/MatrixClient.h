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

#ifndef MATRIXCLIENT_H
#define MATRIXCLIENT_H

#include <QtNetwork/QNetworkAccessManager>

#include "Profile.h"
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
	~MatrixClient();

	// Client API.
	void initialSync();
	void sync();
	void sendTextMessage(QString roomid, QString msg);
	void login(const QString &username, const QString &password);
	void registerUser(const QString &username, const QString &password);
	void versions();

	inline QString getHomeServer();
	inline void incrementTransactionId();

public slots:
	// Profile
	void getOwnProfile();

	inline void setServer(QString server);
	inline void setAccessToken(QString token);
	inline void setNextBatchToken(const QString &next_batch);

signals:
	// Emitted after a error during the login.
	void loginError(QString error);

	// Emitted after succesfull user login. A new access token is returned by the server.
	void loginSuccess(QString user_id, QString home_server, QString token);

	// Returned profile data for the user's account.
	void getOwnProfileResponse(QUrl avatar_url, QString display_name);
	void initialSyncCompleted(SyncResponse response);
	void syncCompleted(SyncResponse response);
	void messageSent(QString event_id, int txn_id);

private slots:
	void onResponse(QNetworkReply *reply);

private:
	enum Endpoint {
		GetOwnProfile,
		GetProfile,
		InitialSync,
		Login,
		Register,
		SendTextMessage,
		Sync,
		Versions,
	};

	// Response handlers.
	void onLoginResponse(QNetworkReply *reply);
	void onRegisterResponse(QNetworkReply *reply);
	void onVersionsResponse(QNetworkReply *reply);
	void onGetOwnProfileResponse(QNetworkReply *reply);
	void onSendTextMessageResponse(QNetworkReply *reply);
	void onInitialSyncResponse(QNetworkReply *reply);
	void onSyncResponse(QNetworkReply *reply);

	// Client API prefix.
	QString api_url_;

	// The Matrix server used for communication.
	QString server_;

	// The access token used for authentication.
	QString token_;

	// Increasing transaction ID.
	int txn_id_;

	// Token to be used for the next sync.
	QString next_batch_;
};

inline QString MatrixClient::getHomeServer()
{
	return server_;
}

inline void MatrixClient::setServer(QString server)
{
	server_ = "https://" + server;
}

inline void MatrixClient::setAccessToken(QString token)
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

#endif  // MATRIXCLIENT_H
