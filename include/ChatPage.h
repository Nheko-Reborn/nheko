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

#include <QFrame>
#include <QHBoxLayout>
#include <QMap>
#include <QPixmap>
#include <QTimer>
#include <QWidget>

#include "CommunitiesList.h"
#include "Community.h"
#include <mtx.hpp>

class Cache;
class MatrixClient;
class OverlayModal;
class QuickSwitcher;
class RoomList;
class RoomSettings;
class RoomState;
class SideBarActions;
class Splitter;
class TextInputWidget;
class TimelineViewManager;
class TopRoomBar;
class TypingDisplay;
class UserInfoWidget;
class UserSettings;

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
        ChatPage(QSharedPointer<MatrixClient> client,
                 QSharedPointer<UserSettings> userSettings,
                 QWidget *parent = 0);
        ~ChatPage();

        // Initialize all the components of the UI.
        void bootstrap(QString userid, QString homeserver, QString token);
        void showQuickSwitcher();
        void showReadReceipts(const QString &event_id);

        static ChatPage *instance() { return instance_; }

signals:
        void contentLoaded();
        void close();
        void changeWindowTitle(const QString &msg);
        void unreadMessages(int count);
        void showNotification(const QString &msg);
        void showLoginPage(const QString &msg);
        void showUserSettingsPage();

private slots:
        void showUnreadMessageNotification(int count);
        void updateTopBarAvatar(const QString &roomid, const QPixmap &img);
        void updateOwnProfileInfo(const QUrl &avatar_url, const QString &display_name);
        void updateOwnCommunitiesInfo(const QList<QString> &own_communities);
        void setOwnAvatar(const QPixmap &img);
        void initialSyncCompleted(const mtx::responses::Sync &response);
        void syncCompleted(const mtx::responses::Sync &response);
        void syncFailed(const QString &msg);
        void changeTopRoomInfo(const QString &room_id);
        void logout();
        void addRoom(const QString &room_id);
        void removeRoom(const QString &room_id);
        void removeInvite(const QString &room_id);

private:
        static ChatPage *instance_;

        using UserID      = QString;
        using RoomStates  = QMap<UserID, RoomState>;
        using Membership  = mtx::events::StateEvent<mtx::events::state::Member>;
        using Memberships = std::map<std::string, Membership>;

        using JoinedRooms = std::map<std::string, mtx::responses::JoinedRoom>;
        using LeftRooms   = std::map<std::string, mtx::responses::LeftRoom>;

        void removeLeftRooms(const LeftRooms &rooms);
        void updateJoinedRooms(const JoinedRooms &rooms);

        RoomStates generateMembershipDifference(const JoinedRooms &rooms,
                                                const RoomStates &states) const;

        void updateTypingUsers(const QString &roomid, const std::vector<std::string> &user_ids);

        using MemberEvent = mtx::events::StateEvent<mtx::events::state::Member>;
        void updateUserDisplayName(const MemberEvent &event);
        void updateUserAvatarUrl(const MemberEvent &event);

        void loadStateFromCache();
        void deleteConfigs();
        void resetUI();

        template<class Collection>
        Memberships getMemberships(const std::vector<Collection> &events) const;

        template<class Collection>
        void updateUserMetadata(const std::vector<Collection> &collection);

        QHBoxLayout *topLayout_;
        Splitter *splitter;

        QWidget *sideBar_;
        QWidget *communitiesSideBar_;
        QVBoxLayout *communitiesSideBarLayout_;
        QVBoxLayout *sideBarLayout_;
        QVBoxLayout *sideBarTopLayout_;
        QVBoxLayout *sideBarMainLayout_;
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

        // Safety net if consensus is not possible or too slow.
        QTimer *showContentTimer_;
        QTimer *consensusTimer_;

        QString current_room_;
        QString current_community_;

        QMap<QString, QPixmap> room_avatars_;
        QMap<QString, QPixmap> community_avatars_;

        UserInfoWidget *user_info_widget_;

        QMap<QString, RoomState> state_manager_;
        QMap<QString, QSharedPointer<RoomSettings>> settingsManager_;

        QMap<QString, QSharedPointer<Community>> communityManager_;

        // Keeps track of the users currently typing on each room.
        QMap<QString, QList<QString>> typingUsers_;
        QTimer *typingRefresher_;

        QSharedPointer<QuickSwitcher> quickSwitcher_;
        QSharedPointer<OverlayModal> quickSwitcherModal_;

        QSharedPointer<dialogs::ReadReceipts> receiptsDialog_;
        QSharedPointer<OverlayModal> receiptsModal_;

        // Matrix Client API provider.
        QSharedPointer<MatrixClient> client_;

        // Global user settings.
        QSharedPointer<UserSettings> userSettings_;

        // LMDB wrapper.
        QSharedPointer<Cache> cache_;

        // If the number of failures exceeds a certain threshold we
        // return to the login page.
        int initialSyncFailures = 0;
};

template<class Collection>
void
ChatPage::updateUserMetadata(const std::vector<Collection> &collection)
{
        using Member = mtx::events::StateEvent<mtx::events::state::Member>;

        for (auto &event : collection) {
                if (mpark::holds_alternative<Member>(event)) {
                        auto member = mpark::get<Member>(event);

                        updateUserAvatarUrl(member);
                        updateUserDisplayName(member);
                }
        }
}

template<class Collection>
std::map<std::string, mtx::events::StateEvent<mtx::events::state::Member>>
ChatPage::getMemberships(const std::vector<Collection> &collection) const
{
        std::map<std::string, mtx::events::StateEvent<mtx::events::state::Member>> memberships;

        using Member = mtx::events::StateEvent<mtx::events::state::Member>;

        for (auto &event : collection) {
                if (mpark::holds_alternative<Member>(event)) {
                        auto member = mpark::get<Member>(event);
                        memberships.emplace(member.state_key, member);
                }
        }

        return memberships;
}
