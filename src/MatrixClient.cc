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
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
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

	// FIXME: Other QNetworkAccessManagers use the finish handler.
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
		emit loginError("Wrong username or password");
		return;
	}

	if (status_code == 404) {
		emit loginError("Login endpoint was not found on the server");
		return;
	}

	if (status_code >= 400) {
		qWarning() << "Login error: " << reply->errorString();
		emit loginError("An unknown error occured. Please try again.");
		return;
	}

	auto data = reply->readAll();
	auto json = QJsonDocument::fromJson(data);

	LoginResponse response;

	try {
		response.deserialize(json);
		emit loginSuccess(response.getUserId(),
				  response.getHomeServer(),
				  response.getAccessToken());
	} catch (DeserializationException &e) {
		qWarning() << "Malformed JSON response" << e.what();
		emit loginError("Malformed response. Possibly not a Matrix server");
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
		emit initialSyncCompleted(response);
	} catch (DeserializationException &e) {
		qWarning() << "Sync malformed response" << e.what();
	}
}

void MatrixClient::onSyncResponse(QNetworkReply *reply)
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
			 reply->property("txn_id").toInt());

	incrementTransactionId();
}

void MatrixClient::onResponse(QNetworkReply *reply)
{
	switch (reply->property("endpoint").toInt()) {
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
	case Endpoint::InitialSync:
		onInitialSyncResponse(reply);
		break;
	case Endpoint::Sync:
		onSyncResponse(reply);
		break;
	case Endpoint::SendTextMessage:
		onSendTextMessageResponse(reply);
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
	reply->setProperty("endpoint", Endpoint::Login);
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
	reply->setProperty("endpoint", Endpoint::Logout);
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
	reply->setProperty("endpoint", Endpoint::Register);
}

void MatrixClient::sync() noexcept
{
	QJsonObject filter{{"room",
			    QJsonObject{{"ephemeral", QJsonObject{{"limit", 0}}}}},
			   {"presence", QJsonObject{{"limit", 0}}}};

	QUrlQuery query;
	query.addQueryItem("set_presence", "online");
	query.addQueryItem("filter", QJsonDocument(filter).toJson(QJsonDocument::Compact));
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
	reply->setProperty("endpoint", Endpoint::Sync);
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

	reply->setProperty("endpoint", Endpoint::SendTextMessage);
	reply->setProperty("txn_id", txn_id_);
}

void MatrixClient::initialSync() noexcept
{
	QJsonObject filter{{"room",
			    QJsonObject{{"timeline", QJsonObject{{"limit", 70}}},
					{"ephemeral", QJsonObject{{"limit", 0}}}}},
			   {"presence", QJsonObject{{"limit", 0}}}};

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
	reply->setProperty("endpoint", Endpoint::InitialSync);
}

void MatrixClient::versions() noexcept
{
	QUrl endpoint(server_);
	endpoint.setPath("/_matrix/client/versions");

	QNetworkRequest request(endpoint);

	QNetworkReply *reply = get(request);
	reply->setProperty("endpoint", Endpoint::Versions);
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
	reply->setProperty("endpoint", Endpoint::GetOwnProfile);
}
