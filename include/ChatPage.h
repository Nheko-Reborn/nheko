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

#include <atomic>

#include <QFrame>
#include <QHBoxLayout>
#include <QMap>
#include <QPixmap>
#include <QTimer>
#include <QWidget>

#include "Cache.h"
#include "CommunitiesList.h"
#include "Community.h"
#include "MatrixClient.h"
#include "notifications/Manager.h"

class OverlayModal;
class QuickSwitcher;
class RoomList;
class SideBarActions;
class Splitter;
class TextInputWidget;
class TimelineViewManager;
class TopRoomBar;
class TypingDisplay;
class UserInfoWidget;
class UserSettings;
class NotificationsManager;

namespace dialogs {
class ReadReceipts;
}

constexpr int CONSENSUS_TIMEOUT      = 1000;
constexpr int SHOW_CONTENT_TIMEOUT   = 3000;
constexpr int TYPING_REFRESH_TIMEOUT = 10000;

class ChatPage : public QWidget
{
        Q_OBJECT

public:
        ChatPage(QSharedPointer<UserSettings> userSettings, QWidget *parent = 0);

        // Initialize all the components of the UI.
        void bootstrap(QString userid, QString homeserver, QString token);
        void showQuickSwitcher();
        void showReadReceipts(const QString &event_id);
        QString currentRoom() const { return current_room_; }

        static ChatPage *instance() { return instance_; }

        QSharedPointer<UserSettings> userSettings() { return userSettings_; }
        void deleteConfigs();

        //! Calculate the width of the message timeline.
        int timelineWidth();
        bool isSideBarExpanded();
        //! Hide the room & group list (if it was visible).
        void hideSideBars();
        //! Show the room/group list (if it was visible).
        void showSideBars();

public slots:
        void leaveRoom(const QString &room_id);

signals:
        void connectionLost();
        void connectionRestored();

        void messageReply(const QString &username, const QString &msg);

        void notificationsRetrieved(const mtx::responses::Notifications &);

        void uploadFailed(const QString &msg);
        void imageUploaded(const QString &roomid,
                           const QString &filename,
                           const QString &url,
                           const QString &mime,
                           qint64 dsize,
                           const QSize &dimensions);
        void fileUploaded(const QString &roomid,
                          const QString &filename,
                          const QString &url,
                          const QString &mime,
                          qint64 dsize);
        void audioUploaded(const QString &roomid,
                           const QString &filename,
                           const QString &url,
                           const QString &mime,
                           qint64 dsize);
        void videoUploaded(const QString &roomid,
                           const QString &filename,
                           const QString &url,
                           const QString &mime,
                           qint64 dsize);

        void contentLoaded();
        void closing();
        void changeWindowTitle(const QString &msg);
        void unreadMessages(int count);
        void showNotification(const QString &msg);
        void showLoginPage(const QString &msg);
        void showUserSettingsPage();
        void showOverlayProgressBar();

        void removeTimelineEvent(const QString &room_id, const QString &event_id);

        void ownProfileOk();
        void setUserDisplayName(const QString &name);
        void setUserAvatar(const QImage &avatar);
        void loggedOut();

        void trySyncCb();
        void tryDelayedSyncCb();
        void tryInitialSyncCb();
        void leftRoom(const QString &room_id);

        void initializeRoomList(QMap<QString, RoomInfo>);
        void initializeViews(const mtx::responses::Rooms &rooms);
        void initializeEmptyViews(const std::map<QString, mtx::responses::Timeline> &msgs);
        void syncUI(const mtx::responses::Rooms &rooms);
        void syncRoomlist(const std::map<QString, RoomInfo> &updates);
        void syncTopBar(const std::map<QString, RoomInfo> &updates);
        void dropToLoginPageCb(const QString &msg);

