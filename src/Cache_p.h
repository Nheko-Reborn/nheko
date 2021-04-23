// SPDX-FileCopyrightText: 2017 Konstantinos Sideris <siderisk@auth.gr>
// SPDX-FileCopyrightText: 2019 The nheko authors
// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <limits>
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

#include <mtx/responses/messages.hpp>
#include <mtx/responses/notifications.hpp>
#include <mtx/responses/sync.hpp>
#include <mtxclient/crypto/client.hpp>
#include <mtxclient/http/client.hpp>

#include "CacheCryptoStructs.h"
#include "CacheStructs.h"

class Cache : public QObject
{
        Q_OBJECT

public:
        Cache(const QString &userId, QObject *parent = nullptr);

        std::string displayName(const std::string &room_id, const std::string &user_id);
        QString displayName(const QString &room_id, const QString &user_id);
        QString avatarUrl(const QString &room_id, const QString &user_id);

        // presence
        mtx::presence::PresenceState presenceState(const std::string &user_id);
        std::string statusMessage(const std::string &user_id);

        // user cache stores user keys
        std::optional<UserKeyCache> userKeys(const std::string &user_id);
        std::map<std::string, std::optional<UserKeyCache>> getMembersWithKeys(
          const std::string &room_id);
        void updateUserKeys(const std::string &sync_token,
                            const mtx::responses::QueryKeys &keyQuery);
        void markUserKeysOutOfDate(lmdb::txn &txn,
                                   lmdb::dbi &db,
                                   const std::vector<std::string> &user_ids,
                                   const std::string &sync_token);
        void deleteUserKeys(lmdb::txn &txn,
                            lmdb::dbi &db,
                            const std::vector<std::string> &user_ids);
        void query_keys(const std::string &user_id,
                        std::function<void(const UserKeyCache &, mtx::http::RequestErr)> cb);

        // device & user verification cache
        VerificationStatus verificationStatus(const std::string &user_id);
        void markDeviceVerified(const std::string &user_id, const std::string &device);
        void markDeviceUnverified(const std::string &user_id, const std::string &device);

        std::vector<std::string> joinedRooms();

        QMap<QString, RoomInfo> roomInfo(bool withInvites = true);
        std::optional<mtx::events::state::CanonicalAlias> getRoomAliases(const std::string &roomid);
        std::map<QString, bool> invites();

        //! Calculate & return the name of the room.
        QString getRoomName(lmdb::txn &txn, lmdb::dbi &statesdb, lmdb::dbi &membersdb);
        //! Get room join rules
        mtx::events::state::JoinRule getRoomJoinRule(lmdb::txn &txn, lmdb::dbi &statesdb);
        bool getRoomGuestAccess(lmdb::txn &txn, lmdb::dbi &statesdb);
        //! Retrieve the topic of the room if any.
        QString getRoomTopic(lmdb::txn &txn, lmdb::dbi &statesdb);
        //! Retrieve the room avatar's url if any.
        QString getRoomAvatarUrl(lmdb::txn &txn, lmdb::dbi &statesdb, lmdb::dbi &membersdb);
        //! Retrieve the version of the room if any.
        QString getRoomVersion(lmdb::txn &txn, lmdb::dbi &statesdb);

        //! Retrieve member info from a room.
        std::vector<RoomMember> getMembers(const std::string &room_id,
                                           std::size_t startIndex = 0,
                                           std::size_t len        = 30);

        void saveState(const mtx::responses::Sync &res);
        bool isInitialized();

        std::string nextBatchToken();

        void deleteData();

        void removeInvite(lmdb::txn &txn, const std::string &room_id);
        void removeInvite(const std::string &room_id);
        void removeRoom(lmdb::txn &txn, const std::string &roomid);
        void removeRoom(const std::string &roomid);
        void setup();

        cache::CacheVersion formatVersion();
        void setCurrentFormat();
        bool runMigrations();

        std::vector<QString> roomIds();
        QMap<QString, mtx::responses::Notifications> getTimelineMentions();

        //! Retrieve all the user ids from a room.
        std::vector<std::string> roomMembers(const std::string &room_id);

        //! Check if the given user has power leve greater than than
        //! lowest power level of the given events.
        bool hasEnoughPowerLevel(const std::vector<mtx::events::EventType> &eventTypes,
                                 const std::string &room_id,
                                 const std::string &user_id);

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

        RoomInfo singleRoomInfo(const std::string &room_id);
        std::vector<std::string> roomsWithStateUpdates(const mtx::responses::Sync &res);
        std::vector<std::string> roomsWithTagUpdates(const mtx::responses::Sync &res);
        std::map<QString, RoomInfo> getRoomInfo(const std::vector<std::string> &rooms);

