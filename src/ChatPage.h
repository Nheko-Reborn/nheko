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
#include <optional>
#include <stack>
#include <variant>

#include <mtx/common.hpp>
#include <mtx/requests.hpp>
#include <mtx/responses.hpp>
#include <mtxclient/http/errors.hpp>

#include <QFrame>
#include <QHBoxLayout>
#include <QMap>
#include <QPixmap>
#include <QPoint>
#include <QTimer>
#include <QWidget>

#include "CacheStructs.h"
#include "CommunitiesList.h"
#include "Utils.h"
#include "notifications/Manager.h"
#include "popups/UserMentions.h"

class OverlayModal;
class QuickSwitcher;
class RoomList;
class SideBarActions;
class Splitter;
class TextInputWidget;
class TimelineViewManager;
class TopRoomBar;
class UserInfoWidget;
class UserSettings;
class NotificationsManager;

constexpr int CONSENSUS_TIMEOUT      = 1000;
constexpr int SHOW_CONTENT_TIMEOUT   = 3000;
constexpr int TYPING_REFRESH_TIMEOUT = 10000;

namespace mtx::http {
using RequestErr = const std::optional<mtx::http::ClientError> &;
}

class ChatPage : public QWidget
{
        Q_OBJECT

public:
        ChatPage(QSharedPointer<UserSettings> userSettings, QWidget *parent = nullptr);

        // Initialize all the components of the UI.
        void bootstrap(QString userid, QString homeserver, QString token);
        void showQuickSwitcher();
        QString currentRoom() const { return current_room_; }

        static ChatPage *instance() { return instance_; }

        QSharedPointer<UserSettings> userSettings() { return userSettings_; }
        void deleteConfigs();

        CommunitiesList *communitiesList() { return communitiesList_; }

        //! Calculate the width of the message timeline.
        uint64_t timelineWidth();
        bool isSideBarExpanded();
        //! Hide the room & group list (if it was visible).
        void hideSideBars();
        //! Show the room/group list (if it was visible).
        void showSideBars();
        void initiateLogout();
        void focusMessageInput();

        QString status() const;
        void setStatus(const QString &status);

        mtx::presence::PresenceState currentPresence() const;

public slots:
        void leaveRoom(const QString &room_id);
        void createRoom(const mtx::requests::CreateRoom &req);

        void inviteUser(QString userid, QString reason);
        void kickUser(QString userid, QString reason);
        void banUser(QString userid, QString reason);
        void unbanUser(QString userid, QString reason);

signals:
        void connectionLost();
        void connectionRestored();

        void notificationsRetrieved(const mtx::responses::Notifications &);
        void highlightedNotifsRetrieved(const mtx::responses::Notifications &,
                                        const QPoint widgetPos);

        void uploadFailed(const QString &msg);
        void mediaUploaded(const QString &roomid,
                           const QString &filename,
                           const std::optional<mtx::crypto::EncryptedFile> &file,
                           const QString &url,
                           const QString &mimeClass,
                           const QString &mime,
                           qint64 dsize,
                           const QSize &dimensions,
                           const QString &blurhash);

        void contentLoaded();
        void closing();
        void changeWindowTitle(const QString &msg);
        void unreadMessages(int count);
        void showNotification(const QString &msg);
        void showLoginPage(const QString &msg);
        void showUserSettingsPage();
        void showOverlayProgressBar();

        void ownProfileOk();
        void setUserDisplayName(const QString &name);
        void setUserAvatar(const QString &avatar);
        void loggedOut();

        void trySyncCb();
        void tryDelayedSyncCb();
        void tryInitialSyncCb();
        void leftRoom(const QString &room_id);

        void initializeRoomList(QMap<QString, RoomInfo>);
        void initializeViews(const mtx::responses::Rooms &rooms);
        void initializeEmptyViews(const std::map<QString, mtx::responses::Timeline> &msgs);
        void initializeMentions(const QMap<QString, mtx::responses::Notifications> &notifs);
        void syncUI(const mtx::responses::Rooms &rooms);
        void syncRoomlist(const std::map<QString, RoomInfo> &updates);
        void syncTags(const std::map<QString, RoomInfo> &updates);
        void syncTopBar(const std::map<QString, RoomInfo> &updates);
        void dropToLoginPageCb(const QString &msg);

        void notifyMessage(const QString &roomid,
                           const QString &eventid,
                           const QString &roomname,
                           const QString &sender,
                           const QString &message,
                           const QImage &icon);

        void updateGroupsInfo(const mtx::responses::JoinedGroups &groups);
        void retrievedPresence(const QString &statusMsg, mtx::presence::PresenceState state);
        void themeChanged();
        void decryptSidebarChanged();

        //! Signals for device verificaiton
        void recievedDeviceVerificationAccept(
          const mtx::events::collections::DeviceEvents &message);
        void recievedDeviceVerificationRequest(
          const mtx::events::collections::DeviceEvents &message);
        void recievedDeviceVerificationCancel(
          const mtx::events::collections::DeviceEvents &message);
        void recievedDeviceVerificationKey(const mtx::events::collections::DeviceEvents &message);
        void recievedDeviceVerificationMac(const mtx::events::collections::DeviceEvents &message);
        void recievedDeviceVerificationStart(const mtx::events::collections::DeviceEvents &message);
        void recievedDeviceVerificationReady(const mtx::events::collections::DeviceEvents &message);
        void recievedDeviceVerificationDone(const mtx::events::collections::DeviceEvents &message);

private slots:
        void showUnreadMessageNotification(int count);
        void updateTopBarAvatar(const QString &roomid, const QString &img);
        void changeTopRoomInfo(const QString &room_id);
        void logout();
        void removeRoom(const QString &room_id);
        void dropToLoginPage(const QString &msg);

        void joinRoom(const QString &room);
        void sendTypingNotifications();

private:
        static ChatPage *instance_;

        //! Handler callback for initial sync. It doesn't run on the main thread so all
        //! communication with the GUI should be done through signals.
        void initialSyncHandler(const mtx::responses::Sync &res, mtx::http::RequestErr err);
        void startInitialSync();
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

        void loadStateFromCache();
        void resetUI();
        //! Decides whether or not to hide the group's sidebar.
        void setGroupViewState(bool isEnabled);

        template<class Collection>
        Memberships getMemberships(const std::vector<Collection> &events) const;

        //! Update the room with the new notification count.
        void updateRoomNotificationCount(const QString &room_id,
                                         uint16_t notification_count,
                                         uint16_t highlight_count);
        //! Send desktop notification for the received messages.
        void sendNotifications(const mtx::responses::Notifications &);

        void showNotificationsDialog(const QPoint &point);

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

        QTimer connectivityTimer_;
        std::atomic_bool isConnected_;

        QString current_room_;
        QString current_community_;

        UserInfoWidget *user_info_widget_;

        popups::UserMentions *user_mentions_popup_;

        QTimer *typingRefresher_;

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
                if (auto member = std::get_if<Member>(event)) {
                        memberships.emplace(member->state_key, *member);
                }
        }

        return memberships;
}
