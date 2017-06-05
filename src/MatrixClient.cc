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
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPixmap>
#include <QSettings>
#include <QUrl>
#include <QUrlQuery>

#include "Login.h"
#include "MatrixClient.h"
#include "Profile.h"
#include "Register.h"

MatrixClient::MatrixClient(QString server, QObject *parent)
    : QNetworkAccessManager(parent)
{
	server_ = "https://" + server;
	api_url_ = "/_matrix/client/r0";
	token_ = "";

	QSettings settings;
	txn_id_ = settings.value("client/transaction_id", 1).toInt();

	connect(this, SIGNAL(finished(QNetworkReply *)), this, SLOT(onResponse(QNetworkReply *)));
}

void MatrixClient::reset() noexcept
{
	next_batch_ = "";
	server_ = "";
	token_ = "";

	txn_id_ = 0;
}

void MatrixClient::onVersionsResponse(QNetworkReply *reply)
{
	reply->deleteLater();

	qDebug() << "Handling the versions response";

	auto data = reply->readAll();
	auto json = QJsonDocument::fromJson(data);

	qDebug() << json;
}

void MatrixClient::onLoginResponse(QNetworkReply *reply)
{
	reply->deleteLater();

	int status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

	if (status_code == 403) {
		emit loginError(tr("Wrong username or password"));
		return;
	}

	if (status_code == 404) {
		emit loginError(tr("Login endpoint was not found on the server"));
		return;
	}

	if (status_code >= 400) {
		qWarning() << "Login error: " << reply->errorString();
		emit loginError(tr("An unknown error occured. Please try again."));
		return;
	}

	auto data = reply->readAll();
	auto json = QJsonDocument::fromJson(data);

	LoginResponse response;

	try {
		response.deserialize(json);
		emit loginSuccess(response.getUserId(), server_.host(), response.getAccessToken());
	} catch (DeserializationException &e) {
		qWarning() << "Malformed JSON response" << e.what();
		emit loginError(tr("Malformed response. Possibly not a Matrix server"));
	}
}

void MatrixClient::onLogoutResponse(QNetworkReply *reply)
{
	reply->deleteLater();

	int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

	if (status != 200) {
		qWarning() << "Logout error: " << reply->errorString();
		return;
	}

	emit loggedOut();
}

void MatrixClient::onRegisterResponse(QNetworkReply *reply)
{
	reply->deleteLater();

	int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

	auto data = reply->readAll();
	auto json = QJsonDocument::fromJson(data);

	if (status == 0 || status >= 400) {
		if (json.isObject() && json.object().contains("error"))
			emit registerError(json.object().value("error").toString());
		else
			emit registerError(reply->errorString());

		return;
	}

	RegisterResponse response;

	try {
		response.deserialize(json);
		emit registerSuccess(response.getUserId(),
				     response.getHomeServer(),
				     response.getAccessToken());
	} catch (DeserializationException &e) {
		qWarning() << "Register" << e.what();
		emit registerError("Received malformed response.");
	}
}

void MatrixClient::onGetOwnProfileResponse(QNetworkReply *reply)
{
	reply->deleteLater();

	int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

	if (status >= 400) {
		qWarning() << reply->errorString();
		return;
	}

	auto data = reply->readAll();
	auto json = QJsonDocument::fromJson(data);

	ProfileResponse response;

	try {
		response.deserialize(json);
		emit getOwnProfileResponse(response.getAvatarUrl(), response.getDisplayName());
	} catch (DeserializationException &e) {
		qWarning() << "Profile:" << e.what();
	}
}

void MatrixClient::onInitialSyncResponse(QNetworkReply *reply)
{
	reply->deleteLater();

	int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

	if (status == 0 || status >= 400) {
		qWarning() << reply->errorString();
		return;
	}

	auto data = reply->readAll();

	if (data.isEmpty())
		return;

	auto json = QJsonDocument::fromJson(data);

	SyncResponse response;

	try {
		response.deserialize(json);
	} catch (DeserializationException &e) {
		qWarning() << "Sync malformed response" << e.what();
		return;
	}

	emit initialSyncCompleted(response);
}

void MatrixClient::onSyncResponse(QNetworkReply *reply)
{
	reply->deleteLater();

	int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

	if (status == 0 || status >= 400) {
		emit syncFailed(reply->errorString());
		return;
	}

	auto data = reply->readAll();

	if (data.isEmpty())
		return;

	auto json = QJsonDocument::fromJson(data);

	SyncResponse response;

	try {
		response.deserialize(json);
		emit syncCompleted(response);
	} catch (DeserializationException &e) {
		qWarning() << "Sync malformed response" << e.what();
	}
}