        //! Calculates which the read status of a room.
        //! Whether all the events in the timeline have been read.
        bool calculateRoomReadStatus(const std::string &room_id);
        void calculateRoomReadStatus();

        std::vector<RoomSearchResult> searchRooms(const std::string &query,
                                                  std::uint8_t max_items = 5);

        void markSentNotification(const std::string &event_id);
        //! Removes an event from the sent notifications.
        void removeReadNotification(const std::string &event_id);
        //! Check if we have sent a desktop notification for the given event id.
        bool isNotificationSent(const std::string &event_id);

        //! Add all notifications containing a user mention to the db.
        void saveTimelineMentions(const mtx::responses::Notifications &res);

        //! retrieve events in timeline and related functions
        struct Messages
        {
                mtx::responses::Timeline timeline;
                uint64_t next_index;
                bool end_of_cache = false;
        };
        Messages getTimelineMessages(lmdb::txn &txn,
                                     const std::string &room_id,
                                     uint64_t index = std::numeric_limits<uint64_t>::max(),
                                     bool forward   = false);

        std::optional<mtx::events::collections::TimelineEvent> getEvent(
          const std::string &room_id,
          const std::string &event_id);
        void storeEvent(const std::string &room_id,
                        const std::string &event_id,
                        const mtx::events::collections::TimelineEvent &event);
        std::vector<std::string> relatedEvents(const std::string &room_id,
                                               const std::string &event_id);

        struct TimelineRange
        {
                uint64_t first, last;
        };
        std::optional<TimelineRange> getTimelineRange(const std::string &room_id);
        std::optional<uint64_t> getTimelineIndex(const std::string &room_id,
                                                 std::string_view event_id);
        std::optional<uint64_t> getEventIndex(const std::string &room_id,
                                              std::string_view event_id);
        std::optional<std::pair<uint64_t, std::string>> lastInvisibleEventAfter(
          const std::string &room_id,
          std::string_view event_id);
        std::optional<std::string> getTimelineEventId(const std::string &room_id, uint64_t index);
        std::optional<uint64_t> getArrivalIndex(const std::string &room_id,
                                                std::string_view event_id);

        std::string previousBatchToken(const std::string &room_id);
        uint64_t saveOldMessages(const std::string &room_id, const mtx::responses::Messages &res);
        void savePendingMessage(const std::string &room_id,
                                const mtx::events::collections::TimelineEvent &message);
        std::optional<mtx::events::collections::TimelineEvent> firstPendingMessage(
          const std::string &room_id);
        void removePendingStatus(const std::string &room_id, const std::string &txn_id);

        //! clear timeline keeping only the latest batch
        void clearTimeline(const std::string &room_id);

        //! Remove old unused data.
        void deleteOldMessages();
        void deleteOldData() noexcept;
        //! Retrieve all saved room ids.
        std::vector<std::string> getRoomIds(lmdb::txn &txn);

        //! Mark a room that uses e2e encryption.
        void setEncryptedRoom(lmdb::txn &txn, const std::string &room_id);
        bool isRoomEncrypted(const std::string &room_id);
        std::optional<mtx::events::state::Encryption> roomEncryptionSettings(
          const std::string &room_id);

        //! Check if a user is a member of the room.
        bool isRoomMember(const std::string &user_id, const std::string &room_id);

        //
        // Outbound Megolm Sessions
        //
        void saveOutboundMegolmSession(const std::string &room_id,
                                       const OutboundGroupSessionData &data,
                                       mtx::crypto::OutboundGroupSessionPtr &session);
        OutboundGroupSessionDataRef getOutboundMegolmSession(const std::string &room_id);
        bool outboundMegolmSessionExists(const std::string &room_id) noexcept;
        void updateOutboundMegolmSession(const std::string &room_id,
                                         const OutboundGroupSessionData &data,
                                         mtx::crypto::OutboundGroupSessionPtr &session);
        void dropOutboundMegolmSession(const std::string &room_id);

        void importSessionKeys(const mtx::crypto::ExportedSessionKeys &keys);
        mtx::crypto::ExportedSessionKeys exportSessionKeys();

        //
        // Inbound Megolm Sessions
        //
        void saveInboundMegolmSession(const MegolmSessionIndex &index,
                                      mtx::crypto::InboundGroupSessionPtr session);
        mtx::crypto::InboundGroupSessionPtr getInboundMegolmSession(
          const MegolmSessionIndex &index);
        bool inboundMegolmSessionExists(const MegolmSessionIndex &index);

        //
        // Olm Sessions
        //
        void saveOlmSession(const std::string &curve25519,
                            mtx::crypto::OlmSessionPtr session,
                            uint64_t timestamp);
        std::vector<std::string> getOlmSessions(const std::string &curve25519);
        std::optional<mtx::crypto::OlmSessionPtr> getOlmSession(const std::string &curve25519,
                                                                const std::string &session_id);
        std::optional<mtx::crypto::OlmSessionPtr> getLatestOlmSession(
          const std::string &curve25519);

