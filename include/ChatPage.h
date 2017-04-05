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

#ifndef CHATPAGE_H
#define CHATPAGE_H

#include <QByteArray>
#include <QNetworkAccessManager>
#include <QPixmap>
#include <QTimer>
#include <QWidget>

#include "HistoryViewManager.h"
#include "MatrixClient.h"
#include "RoomInfo.h"
#include "RoomList.h"
#include "TextInputWidget.h"
#include "TopRoomBar.h"
#include "UserInfoWidget.h"

namespace Ui
{
class ChatPage;
}

class ChatPage : public QWidget
{
	Q_OBJECT

public:
	explicit ChatPage(QWidget *parent = 0);
	~ChatPage();

	// Initialize all the components of the UI.
	void bootstrap(QString userid, QString homeserver, QString token);

public slots:
	// Updates the user info box.
	void updateOwnProfileInfo(QUrl avatar_url, QString display_name);
	void fetchRoomAvatar(const QString &roomid, const QUrl &avatar_url);
	void initialSyncCompleted(SyncResponse response);
	void syncCompleted(SyncResponse response);
	void changeTopRoomInfo(const RoomInfo &info);
	void sendTextMessage(const QString &msg);
	void messageSent(const QString event_id, int txn_id);
	void startSync();

private:
	Ui::ChatPage *ui;

	void setOwnAvatar(QByteArray img);

	RoomList *room_list_;
	HistoryViewManager *view_manager_;

	TopRoomBar *top_bar_;
	TextInputWidget *text_input_;

	QTimer *sync_timer_;
	int sync_interval_;

	RoomInfo current_room_;
	QMap<QString, QPixmap> room_avatars_;

	UserInfoWidget *user_info_widget_;

	// Matrix client
	MatrixClient *matrix_client_;

	// Used for one off media requests.
	QNetworkAccessManager *content_downloader_;
};

#endif  // CHATPAGE_H