void MatrixClient::onSendTextMessageResponse(QNetworkReply *reply)
{
	reply->deleteLater();

	int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

	if (status == 0 || status >= 400) {
		qWarning() << reply->errorString();
		return;
	}

	auto data = reply->readAll();

	if (data.isEmpty())
		return;

	auto json = QJsonDocument::fromJson(data);

	if (!json.isObject()) {
		qDebug() << "Send message response is not a JSON object";
		return;
	}

	auto object = json.object();

	if (!object.contains("event_id")) {
		qDebug() << "SendTextMessage: missing event_id from response";
		return;
	}

	emit messageSent(object.value("event_id").toString(),
			 reply->property("roomid").toString(),
			 reply->property("txn_id").toInt());
}

void MatrixClient::onRoomAvatarResponse(QNetworkReply *reply)
{
	reply->deleteLater();

	int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

	if (status == 0 || status >= 400) {
		qWarning() << reply->errorString();
		return;
	}

	auto img = reply->readAll();

	if (img.size() == 0)
		return;

	auto roomid = reply->property("roomid").toString();

	QPixmap pixmap;
	pixmap.loadFromData(img);

	emit roomAvatarRetrieved(roomid, pixmap);
}

void MatrixClient::onUserAvatarResponse(QNetworkReply *reply)
{
	reply->deleteLater();

	int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

	if (status == 0 || status >= 400) {
		qWarning() << reply->errorString();
		return;
	}

	auto data = reply->readAll();

	if (data.size() == 0)
		return;

	auto roomid = reply->property("userid").toString();

	QImage img;
	img.loadFromData(data);

	emit userAvatarRetrieved(roomid, img);
}
void MatrixClient::onGetOwnAvatarResponse(QNetworkReply *reply)
{
	reply->deleteLater();

	int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

	if (status == 0 || status >= 400) {
		qWarning() << reply->errorString();
		return;
	}

	auto img = reply->readAll();

	if (img.size() == 0)
		return;

	QPixmap pixmap;
	pixmap.loadFromData(img);

	emit ownAvatarRetrieved(pixmap);
}

void MatrixClient::onImageResponse(QNetworkReply *reply)
{
	reply->deleteLater();

	int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

	if (status == 0 || status >= 400) {
		qWarning() << reply->errorString();
		return;
	}

	auto img = reply->readAll();

	if (img.size() == 0)
		return;

	QPixmap pixmap;
	pixmap.loadFromData(img);

	auto event_id = reply->property("event_id").toString();

	emit imageDownloaded(event_id, pixmap);
}

void MatrixClient::onMessagesResponse(QNetworkReply *reply)
{
	reply->deleteLater();

	int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

	if (status == 0 || status >= 400) {
		qWarning() << reply->errorString();
		return;
	}

	auto data = reply->readAll();
	auto room_id = reply->property("room_id").toString();

	RoomMessages msgs;

	try {
		msgs.deserialize(QJsonDocument::fromJson(data));
	} catch (const DeserializationException &e) {
		qWarning() << "Room messages from" << room_id << e.what();
		return;
	}

	emit messagesRetrieved(room_id, msgs);
}

void MatrixClient::onResponse(QNetworkReply *reply)
{
	switch (static_cast<Endpoint>(reply->property("endpoint").toInt())) {
	case Endpoint::Versions:
		onVersionsResponse(reply);
		break;
	case Endpoint::Login:
		onLoginResponse(reply);
		break;
	case Endpoint::Logout:
		onLogoutResponse(reply);
		break;
	case Endpoint::Register:
		onRegisterResponse(reply);
		break;
	case Endpoint::GetOwnProfile:
		onGetOwnProfileResponse(reply);
		break;
	case Endpoint::Image:
		onImageResponse(reply);
		break;
	case Endpoint::InitialSync:
		onInitialSyncResponse(reply);
		break;
	case Endpoint::Sync:
		onSyncResponse(reply);
		break;
	case Endpoint::SendTextMessage:
		onSendTextMessageResponse(reply);
		break;
	case Endpoint::RoomAvatar:
		onRoomAvatarResponse(reply);
		break;
	case Endpoint::UserAvatar:
		onUserAvatarResponse(reply);
		break;
	case Endpoint::GetOwnAvatar:
		onGetOwnAvatarResponse(reply);
		break;
	case Endpoint::Messages:
		onMessagesResponse(reply);
		break;
	default:
		break;
	}
}

