/*
 * nheko Copyright (C) 2019  The nheko authors
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

#include <optional>

#include <QDateTime>
#include <QDir>
#include <QImage>
#include <QString>

#if __has_include(<lmdbxx/lmdb++.h>)
#include <lmdbxx/lmdb++.h>
#else
#include <lmdb++.h>
#endif
#include <nlohmann/json.hpp>

#include <mtx/responses.hpp>
#include <mtxclient/crypto/client.hpp>

#include "CacheCryptoStructs.h"
#include "CacheStructs.h"

int
numeric_key_comparison(const MDB_val *a, const MDB_val *b);

class Cache : public QObject
{
        Q_OBJECT

public:
        Cache(const QString &userId, QObject *parent = nullptr);

        static std::string displayName(const std::string &room_id, const std::string &user_id);
        static QString displayName(const QString &room_id, const QString &user_id);
        static QString avatarUrl(const QString &room_id, const QString &user_id);

        // presence
        mtx::presence::PresenceState presenceState(const std::string &user_id);
        std::string statusMessage(const std::string &user_id);

        // user cache stores user keys
        UserCache getUserCache(const std::string &user_id);
        int setUserCache(const std::string &user_id, const UserCache &body);
        int deleteUserCache(const std::string &user_id);

        // device verified cache
        DeviceVerifiedCache getVerifiedCache(const std::string &user_id);
        int setVerifiedCache(const std::string &user_id, const DeviceVerifiedCache &body);

        static void removeDisplayName(const QString &room_id, const QString &user_id);
        static void removeAvatarUrl(const QString &room_id, const QString &user_id);

        static void insertDisplayName(const QString &room_id,
                                      const QString &user_id,
                                      const QString &display_name);
        static void insertAvatarUrl(const QString &room_id,
                                    const QString &user_id,
                                    const QString &avatar_url);

        //! Load saved data for the display names & avatars.
        void populateMembers();
        std::vector<std::string> joinedRooms();

        QMap<QString, RoomInfo> roomInfo(bool withInvites = true);
        std::map<QString, bool> invites();

        //! Calculate & return the name of the room.
        QString getRoomName(lmdb::txn &txn, lmdb::dbi &statesdb, lmdb::dbi &membersdb);
        //! Get room join rules
        mtx::events::state::JoinRule getRoomJoinRule(lmdb::txn &txn, lmdb::dbi &statesdb);
        bool getRoomGuestAccess(lmdb::txn &txn, lmdb::dbi &statesdb);
        //! Retrieve the topic of the room if any.
        QString getRoomTopic(lmdb::txn &txn, lmdb::dbi &statesdb);
        //! Retrieve the room avatar's url if any.
        QString getRoomAvatarUrl(lmdb::txn &txn,
                                 lmdb::dbi &statesdb,
                                 lmdb::dbi &membersdb,
                                 const QString &room_id);
        //! Retrieve the version of the room if any.
        QString getRoomVersion(lmdb::txn &txn, lmdb::dbi &statesdb);

        //! Retrieve member info from a room.
        std::vector<RoomMember> getMembers(const std::string &room_id,
                                           std::size_t startIndex = 0,
                                           std::size_t len        = 30);

        void saveState(const mtx::responses::Sync &res);
        bool isInitialized() const;

        std::string nextBatchToken() const;

        void deleteData();

        void removeInvite(lmdb::txn &txn, const std::string &room_id);
        void removeInvite(const std::string &room_id);
        void removeRoom(lmdb::txn &txn, const std::string &roomid);
        void removeRoom(const std::string &roomid);
        void setup();

        cache::CacheVersion formatVersion();
        void setCurrentFormat();
        bool runMigrations();

        std::map<QString, mtx::responses::Timeline> roomMessages();

        QMap<QString, mtx::responses::Notifications> getTimelineMentions();

        //! Retrieve all the user ids from a room.
        std::vector<std::string> roomMembers(const std::string &room_id);

        //! Check if the given user has power leve greater than than
        //! lowest power level of the given events.
        bool hasEnoughPowerLevel(const std::vector<mtx::events::EventType> &eventTypes,
                                 const std::string &room_id,
                                 const std::string &user_id);

        //! Retrieves the saved room avatar.
        QImage getRoomAvatar(const QString &id);
        QImage getRoomAvatar(const std::string &id);

        //! Adds a user to the read list for the given event.
        //!
        //! There should be only one user id present in a receipt list per room.
        //! The user id should be removed from any other lists.
        using Receipts = std::map<std::string, std::map<std::string, uint64_t>>;
        void updateReadReceipt(lmdb::txn &txn,
                               const std::string &room_id,
                               const Receipts &receipts);

        //! Retrieve all the read receipts for the given event id and room.
        //!
        //! Returns a map of user ids and the time of the read receipt in milliseconds.
        using UserReceipts = std::multimap<uint64_t, std::string, std::greater<uint64_t>>;
        UserReceipts readReceipts(const QString &event_id, const QString &room_id);

        QByteArray image(const QString &url) const;
        QByteArray image(lmdb::txn &txn, const std::string &url) const;
        void saveImage(const std::string &url, const std::string &data);
        void saveImage(const QString &url, const QByteArray &data);

        RoomInfo singleRoomInfo(const std::string &room_id);
        std::vector<std::string> roomsWithStateUpdates(const mtx::responses::Sync &res);
        std::vector<std::string> roomsWithTagUpdates(const mtx::responses::Sync &res);
        std::map<QString, RoomInfo> getRoomInfo(const std::vector<std::string> &rooms);

        //! Calculates which the read status of a room.
        //! Whether all the events in the timeline have been read.
        bool calculateRoomReadStatus(const std::string &room_id);
        void calculateRoomReadStatus();

        std::vector<SearchResult> searchUsers(const std::string &room_id,
                                              const std::string &query,
                                              std::uint8_t max_items = 5);
        std::vector<RoomSearchResult> searchRooms(const std::string &query,
                                                  std::uint8_t max_items = 5);

        void markSentNotification(const std::string &event_id);
        //! Removes an event from the sent notifications.
        void removeReadNotification(const std::string &event_id);
        //! Check if we have sent a desktop notification for the given event id.
        bool isNotificationSent(const std::string &event_id);

        //! Add all notifications containing a user mention to the db.
        void saveTimelineMentions(const mtx::responses::Notifications &res);

        //! Remove old unused data.
        void deleteOldMessages();
        void deleteOldData() noexcept;
        //! Retrieve all saved room ids.
        std::vector<std::string> getRoomIds(lmdb::txn &txn);

        //! Mark a room that uses e2e encryption.
        void setEncryptedRoom(lmdb::txn &txn, const std::string &room_id);
        bool isRoomEncrypted(const std::string &room_id);

        //! Check if a user is a member of the room.
        bool isRoomMember(const std::string &user_id, const std::string &room_id);

        //
        // Outbound Megolm Sessions
        //
        void saveOutboundMegolmSession(const std::string &room_id,
                                       const OutboundGroupSessionData &data,
                                       mtx::crypto::OutboundGroupSessionPtr session);
        OutboundGroupSessionDataRef getOutboundMegolmSession(const std::string &room_id);
        bool outboundMegolmSessionExists(const std::string &room_id) noexcept;
        void updateOutboundMegolmSession(const std::string &room_id, int message_index);

        void importSessionKeys(const mtx::crypto::ExportedSessionKeys &keys);
        mtx::crypto::ExportedSessionKeys exportSessionKeys();

        //
        // Inbound Megolm Sessions
        //
        void saveInboundMegolmSession(const MegolmSessionIndex &index,
                                      mtx::crypto::InboundGroupSessionPtr session);
        OlmInboundGroupSession *getInboundMegolmSession(const MegolmSessionIndex &index);
        bool inboundMegolmSessionExists(const MegolmSessionIndex &index);

        //
        // Olm Sessions
        //
        void saveOlmSession(const std::string &curve25519, mtx::crypto::OlmSessionPtr session);
        std::vector<std::string> getOlmSessions(const std::string &curve25519);
        std::optional<mtx::crypto::OlmSessionPtr> getOlmSession(const std::string &curve25519,
                                                                const std::string &session_id);

        void saveOlmAccount(const std::string &pickled);
        std::string restoreOlmAccount();

        void restoreSessions();

signals:
        void newReadReceipts(const QString &room_id, const std::vector<QString> &event_ids);
        void roomReadStatus(const std::map<QString, bool> &status);
        void removeNotification(const QString &room_id, const QString &event_id);

private:
        //! Save an invited room.
        void saveInvite(lmdb::txn &txn,
                        lmdb::dbi &statesdb,
                        lmdb::dbi &membersdb,
                        const mtx::responses::InvitedRoom &room);

        //! Add a notification containing a user mention to the db.
        void saveTimelineMentions(lmdb::txn &txn,
                                  const std::string &room_id,
                                  const QList<mtx::responses::Notification> &res);

        //! Get timeline items that a user was mentions in for a given room
        mtx::responses::Notifications getTimelineMentionsForRoom(lmdb::txn &txn,
                                                                 const std::string &room_id);

        QString getInviteRoomName(lmdb::txn &txn, lmdb::dbi &statesdb, lmdb::dbi &membersdb);
        QString getInviteRoomTopic(lmdb::txn &txn, lmdb::dbi &statesdb);
        QString getInviteRoomAvatarUrl(lmdb::txn &txn, lmdb::dbi &statesdb, lmdb::dbi &membersdb);

        std::string getLastEventId(lmdb::txn &txn, const std::string &room_id);
        DescInfo getLastMessageInfo(lmdb::txn &txn, const std::string &room_id);
        void saveTimelineMessages(lmdb::txn &txn,
                                  const std::string &room_id,
                                  const mtx::responses::Timeline &res);

        mtx::responses::Timeline getTimelineMessages(lmdb::txn &txn, const std::string &room_id);

        //! Remove a room from the cache.
        // void removeLeftRoom(lmdb::txn &txn, const std::string &room_id);
        template<class T>
        void saveStateEvents(lmdb::txn &txn,
                             const lmdb::dbi &statesdb,
                             const lmdb::dbi &membersdb,
                             const std::string &room_id,
                             const std::vector<T> &events)
        {
                for (const auto &e : events)
                        saveStateEvent(txn, statesdb, membersdb, room_id, e);
        }

        template<class T>
        void saveStateEvent(lmdb::txn &txn,
                            const lmdb::dbi &statesdb,
                            const lmdb::dbi &membersdb,
                            const std::string &room_id,
                            const T &event)
        {
                using namespace mtx::events;
                using namespace mtx::events::state;

                if (auto e = std::get_if<StateEvent<Member>>(&event); e != nullptr) {
                        switch (e->content.membership) {
                        //
                        // We only keep users with invite or join membership.
                        //
                        case Membership::Invite:
                        case Membership::Join: {
                                auto display_name = e->content.display_name.empty()
                                                      ? e->state_key
                                                      : e->content.display_name;

                                // Lightweight representation of a member.
                                MemberInfo tmp{display_name, e->content.avatar_url};

                                lmdb::dbi_put(txn,
                                              membersdb,
                                              lmdb::val(e->state_key),
                                              lmdb::val(json(tmp).dump()));

                                insertDisplayName(QString::fromStdString(room_id),
                                                  QString::fromStdString(e->state_key),
                                                  QString::fromStdString(display_name));

                                insertAvatarUrl(QString::fromStdString(room_id),
                                                QString::fromStdString(e->state_key),
                                                QString::fromStdString(e->content.avatar_url));

                                break;
                        }
                        default: {
                                lmdb::dbi_del(
                                  txn, membersdb, lmdb::val(e->state_key), lmdb::val(""));

                                removeDisplayName(QString::fromStdString(room_id),
                                                  QString::fromStdString(e->state_key));
                                removeAvatarUrl(QString::fromStdString(room_id),
                                                QString::fromStdString(e->state_key));

                                break;
                        }
                        }

                        return;
                } else if (std::holds_alternative<StateEvent<Encryption>>(event)) {
                        setEncryptedRoom(txn, room_id);
                        return;
                }

                if (!isStateEvent(event))
                        return;

                std::visit(
                  [&txn, &statesdb](auto e) {
                          lmdb::dbi_put(
                            txn, statesdb, lmdb::val(to_string(e.type)), lmdb::val(json(e).dump()));
                  },
                  event);
        }

        template<class T>
        bool isStateEvent(const T &e)
        {
                using namespace mtx::events;
                using namespace mtx::events::state;

                return std::holds_alternative<StateEvent<Aliases>>(e) ||
                       std::holds_alternative<StateEvent<state::Avatar>>(e) ||
                       std::holds_alternative<StateEvent<CanonicalAlias>>(e) ||
                       std::holds_alternative<StateEvent<Create>>(e) ||
                       std::holds_alternative<StateEvent<GuestAccess>>(e) ||
                       std::holds_alternative<StateEvent<HistoryVisibility>>(e) ||
                       std::holds_alternative<StateEvent<JoinRules>>(e) ||
                       std::holds_alternative<StateEvent<Name>>(e) ||
                       std::holds_alternative<StateEvent<Member>>(e) ||
                       std::holds_alternative<StateEvent<PowerLevels>>(e) ||
                       std::holds_alternative<StateEvent<Topic>>(e);
        }

        template<class T>
        bool containsStateUpdates(const T &e)
        {
                using namespace mtx::events;
                using namespace mtx::events::state;

                return std::holds_alternative<StateEvent<state::Avatar>>(e) ||
                       std::holds_alternative<StateEvent<CanonicalAlias>>(e) ||
                       std::holds_alternative<StateEvent<Name>>(e) ||
                       std::holds_alternative<StateEvent<Member>>(e) ||
                       std::holds_alternative<StateEvent<Topic>>(e);
        }

        bool containsStateUpdates(const mtx::events::collections::StrippedEvents &e)
        {
                using namespace mtx::events;
                using namespace mtx::events::state;

                return std::holds_alternative<StrippedEvent<state::Avatar>>(e) ||
                       std::holds_alternative<StrippedEvent<CanonicalAlias>>(e) ||
                       std::holds_alternative<StrippedEvent<Name>>(e) ||
                       std::holds_alternative<StrippedEvent<Member>>(e) ||
                       std::holds_alternative<StrippedEvent<Topic>>(e);
        }

        void saveInvites(lmdb::txn &txn,
                         const std::map<std::string, mtx::responses::InvitedRoom> &rooms);

        void savePresence(
          lmdb::txn &txn,
          const std::vector<mtx::events::Event<mtx::events::presence::Presence>> &presenceUpdates);

        //! Sends signals for the rooms that are removed.
        void removeLeftRooms(lmdb::txn &txn,
                             const std::map<std::string, mtx::responses::LeftRoom> &rooms)
        {
                for (const auto &room : rooms) {
                        removeRoom(txn, room.first);

                        // Clean up leftover invites.
                        removeInvite(txn, room.first);
                }
        }

        lmdb::dbi getPendingReceiptsDb(lmdb::txn &txn)
        {
                return lmdb::dbi::open(txn, "pending_receipts", MDB_CREATE);
        }

        lmdb::dbi getMessagesDb(lmdb::txn &txn, const std::string &room_id)
        {
                auto db =
                  lmdb::dbi::open(txn, std::string(room_id + "/messages").c_str(), MDB_CREATE);
                lmdb::dbi_set_compare(txn, db, numeric_key_comparison);

                return db;
        }

        lmdb::dbi getInviteStatesDb(lmdb::txn &txn, const std::string &room_id)
        {
                return lmdb::dbi::open(
                  txn, std::string(room_id + "/invite_state").c_str(), MDB_CREATE);
        }

        lmdb::dbi getInviteMembersDb(lmdb::txn &txn, const std::string &room_id)
        {
                return lmdb::dbi::open(
                  txn, std::string(room_id + "/invite_members").c_str(), MDB_CREATE);
        }

        lmdb::dbi getStatesDb(lmdb::txn &txn, const std::string &room_id)
        {
                return lmdb::dbi::open(txn, std::string(room_id + "/state").c_str(), MDB_CREATE);
        }

        lmdb::dbi getMembersDb(lmdb::txn &txn, const std::string &room_id)
        {
                return lmdb::dbi::open(txn, std::string(room_id + "/members").c_str(), MDB_CREATE);
        }

        lmdb::dbi getMentionsDb(lmdb::txn &txn, const std::string &room_id)
        {
                return lmdb::dbi::open(txn, std::string(room_id + "/mentions").c_str(), MDB_CREATE);
        }

        lmdb::dbi getPresenceDb(lmdb::txn &txn)
        {
                return lmdb::dbi::open(txn, "presence", MDB_CREATE);
        }

        lmdb::dbi getUserCacheDb(lmdb::txn &txn)
        {
                return lmdb::dbi::open(txn, std::string("user_cache").c_str(), MDB_CREATE);
        }

        lmdb::dbi getDeviceVerifiedDb(lmdb::txn &txn)
        {
                return lmdb::dbi::open(txn, std::string("verified").c_str(), MDB_CREATE);
        }

        //! Retrieves or creates the database that stores the open OLM sessions between our device
        //! and the given curve25519 key which represents another device.
        //!
        //! Each entry is a map from the session_id to the pickled representation of the session.
        lmdb::dbi getOlmSessionsDb(lmdb::txn &txn, const std::string &curve25519_key)
        {
                return lmdb::dbi::open(
                  txn, std::string("olm_sessions/" + curve25519_key).c_str(), MDB_CREATE);
        }

        QString getDisplayName(const mtx::events::StateEvent<mtx::events::state::Member> &event)
        {
                if (!event.content.display_name.empty())
                        return QString::fromStdString(event.content.display_name);

                return QString::fromStdString(event.state_key);
        }

        void setNextBatchToken(lmdb::txn &txn, const std::string &token);
        void setNextBatchToken(lmdb::txn &txn, const QString &token);

        lmdb::env env_;
        lmdb::dbi syncStateDb_;
        lmdb::dbi roomsDb_;
        lmdb::dbi invitesDb_;
        lmdb::dbi mediaDb_;
        lmdb::dbi readReceiptsDb_;
        lmdb::dbi notificationsDb_;

        lmdb::dbi devicesDb_;
        lmdb::dbi deviceKeysDb_;

        lmdb::dbi inboundMegolmSessionDb_;
        lmdb::dbi outboundMegolmSessionDb_;

        QString localUserId_;
        QString cacheDirectory_;

        static QHash<QString, QString> DisplayNames;
        static QHash<QString, QString> AvatarUrls;

        OlmSessionStorage session_storage;
};

namespace cache {
Cache *
client();
}