        void notifyMessage(const QString &roomid,
                           const QString &eventid,
                           const QString &roomname,
                           const QString &sender,
                           const QString &message,
                           const QImage &icon);

private slots:
        void showUnreadMessageNotification(int count);
        void updateTopBarAvatar(const QString &roomid, const QPixmap &img);
        void updateOwnCommunitiesInfo(const QList<QString> &own_communities);
        void changeTopRoomInfo(const QString &room_id);
        void logout();
        void removeRoom(const QString &room_id);
        void dropToLoginPage(const QString &msg);

        void joinRoom(const QString &room);
        void createRoom(const mtx::requests::CreateRoom &req);
        void sendTypingNotifications();

private:
        static ChatPage *instance_;

        //! Handler callback for initial sync. It doesn't run on the main thread so all
        //! communication with the GUI should be done through signals.
        void initialSyncHandler(const mtx::responses::Sync &res, mtx::http::RequestErr err);
        void tryInitialSync();
        void trySync();
        void ensureOneTimeKeyCount(const std::map<std::string, uint16_t> &counts);
        void getProfileInfo();

        //! Check if the given room is currently open.
        bool isRoomActive(const QString &room_id)
        {
                return isActiveWindow() && currentRoom() == room_id;
        }

        using UserID      = QString;
        using Membership  = mtx::events::StateEvent<mtx::events::state::Member>;
        using Memberships = std::map<std::string, Membership>;

        using LeftRooms = std::map<std::string, mtx::responses::LeftRoom>;
        void removeLeftRooms(const LeftRooms &rooms);

        void updateTypingUsers(const QString &roomid, const std::vector<std::string> &user_ids);

        void loadStateFromCache();
        void resetUI();
        //! Decides whether or not to hide the group's sidebar.
        void setGroupViewState(bool isEnabled);

        template<class Collection>
        Memberships getMemberships(const std::vector<Collection> &events) const;

        //! Update the room with the new notification count.
        void updateRoomNotificationCount(const QString &room_id, uint16_t notification_count);
        //! Send desktop notification for the received messages.
        void sendDesktopNotifications(const mtx::responses::Notifications &);

        QStringList generateTypingUsers(const QString &room_id,
                                        const std::vector<std::string> &typing_users);

        QHBoxLayout *topLayout_;
        Splitter *splitter;

        QWidget *sideBar_;
        QVBoxLayout *sideBarLayout_;
        QWidget *sideBarTopWidget_;
        QVBoxLayout *sideBarTopWidgetLayout_;

        QFrame *content_;
        QVBoxLayout *contentLayout_;

        CommunitiesList *communitiesList_;
        RoomList *room_list_;

        TimelineViewManager *view_manager_;
        SideBarActions *sidebarActions_;

        TopRoomBar *top_bar_;
        TextInputWidget *text_input_;
        TypingDisplay *typingDisplay_;

        QTimer connectivityTimer_;
        std::atomic_bool isConnected_;

        QString current_room_;
        QString current_community_;

        UserInfoWidget *user_info_widget_;

        std::map<QString, QSharedPointer<Community>> communities_;

        // Keeps track of the users currently typing on each room.
        std::map<QString, QList<QString>> typingUsers_;
        QTimer *typingRefresher_;

        QSharedPointer<QuickSwitcher> quickSwitcher_;
        QSharedPointer<OverlayModal> quickSwitcherModal_;

        QSharedPointer<dialogs::ReadReceipts> receiptsDialog_;
        QSharedPointer<OverlayModal> receiptsModal_;

        // Global user settings.
        QSharedPointer<UserSettings> userSettings_;

        NotificationsManager notificationsManager;
};

template<class Collection>
std::map<std::string, mtx::events::StateEvent<mtx::events::state::Member>>
ChatPage::getMemberships(const std::vector<Collection> &collection) const
{
        std::map<std::string, mtx::events::StateEvent<mtx::events::state::Member>> memberships;

        using Member = mtx::events::StateEvent<mtx::events::state::Member>;

        for (const auto &event : collection) {
                if (mpark::holds_alternative<Member>(event)) {
                        auto member = mpark::get<Member>(event);
                        memberships.emplace(member.state_key, member);
                }
        }

        return memberships;
}