void MatrixClient::login(const QString &username, const QString &password) noexcept
{
	QUrl endpoint(server_);
	endpoint.setPath(api_url_ + "/login");

	QNetworkRequest request(endpoint);
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

	LoginRequest body(username, password);

	QNetworkReply *reply = post(request, body.serialize());
	reply->setProperty("endpoint", static_cast<int>(Endpoint::Login));
}

void MatrixClient::logout() noexcept
{
	QUrlQuery query;
	query.addQueryItem("access_token", token_);

	QUrl endpoint(server_);
	endpoint.setPath(api_url_ + "/logout");
	endpoint.setQuery(query);

	QNetworkRequest request(endpoint);
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

	QJsonObject body{};
	QNetworkReply *reply = post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
	reply->setProperty("endpoint", static_cast<int>(Endpoint::Logout));
}

void MatrixClient::registerUser(const QString &user, const QString &pass, const QString &server) noexcept
{
	setServer(server);

	QUrlQuery query;
	query.addQueryItem("kind", "user");

	QUrl endpoint(server_);
	endpoint.setPath(api_url_ + "/register");
	endpoint.setQuery(query);

	QNetworkRequest request(QString(endpoint.toEncoded()));
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

	RegisterRequest body(user, pass);

	QNetworkReply *reply = post(request, body.serialize());
	reply->setProperty("endpoint", static_cast<int>(Endpoint::Register));
}

void MatrixClient::sync() noexcept
{
	QJsonObject filter{{"room",
			    QJsonObject{{"ephemeral", QJsonObject{{"limit", 0}}}}},
			   {"presence", QJsonObject{{"limit", 0}}}};

	QUrlQuery query;
	query.addQueryItem("set_presence", "online");
	query.addQueryItem("filter", QJsonDocument(filter).toJson(QJsonDocument::Compact));
	query.addQueryItem("timeout", "30000");
	query.addQueryItem("access_token", token_);

	if (next_batch_.isEmpty()) {
		qDebug() << "Sync requires a valid next_batch token. Initial sync should be performed.";
		return;
	}

	query.addQueryItem("since", next_batch_);

	QUrl endpoint(server_);
	endpoint.setPath(api_url_ + "/sync");
	endpoint.setQuery(query);

	QNetworkRequest request(QString(endpoint.toEncoded()));

	QNetworkReply *reply = get(request);
	reply->setProperty("endpoint", static_cast<int>(Endpoint::Sync));
}

void MatrixClient::sendTextMessage(const QString &roomid, const QString &msg) noexcept
{
	QUrlQuery query;
	query.addQueryItem("access_token", token_);

	QUrl endpoint(server_);
	endpoint.setPath(api_url_ + QString("/rooms/%1/send/m.room.message/%2").arg(roomid).arg(txn_id_));
	endpoint.setQuery(query);

	QJsonObject body{
		{"msgtype", "m.text"},
		{"body", msg}};

	QNetworkRequest request(QString(endpoint.toEncoded()));
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

	QNetworkReply *reply = put(request, QJsonDocument(body).toJson(QJsonDocument::Compact));

	reply->setProperty("endpoint", static_cast<int>(Endpoint::SendTextMessage));
	reply->setProperty("txn_id", txn_id_);
	reply->setProperty("roomid", roomid);

	incrementTransactionId();
}

void MatrixClient::initialSync() noexcept
{
	QJsonArray excluded_presence = {
		QString("m.presence"),
	};

	QJsonObject filter{{"room",
			    QJsonObject{{"timeline", QJsonObject{{"limit", 20}}},
					{"ephemeral", QJsonObject{{"limit", 0}}}}},
			   {"presence", QJsonObject{{"not_types", excluded_presence}}}};

	QUrlQuery query;
	query.addQueryItem("full_state", "true");
	query.addQueryItem("set_presence", "online");
	query.addQueryItem("filter", QJsonDocument(filter).toJson(QJsonDocument::Compact));
	query.addQueryItem("access_token", token_);

	QUrl endpoint(server_);
	endpoint.setPath(api_url_ + "/sync");
	endpoint.setQuery(query);

	QNetworkRequest request(QString(endpoint.toEncoded()));

	QNetworkReply *reply = get(request);
	reply->setProperty("endpoint", static_cast<int>(Endpoint::InitialSync));
}

void MatrixClient::versions() noexcept
{
	QUrl endpoint(server_);
	endpoint.setPath("/_matrix/client/versions");

	QNetworkRequest request(endpoint);

	QNetworkReply *reply = get(request);
	reply->setProperty("endpoint", static_cast<int>(Endpoint::Versions));
}