        void saveOlmAccount(const std::string &pickled);
        std::string restoreOlmAccount();

        void storeSecret(const std::string &name, const std::string &secret);
        void deleteSecret(const std::string &name);
        std::optional<std::string> secret(const std::string &name);

        template<class T>
        static constexpr bool isStateEvent(const mtx::events::StateEvent<T> &)
        {
                return true;
        }
        template<class T>
        static constexpr bool isStateEvent(const mtx::events::Event<T> &)
        {
                return false;
        }

        static int compare_state_key(const MDB_val *a, const MDB_val *b)
        {
                auto get_skey = [](const MDB_val *v) {
                        return nlohmann::json::parse(
                                 std::string_view(static_cast<const char *>(v->mv_data),
                                                  v->mv_size))
                          .value("key", "");
                };

                return get_skey(a).compare(get_skey(b));
        }

signals:
        void newReadReceipts(const QString &room_id, const std::vector<QString> &event_ids);
        void roomReadStatus(const std::map<QString, bool> &status);
        void removeNotification(const QString &room_id, const QString &event_id);
        void userKeysUpdate(const std::string &sync_token,
                            const mtx::responses::QueryKeys &keyQuery);
        void verificationStatusChanged(const std::string &userid);
        void secretChanged(const std::string name);

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

        std::optional<MemberInfo> getMember(const std::string &room_id, const std::string &user_id);

        std::string getLastEventId(lmdb::txn &txn, const std::string &room_id);
        DescInfo getLastMessageInfo(lmdb::txn &txn, const std::string &room_id);
        void saveTimelineMessages(lmdb::txn &txn,
                                  const std::string &room_id,
                                  const mtx::responses::Timeline &res);

        //! retrieve a specific event from account data
        //! pass empty room_id for global account data
        std::optional<mtx::events::collections::RoomAccountDataEvents>
        getAccountData(lmdb::txn &txn, mtx::events::EventType type, const std::string &room_id);
        bool isHiddenEvent(lmdb::txn &txn,
                           mtx::events::collections::TimelineEvents e,
                           const std::string &room_id);

        //! Remove a room from the cache.
        // void removeLeftRoom(lmdb::txn &txn, const std::string &room_id);
        template<class T>
        void saveStateEvents(lmdb::txn &txn,
                             lmdb::dbi &statesdb,
                             lmdb::dbi &stateskeydb,
                             lmdb::dbi &membersdb,
                             const std::string &room_id,
                             const std::vector<T> &events)
        {
                for (const auto &e : events)
                        saveStateEvent(txn, statesdb, stateskeydb, membersdb, room_id, e);
        }

