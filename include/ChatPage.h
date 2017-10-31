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

#include <QHBoxLayout>
#include <QMap>
#include <QPixmap>
#include <QTimer>
#include <QWidget>

#include "MemberEventContent.h"
#include "MessageEvent.h"
#include "StateEvent.h"

class Cache;
class MatrixClient;
class OverlayModal;
class QuickSwitcher;
class RoomList;
class RoomSettings;
class RoomState;
class SideBarActions;
class Splitter;
class SyncResponse;
class TextInputWidget;
class TimelineViewManager;
class TopRoomBar;
class TypingDisplay;
class UserInfoWidget;
class JoinedRoom;
class LeftRoom;

constexpr int CONSENSUS_TIMEOUT      = 1000;
constexpr int SHOW_CONTENT_TIMEOUT   = 3000;
constexpr int TYPING_REFRESH_TIMEOUT = 10000;

class ChatPage : public QWidget
{
        Q_OBJECT

public:
        ChatPage(QSharedPointer<MatrixClient> client, QWidget *parent = 0);
        ~ChatPage();

        // Initialize all the components of the UI.
        void bootstrap(QString userid, QString homeserver, QString token);
        void showQuickSwitcher();

signals:
        void contentLoaded();
        void close();
        void changeWindowTitle(const QString &msg);
        void unreadMessages(int count);
        void showNotification(const QString &msg);
        void showLoginPage(const QString &msg);

private slots:
        void showUnreadMessageNotification(int count);
        void updateTopBarAvatar(const QString &roomid, const QPixmap &img);
        void updateOwnProfileInfo(const QUrl &avatar_url, const QString &display_name);
        void setOwnAvatar(const QPixmap &img);
        void initialSyncCompleted(const SyncResponse &response);
        void syncCompleted(const SyncResponse &response);
        void syncFailed(const QString &msg);
        void changeTopRoomInfo(const QString &room_id);
        void logout();
        void addRoom(const QString &room_id);
        void removeRoom(const QString &room_id);

private:
        using UserID      = QString;
        using RoomStates  = QMap<UserID, RoomState>;
        using JoinedRooms = QMap<UserID, JoinedRoom>;
        using LeftRooms   = QMap<UserID, LeftRoom>;
        using Membership  = matrix::events::StateEvent<matrix::events::MemberEventContent>;
        using Memberships = QMap<UserID, Membership>;

        void removeLeftRooms(const LeftRooms &rooms);
        void updateJoinedRooms(const JoinedRooms &rooms);

        Memberships getMemberships(const QJsonArray &events) const;
        RoomStates generateMembershipDifference(const JoinedRooms &rooms,
                                                const RoomStates &states) const;

        void updateTypingUsers(const QString &roomid, const QList<QString> &user_ids);
        void updateUserMetadata(const QJsonArray &events);
        void updateUserDisplayName(const Membership &event);
        void updateUserAvatarUrl(const Membership &event);
        void loadStateFromCache();
        void deleteConfigs();
        void resetUI();

        QHBoxLayout *topLayout_;
        Splitter *splitter;

        QWidget *sideBar_;
        QVBoxLayout *sideBarLayout_;
        QVBoxLayout *sideBarTopLayout_;
        QVBoxLayout *sideBarMainLayout_;
        QWidget *sideBarTopWidget_;
        QVBoxLayout *sideBarTopWidgetLayout_;

        QWidget *content_;
        QVBoxLayout *contentLayout_;
        QHBoxLayout *topBarLayout_;
        QVBoxLayout *mainContentLayout_;

        RoomList *room_list_;
        TimelineViewManager *view_manager_;
        SideBarActions *sidebarActions_;

        TopRoomBar *top_bar_;
        TextInputWidget *text_input_;
        TypingDisplay *typingDisplay_;

        // Safety net if consensus is not possible or too slow.
        QTimer *showContentTimer_;
        QTimer *consensusTimer_;

        QString current_room_;
        QMap<QString, QPixmap> room_avatars_;

        UserInfoWidget *user_info_widget_;

        QMap<QString, RoomState> state_manager_;
        QMap<QString, QSharedPointer<RoomSettings>> settingsManager_;

        // Keeps track of the users currently typing on each room.
        QMap<QString, QList<QString>> typingUsers_;
        QTimer *typingRefresher_;

        QSharedPointer<QuickSwitcher> quickSwitcher_;
        QSharedPointer<OverlayModal> quickSwitcherModal_;

        // Matrix Client API provider.
        QSharedPointer<MatrixClient> client_;

        // LMDB wrapper.
        QSharedPointer<Cache> cache_;

        // If the number of failures exceeds a certain threshold we
        // return to the login page.
        int initialSyncFailures = 0;
};
