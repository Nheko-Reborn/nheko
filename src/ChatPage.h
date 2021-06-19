// SPDX-FileCopyrightText: 2017 Konstantinos Sideris <siderisk@auth.gr>
// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <atomic>
#include <optional>
#include <stack>
#include <variant>

#include <mtx/common.hpp>
#include <mtx/events.hpp>
#include <mtx/events/encrypted.hpp>
#include <mtx/events/member.hpp>
#include <mtx/events/presence.hpp>
#include <mtx/secret_storage.hpp>

#include <QFrame>
#include <QHBoxLayout>
#include <QMap>
#include <QPixmap>
#include <QPoint>
#include <QTimer>
#include <QWidget>

#include "CacheCryptoStructs.h"
#include "CacheStructs.h"
#include "notifications/Manager.h"

class OverlayModal;
class TimelineViewManager;
class UserSettings;
class NotificationsManager;
class TimelineModel;
class CallManager;

constexpr int CONSENSUS_TIMEOUT      = 1000;
constexpr int SHOW_CONTENT_TIMEOUT   = 3000;
constexpr int TYPING_REFRESH_TIMEOUT = 10000;

namespace mtx::requests {
struct CreateRoom;
}
namespace mtx::responses {
struct Notifications;
struct Sync;
struct Timeline;
struct Rooms;
}

using SecretsToDecrypt = std::map<std::string, mtx::secret_storage::AesHmacSha2EncryptedData>;

class ChatPage : public QWidget
{
        Q_OBJECT

public:
        ChatPage(QSharedPointer<UserSettings> userSettings, QWidget *parent = nullptr);

        // Initialize all the components of the UI.
        void bootstrap(QString userid, QString homeserver, QString token);

        static ChatPage *instance() { return instance_; }

        QSharedPointer<UserSettings> userSettings() { return userSettings_; }
        CallManager *callManager() { return callManager_; }
        TimelineViewManager *timelineManager() { return view_manager_; }
        void deleteConfigs();

        void initiateLogout();

        QString status() const;
        void setStatus(const QString &status);

        mtx::presence::PresenceState currentPresence() const;

        // TODO(Nico): Get rid of this!
        QString currentRoom() const;

public slots:
        void handleMatrixUri(const QByteArray &uri);
        void handleMatrixUri(const QUrl &uri);

        void startChat(QString userid);
        void leaveRoom(const QString &room_id);
        void createRoom(const mtx::requests::CreateRoom &req);
        void joinRoom(const QString &room);
        void joinRoomVia(const std::string &room_id,
                         const std::vector<std::string> &via,
                         bool promptForConfirmation = true);

        void inviteUser(QString userid, QString reason);
        void kickUser(QString userid, QString reason);
        void banUser(QString userid, QString reason);
        void unbanUser(QString userid, QString reason);

        void receivedSessionKey(const std::string &room_id, const std::string &session_id);
        void decryptDownloadedSecrets(mtx::secret_storage::AesHmacSha2KeyDescription keyDesc,
                                      const SecretsToDecrypt &secrets);
signals:
        void connectionLost();
        void connectionRestored();

        void notificationsRetrieved(const mtx::responses::Notifications &);
        void highlightedNotifsRetrieved(const mtx::responses::Notifications &,
                                        const QPoint widgetPos);

        void contentLoaded();
        void closing();
        void changeWindowTitle(const int);
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
        void newSyncResponse(const mtx::responses::Sync &res, const std::string &prev_batch_token);
        void leftRoom(const QString &room_id);
        void newRoom(const QString &room_id);

        void initializeViews(const mtx::responses::Rooms &rooms);
        void initializeEmptyViews();
        void initializeMentions(const QMap<QString, mtx::responses::Notifications> &notifs);
        void syncUI(const mtx::responses::Rooms &rooms);
        void dropToLoginPageCb(const QString &msg);

        void notifyMessage(const QString &roomid,
                           const QString &eventid,
                           const QString &roomname,
                           const QString &sender,
                           const QString &message,
                           const QImage &icon);

        void retrievedPresence(const QString &statusMsg, mtx::presence::PresenceState state);
        void themeChanged();
        void decryptSidebarChanged();
        void chatFocusChanged(const bool focused);

        //! Signals for device verificaiton
        void receivedDeviceVerificationAccept(
          const mtx::events::msg::KeyVerificationAccept &message);
        void receivedDeviceVerificationRequest(
          const mtx::events::msg::KeyVerificationRequest &message,
          std::string sender);
        void receivedRoomDeviceVerificationRequest(
          const mtx::events::RoomEvent<mtx::events::msg::KeyVerificationRequest> &message,
          TimelineModel *model);
        void receivedDeviceVerificationCancel(
          const mtx::events::msg::KeyVerificationCancel &message);
        void receivedDeviceVerificationKey(const mtx::events::msg::KeyVerificationKey &message);
        void receivedDeviceVerificationMac(const mtx::events::msg::KeyVerificationMac &message);
        void receivedDeviceVerificationStart(const mtx::events::msg::KeyVerificationStart &message,
                                             std::string sender);
        void receivedDeviceVerificationReady(const mtx::events::msg::KeyVerificationReady &message);
        void receivedDeviceVerificationDone(const mtx::events::msg::KeyVerificationDone &message);

        void downloadedSecrets(mtx::secret_storage::AesHmacSha2KeyDescription keyDesc,
                               const SecretsToDecrypt &secrets);

private slots:
        void logout();
        void removeRoom(const QString &room_id);
        void changeRoom(const QString &room_id);
        void dropToLoginPage(const QString &msg);

        void handleSyncResponse(const mtx::responses::Sync &res,
                                const std::string &prev_batch_token);

private:
        static ChatPage *instance_;

        void startInitialSync();
        void tryInitialSync();
        void trySync();
        void ensureOneTimeKeyCount(const std::map<std::string, uint16_t> &counts);
        void getProfileInfo();

        //! Check if the given room is currently open.
        bool isRoomActive(const QString &room_id);

        using UserID      = QString;
        using Membership  = mtx::events::StateEvent<mtx::events::state::Member>;
        using Memberships = std::map<std::string, Membership>;

        void loadStateFromCache();
        void resetUI();

        template<class Collection>
        Memberships getMemberships(const std::vector<Collection> &events) const;

        //! Send desktop notification for the received messages.
        void sendNotifications(const mtx::responses::Notifications &);

        template<typename T>
        void connectCallMessage();

        QHBoxLayout *topLayout_;

        TimelineViewManager *view_manager_;

        QTimer connectivityTimer_;
        std::atomic_bool isConnected_;

        // Global user settings.
        QSharedPointer<UserSettings> userSettings_;

        NotificationsManager notificationsManager;
        CallManager *callManager_;
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