        template<class T>
        void saveStateEvent(lmdb::txn &txn,
                            lmdb::dbi &statesdb,
                            lmdb::dbi &stateskeydb,
                            lmdb::dbi &membersdb,
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

                                membersdb.put(txn, e->state_key, json(tmp).dump());
                                break;
                        }
                        default: {
                                membersdb.del(txn, e->state_key, "");
                                break;
                        }
                        }

                        return;
                } else if (std::holds_alternative<StateEvent<Encryption>>(event)) {
                        setEncryptedRoom(txn, room_id);
                        return;
                }

                std::visit(
                  [&txn, &statesdb, &stateskeydb](auto e) {
                          if constexpr (isStateEvent(e))
                                  if (e.type != EventType::Unsupported) {
                                          if (e.state_key.empty())
                                                  statesdb.put(
                                                    txn, to_string(e.type), json(e).dump());
                                          else
                                                  stateskeydb.put(
                                                    txn,
                                                    to_string(e.type),
                                                    json::object({
                                                                   {"key", e.state_key},
                                                                   {"id", e.event_id},
                                                                 })
                                                      .dump());
                                  }
                  },
                  event);
        }

        template<typename T>
        std::optional<mtx::events::StateEvent<T>> getStateEvent(lmdb::txn txn,
                                                                const std::string &room_id,
                                                                std::string_view state_key = "")
        {
                constexpr auto type = mtx::events::state_content_to_type<T>;
                static_assert(type != mtx::events::EventType::Unsupported,
                              "Not a supported type in state events.");

                if (room_id.empty())
                        return std::nullopt;

                std::string_view value;
                if (state_key.empty()) {
                        auto db = getStatesDb(txn, room_id);
                        if (!db.get(txn, to_string(type), value)) {
                                return std::nullopt;
                        }
                } else {
                        auto db               = getStatesKeyDb(txn, room_id);
                        std::string d         = json::object({{"key", state_key}}).dump();
                        std::string_view data = d;

                        auto cursor = lmdb::cursor::open(txn, db);
                        if (!cursor.get(state_key, data, MDB_GET_BOTH))
                                return std::nullopt;

                        try {
                                auto eventsDb = getEventsDb(txn, room_id);
                                if (!eventsDb.get(
                                      txn, json::parse(data)["id"].get<std::string>(), value))
                                        return std::nullopt;
                        } catch (std::exception &e) {
                                return std::nullopt;
                        }
                }

                try {
                        return json::parse(value).get<mtx::events::StateEvent<T>>();
                } catch (std::exception &e) {
                        return std::nullopt;
                }
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

        lmdb::dbi getEventsDb(lmdb::txn &txn, const std::string &room_id)
        {
                return lmdb::dbi::open(txn, std::string(room_id + "/events").c_str(), MDB_CREATE);
        }

        lmdb::dbi getEventOrderDb(lmdb::txn &txn, const std::string &room_id)
        {
                return lmdb::dbi::open(
                  txn, std::string(room_id + "/event_order").c_str(), MDB_CREATE | MDB_INTEGERKEY);
        }

        // inverse of EventOrderDb
        lmdb::dbi getEventToOrderDb(lmdb::txn &txn, const std::string &room_id)
        {
                return lmdb::dbi::open(
                  txn, std::string(room_id + "/event2order").c_str(), MDB_CREATE);
        }

        lmdb::dbi getMessageToOrderDb(lmdb::txn &txn, const std::string &room_id)
        {
                return lmdb::dbi::open(
                  txn, std::string(room_id + "/msg2order").c_str(), MDB_CREATE);
        }

        lmdb::dbi getOrderToMessageDb(lmdb::txn &txn, const std::string &room_id)
        {
                return lmdb::dbi::open(
                  txn, std::string(room_id + "/order2msg").c_str(), MDB_CREATE | MDB_INTEGERKEY);
        }

        lmdb::dbi getPendingMessagesDb(lmdb::txn &txn, const std::string &room_id)
        {
                return lmdb::dbi::open(
                  txn, std::string(room_id + "/pending").c_str(), MDB_CREATE | MDB_INTEGERKEY);
        }

        lmdb::dbi getRelationsDb(lmdb::txn &txn, const std::string &room_id)
        {
                return lmdb::dbi::open(
                  txn, std::string(room_id + "/related").c_str(), MDB_CREATE | MDB_DUPSORT);
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

        lmdb::dbi getStatesKeyDb(lmdb::txn &txn, const std::string &room_id)
        {
                auto db =
                  lmdb::dbi::open(txn, std::string(room_id + "/state_by_key").c_str(), MDB_CREATE);
                lmdb::dbi_set_dupsort(txn, db, compare_state_key);
                return db;
        }

        lmdb::dbi getAccountDataDb(lmdb::txn &txn, const std::string &room_id)
        {
                return lmdb::dbi::open(
                  txn, std::string(room_id + "/account_data").c_str(), MDB_CREATE);
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

        lmdb::dbi getUserKeysDb(lmdb::txn &txn)
        {
                return lmdb::dbi::open(txn, "user_key", MDB_CREATE);
        }

        lmdb::dbi getVerificationDb(lmdb::txn &txn)
        {
                return lmdb::dbi::open(txn, "verified", MDB_CREATE);
        }

        //! Retrieves or creates the database that stores the open OLM sessions between our device
        //! and the given curve25519 key which represents another device.
        //!
        //! Each entry is a map from the session_id to the pickled representation of the session.
        lmdb::dbi getOlmSessionsDb(lmdb::txn &txn, const std::string &curve25519_key)
        {
                return lmdb::dbi::open(
                  txn, std::string("olm_sessions.v2/" + curve25519_key).c_str(), MDB_CREATE);
        }

        QString getDisplayName(const mtx::events::StateEvent<mtx::events::state::Member> &event)
        {
                if (!event.content.display_name.empty())
                        return QString::fromStdString(event.content.display_name);

                return QString::fromStdString(event.state_key);
        }

        std::optional<VerificationCache> verificationCache(const std::string &user_id);

        void setNextBatchToken(lmdb::txn &txn, const std::string &token);
        void setNextBatchToken(lmdb::txn &txn, const QString &token);

        lmdb::env env_;
        lmdb::dbi syncStateDb_;
        lmdb::dbi roomsDb_;
        lmdb::dbi invitesDb_;
        lmdb::dbi readReceiptsDb_;
        lmdb::dbi notificationsDb_;

        lmdb::dbi devicesDb_;
        lmdb::dbi deviceKeysDb_;

        lmdb::dbi inboundMegolmSessionDb_;
        lmdb::dbi outboundMegolmSessionDb_;

        QString localUserId_;
        QString cacheDirectory_;

        VerificationStorage verification_storage;
};

namespace cache {
Cache *
client();
}