void MatrixClient::getOwnProfile() noexcept
{
	// FIXME: Remove settings from the matrix client. The class should store the user's matrix ID.
	QSettings settings;
	auto userid = settings.value("auth/user_id", "").toString();

	QUrlQuery query;
	query.addQueryItem("access_token", token_);

	QUrl endpoint(server_);
	endpoint.setPath(api_url_ + "/profile/" + userid);
	endpoint.setQuery(query);

	QNetworkRequest request(QString(endpoint.toEncoded()));

	QNetworkReply *reply = get(request);
	reply->setProperty("endpoint", static_cast<int>(Endpoint::GetOwnProfile));
}

void MatrixClient::fetchRoomAvatar(const QString &roomid, const QUrl &avatar_url)
{
	QList<QString> url_parts = avatar_url.toString().split("mxc://");

	if (url_parts.size() != 2) {
		qDebug() << "Invalid format for room avatar " << avatar_url.toString();
		return;
	}

	QUrlQuery query;
	query.addQueryItem("width", "512");
	query.addQueryItem("height", "512");
	query.addQueryItem("method", "crop");

	QString media_url = QString("%1/_matrix/media/r0/thumbnail/%2").arg(getHomeServer().toString(), url_parts[1]);

	QUrl endpoint(media_url);
	endpoint.setQuery(query);

	QNetworkRequest avatar_request(endpoint);

	QNetworkReply *reply = get(avatar_request);
	reply->setProperty("roomid", roomid);
	reply->setProperty("endpoint", static_cast<int>(Endpoint::RoomAvatar));
}

void MatrixClient::fetchUserAvatar(const QString &userId, const QUrl &avatarUrl)
{
	QList<QString> url_parts = avatarUrl.toString().split("mxc://");

	if (url_parts.size() != 2) {
		qDebug() << "Invalid format for user avatar " << avatarUrl.toString();
		return;
	}

	QUrlQuery query;
	query.addQueryItem("width", "128");
	query.addQueryItem("height", "128");
	query.addQueryItem("method", "crop");

	QString media_url = QString("%1/_matrix/media/r0/thumbnail/%2").arg(getHomeServer().toString(), url_parts[1]);

	QUrl endpoint(media_url);
	endpoint.setQuery(query);

	QNetworkRequest avatar_request(endpoint);

	QNetworkReply *reply = get(avatar_request);
	reply->setProperty("userid", userId);
	reply->setProperty("endpoint", static_cast<int>(Endpoint::UserAvatar));
}

void MatrixClient::downloadImage(const QString &event_id, const QUrl &url)
{
	QNetworkRequest image_request(url);

	QNetworkReply *reply = get(image_request);
	reply->setProperty("event_id", event_id);
	reply->setProperty("endpoint", static_cast<int>(Endpoint::Image));
}

void MatrixClient::fetchOwnAvatar(const QUrl &avatar_url)
{
	QList<QString> url_parts = avatar_url.toString().split("mxc://");

	if (url_parts.size() != 2) {
		qDebug() << "Invalid format for media " << avatar_url.toString();
		return;
	}

	QUrlQuery query;
	query.addQueryItem("width", "512");
	query.addQueryItem("height", "512");
	query.addQueryItem("method", "crop");

	QString media_url = QString("%1/_matrix/media/r0/thumbnail/%2").arg(getHomeServer().toString(), url_parts[1]);

	QUrl endpoint(media_url);
	endpoint.setQuery(query);

	QNetworkRequest avatar_request(endpoint);

	QNetworkReply *reply = get(avatar_request);
	reply->setProperty("endpoint", static_cast<int>(Endpoint::GetOwnAvatar));
}

void MatrixClient::messages(const QString &room_id, const QString &from_token, int limit) noexcept
{
	QUrlQuery query;
	query.addQueryItem("access_token", token_);
	query.addQueryItem("from", from_token);
	query.addQueryItem("dir", "b");
	query.addQueryItem("limit", QString::number(limit));

	QUrl endpoint(server_);
	endpoint.setPath(api_url_ + QString("/rooms/%1/messages").arg(room_id));
	endpoint.setQuery(query);

	QNetworkRequest request(QString(endpoint.toEncoded()));

	QNetworkReply *reply = get(request);
	reply->setProperty("endpoint", static_cast<int>(Endpoint::Messages));
	reply->setProperty("room_id", room_id);
}
