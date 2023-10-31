// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <optional>

#include <QDateTime>
#include <QString>

#if __has_include(<lmdbxx/lmdb++.h>)
#include <lmdbxx/lmdb++.h>
#else
#include <lmdb++.h>
#endif

#include <mtx/events/collections.hpp>
#include <mtx/responses/notifications.hpp>
#include <mtx/responses/sync.hpp>
#include <mtxclient/crypto/types.hpp>
#include <mtxclient/http/client.hpp>

#include "CacheCryptoStructs.h"
#include "CacheStructs.h"

namespace mtx::responses {
struct Messages;
}

class Cache final : public QObject
{
    Q_OBJECT

public:
    Cache(const QString &userId, QObject *parent = nullptr);

    std::string displayName(const std::string &room_id, const std::string &user_id);
    QString displayName(const QString &room_id, const QString &user_id);
    QString avatarUrl(const QString &room_id, const QString &user_id);

    // presence
    mtx::events::presence::Presence presence(const std::string &user_id);

    // user cache stores user keys
    std::map<std::string, std::optional<UserKeyCache>>
    getMembersWithKeys(const std::string &room_id, bool verified_only);
    void updateUserKeys(const std::string &sync_token, const mtx::responses::QueryKeys &keyQuery);
    void markUserKeysOutOfDate(const std::vector<std::string> &user_ids);
    void markUserKeysOutOfDate(lmdb::txn &txn,
                               lmdb::dbi &db,
                               const std::vector<std::string> &user_ids,
                               const std::string &sync_token);
    void query_keys(const std::string &user_id,
                    std::function<void(const UserKeyCache &, mtx::http::RequestErr)> cb);

    // device & user verification cache
    std::optional<UserKeyCache> userKeys(const std::string &user_id);
    VerificationStatus verificationStatus(const std::string &user_id);
    void markDeviceVerified(const std::string &user_id, const std::string &device);
    void markDeviceUnverified(const std::string &user_id, const std::string &device);
    crypto::Trust roomVerificationStatus(const std::string &room_id);

    std::vector<std::string> joinedRooms();
    std::map<std::string, RoomInfo> getCommonRooms(const std::string &user_id);

    QMap<QString, RoomInfo> roomInfo(bool withInvites = true);
    QHash<QString, RoomInfo> invites();
    std::optional<RoomInfo> invite(std::string_view roomid);
    QMap<QString, std::optional<RoomInfo>> spaces();

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
    //! Retrieve if the room is a space
    bool getRoomIsSpace(lmdb::txn &txn, lmdb::dbi &statesdb);
    //! Retrieve if the room is tombstoned (closed or replaced by a different room)
    bool getRoomIsTombstoned(lmdb::txn &txn, lmdb::dbi &statesdb);

    // for the event expiry background job
    void storeEventExpirationProgress(const std::string &room,
                                      const std::string &expirationSettings,
                                      const std::string &stopMarker);
    std::string
    loadEventExpirationProgress(const std::string &room, const std::string &expirationSettings);

    //! Get a specific state event
    template<typename T>
    std::optional<mtx::events::StateEvent<T>>
    getStateEvent(const std::string &room_id, std::string_view state_key = "")
    {
        auto txn = lmdb::txn::begin(env_, nullptr, MDB_RDONLY);
        return getStateEvent<T>(txn, room_id, state_key);
    }
    template<typename T>
    std::vector<mtx::events::StateEvent<T>>
    getStateEventsWithType(const std::string &room_id,
                           mtx::events::EventType type = mtx::events::state_content_to_type<T>)
    {
        auto txn = lmdb::txn::begin(env_, nullptr, MDB_RDONLY);
        return getStateEventsWithType<T>(txn, room_id, type);
    }

    //! retrieve a specific event from account data
    //! pass empty room_id for global account data
    std::optional<mtx::events::collections::RoomAccountDataEvents>
    getAccountData(mtx::events::EventType type, const std::string &room_id = "");

    //! Retrieve member info from a room.
    std::vector<RoomMember>
    getMembers(const std::string &room_id, std::size_t startIndex = 0, std::size_t len = 30);

    std::vector<RoomMember> getMembersFromInvite(const std::string &room_id,
                                                 std::size_t startIndex = 0,
                                                 std::size_t len        = 30);
    size_t memberCount(const std::string &room_id);

    void updateState(const std::string &room,
                     const mtx::responses::StateEvents &state,
                     bool wipe = false);
    void saveState(const mtx::responses::Sync &res);
    bool isInitialized();
    bool isDatabaseReady() { return databaseReady_ && isInitialized(); }

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
    void updateReadReceipt(lmdb::txn &txn, const std::string &room_id, const Receipts &receipts);

    //! Retrieve all the read receipts for the given event id and room.
    //!
    //! Returns a map of user ids and the time of the read receipt in milliseconds.
    using UserReceipts = std::multimap<uint64_t, std::string, std::greater<uint64_t>>;
    UserReceipts readReceipts(const QString &event_id, const QString &room_id);

    RoomInfo singleRoomInfo(const std::string &room_id);
    std::vector<std::string> roomsWithStateUpdates(const mtx::responses::Sync &res);
    std::map<QString, RoomInfo> getRoomInfo(const std::vector<std::string> &rooms);

    std::vector<RoomNameAlias> roomNamesAndAliases();

    void updateLastMessageTimestamp(const std::string &room_id, uint64_t ts);

    //! Calculates which the read status of a room.
    //! Whether all the events in the timeline have been read.
    std::string getFullyReadEventId(const std::string &room_id);
    bool calculateRoomReadStatus(const std::string &room_id);
    void calculateRoomReadStatus();

    void markSentNotification(const std::string &event_id);
    //! Removes an event from the sent notifications.
    void removeReadNotification(const std::string &event_id);
    //! Check if we have sent a desktop notification for the given event id.
    bool isNotificationSent(const std::string &event_id);

    std::optional<mtx::events::collections::TimelineEvents>
    getEvent(const std::string &room_id, std::string_view event_id);
    void storeEvent(const std::string &room_id,
                    const std::string &event_id,
                    const mtx::events::collections::TimelineEvents &event);
    void replaceEvent(const std::string &room_id,
                      const std::string &event_id,
                      const mtx::events::collections::TimelineEvents &event);
    std::vector<std::string> relatedEvents(const std::string &room_id, const std::string &event_id);

    struct TimelineRange
    {
        uint64_t first, last;
    };
    std::optional<TimelineRange> getTimelineRange(const std::string &room_id);
    std::optional<uint64_t> getTimelineIndex(const std::string &room_id, std::string_view event_id);
    std::optional<uint64_t> getEventIndex(const std::string &room_id, std::string_view event_id);
    std::optional<std::pair<uint64_t, std::string>>
    lastInvisibleEventAfter(const std::string &room_id, std::string_view event_id);
    std::optional<std::pair<uint64_t, std::string>>
    lastVisibleEvent(const std::string &room_id, std::string_view event_id);
    std::optional<std::string> getTimelineEventId(const std::string &room_id, uint64_t index);

    std::string previousBatchToken(const std::string &room_id);
    uint64_t saveOldMessages(const std::string &room_id, const mtx::responses::Messages &res);
    void savePendingMessage(const std::string &room_id,
                            const mtx::events::collections::TimelineEvents &message);
    std::vector<std::string> pendingEvents(const std::string &room_id);
    std::optional<mtx::events::collections::TimelineEvents>
    firstPendingMessage(const std::string &room_id);
    void removePendingStatus(const std::string &room_id, const std::string &txn_id);

    //! clear timeline keeping only the latest batch
    void clearTimeline(const std::string &room_id);

    //! Remove old unused data.
    void deleteOldMessages();
    void deleteOldData() noexcept;
    //! Retrieve all saved room ids.
    std::vector<std::string> getRoomIds(lmdb::txn &txn);
    std::vector<std::string> getParentRoomIds(const std::string &room_id);
    std::vector<std::string> getChildRoomIds(const std::string &room_id);

    std::vector<ImagePackInfo>
    getImagePacks(const std::string &room_id, std::optional<bool> stickers);

    //! Mark a room that uses e2e encryption.
    void setEncryptedRoom(lmdb::txn &txn, const std::string &room_id);
    bool isRoomEncrypted(const std::string &room_id);
    std::optional<mtx::events::state::Encryption>
    roomEncryptionSettings(const std::string &room_id);

    //! Check if a user is a member of the room.
    bool isRoomMember(const std::string &user_id, const std::string &room_id);
    std::optional<MemberInfo>
    getInviteMember(const std::string &room_id, const std::string &user_id);

    //
    // Outbound Megolm Sessions
    //
    void saveOutboundMegolmSession(const std::string &room_id,
                                   const GroupSessionData &data,
                                   mtx::crypto::OutboundGroupSessionPtr &session);
    OutboundGroupSessionDataRef getOutboundMegolmSession(const std::string &room_id);
    bool outboundMegolmSessionExists(const std::string &room_id) noexcept;
    void updateOutboundMegolmSession(const std::string &room_id,
                                     const GroupSessionData &data,
                                     mtx::crypto::OutboundGroupSessionPtr &session);
    void dropOutboundMegolmSession(const std::string &room_id);

    void importSessionKeys(const mtx::crypto::ExportedSessionKeys &keys);
    mtx::crypto::ExportedSessionKeys exportSessionKeys();

    //
    // Inbound Megolm Sessions
    //
    void saveInboundMegolmSession(const MegolmSessionIndex &index,
                                  mtx::crypto::InboundGroupSessionPtr session,
                                  const GroupSessionData &data);
    mtx::crypto::InboundGroupSessionPtr getInboundMegolmSession(const MegolmSessionIndex &index);
    bool inboundMegolmSessionExists(const MegolmSessionIndex &index);
    std::optional<GroupSessionData> getMegolmSessionData(const MegolmSessionIndex &index);

    //
    // Olm Sessions
    //
    void saveOlmSession(const std::string &curve25519,
                        mtx::crypto::OlmSessionPtr session,
                        uint64_t timestamp);
    void saveOlmSessions(std::vector<std::pair<std::string, mtx::crypto::OlmSessionPtr>> sessions,
                         uint64_t timestamp);
    std::vector<std::string> getOlmSessions(const std::string &curve25519);
    std::optional<mtx::crypto::OlmSessionPtr>
    getOlmSession(const std::string &curve25519, const std::string &session_id);
    std::optional<mtx::crypto::OlmSessionPtr> getLatestOlmSession(const std::string &curve25519);

    void saveOlmAccount(const std::string &pickled);
    std::string restoreOlmAccount();

    void saveBackupVersion(const OnlineBackupVersion &data);
    void deleteBackupVersion();
    std::optional<OnlineBackupVersion> backupVersion();

    void storeSecret(std::string_view name, const std::string &secret, bool internal = false);
    void deleteSecret(std::string_view name, bool internal = false);
    std::optional<std::string> secret(std::string_view name, bool internal = false);

    std::string pickleSecret();

    template<class T>
    constexpr static bool isStateEvent_ =
      std::is_same_v<std::remove_cv_t<std::remove_reference_t<T>>,
                     mtx::events::StateEvent<decltype(std::declval<T>().content)>>;

    static int compare_state_key(const MDB_val *a, const MDB_val *b)
    {
        auto get_skey = [](const MDB_val *v) {
            auto temp = std::string_view(static_cast<const char *>(v->mv_data), v->mv_size);
            // allow only passing the state key, in which case no null char will be in it and we
            // return the whole string because rfind returns npos.
            // We search from the back, because state keys could include nullbytes, event ids can
            // not.
            return temp.substr(0, temp.rfind('\0'));
        };

        return get_skey(a).compare(get_skey(b));
    }

signals:
    void newReadReceipts(const QString &room_id, const std::vector<QString> &event_ids);
    void roomReadStatus(const std::map<QString, bool> &status);
    void userKeysUpdate(const std::string &sync_token, const mtx::responses::QueryKeys &keyQuery);
    void userKeysUpdateFinalize(const std::string &user_id);
    void verificationStatusChanged(const std::string &userid);
    void selfVerificationStatusChanged();
    void secretChanged(const std::string name);
    void databaseReady();

private:
    void loadSecretsFromStore(
      std::vector<std::pair<std::string, bool>> toLoad,
      std::function<void(const std::string &name, bool internal, const std::string &value)>
        callback,
      bool databaseReadyOnFinished = false);
    void storeSecretInStore(const std::string name, const std::string secret);
    void deleteSecretFromStore(const std::string name, bool internal);

    //! Save an invited room.
    void saveInvite(lmdb::txn &txn,
                    lmdb::dbi &statesdb,
                    lmdb::dbi &membersdb,
                    const mtx::responses::InvitedRoom &room);

    QString getInviteRoomName(lmdb::txn &txn, lmdb::dbi &statesdb, lmdb::dbi &membersdb);
    QString getInviteRoomTopic(lmdb::txn &txn, lmdb::dbi &statesdb);
    QString getInviteRoomAvatarUrl(lmdb::txn &txn, lmdb::dbi &statesdb, lmdb::dbi &membersdb);
    bool getInviteRoomIsSpace(lmdb::txn &txn, lmdb::dbi &db);

    std::optional<MemberInfo> getMember(const std::string &room_id, const std::string &user_id);

    std::string getLastEventId(lmdb::txn &txn, const std::string &room_id);
    void saveTimelineMessages(lmdb::txn &txn,
                              lmdb::dbi &eventsDb,
                              const std::string &room_id,
                              const mtx::responses::Timeline &res);

    //! retrieve a specific event from account data
    //! pass empty room_id for global account data
    std::optional<mtx::events::collections::RoomAccountDataEvents>
    getAccountData(lmdb::txn &txn, mtx::events::EventType type, const std::string &room_id);
    bool isHiddenEvent(lmdb::txn &txn,
                       mtx::events::collections::TimelineEvents e,
                       const std::string &room_id);

    template<class T>
    void saveStateEvents(lmdb::txn &txn,
                         lmdb::dbi &statesdb,
                         lmdb::dbi &stateskeydb,
                         lmdb::dbi &membersdb,
                         lmdb::dbi &eventsDb,
                         const std::string &room_id,
                         const std::vector<T> &events);

    template<class T>
    void saveStateEvent(lmdb::txn &txn,
                        lmdb::dbi &statesdb,
                        lmdb::dbi &stateskeydb,
                        lmdb::dbi &membersdb,
                        lmdb::dbi &eventsDb,
                        const std::string &room_id,
                        const T &event);

    template<typename T>
    std::optional<mtx::events::StateEvent<T>>
    getStateEvent(lmdb::txn &txn, const std::string &room_id, std::string_view state_key = "");

    template<typename T>
    std::vector<mtx::events::StateEvent<T>>
    getStateEventsWithType(lmdb::txn &txn,
                           const std::string &room_id,
                           mtx::events::EventType type = mtx::events::state_content_to_type<T>);

    void
    saveInvites(lmdb::txn &txn, const std::map<std::string, mtx::responses::InvitedRoom> &rooms);

    void savePresence(
      lmdb::txn &txn,
      const std::vector<mtx::events::Event<mtx::events::presence::Presence>> &presenceUpdates);

    //! Sends signals for the rooms that are removed.
    void
    removeLeftRooms(lmdb::txn &txn, const std::map<std::string, mtx::responses::LeftRoom> &rooms)
    {
        for (const auto &room : rooms) {
            removeRoom(txn, room.first);

            // Clean up leftover invites.
            removeInvite(txn, room.first);
        }
    }

    void updateSpaces(lmdb::txn &txn,
                      const std::set<std::string> &spaces_with_updates,
                      std::set<std::string> rooms_with_updates);

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
        return lmdb::dbi::open(txn, std::string(room_id + "/event2order").c_str(), MDB_CREATE);
    }

    lmdb::dbi getMessageToOrderDb(lmdb::txn &txn, const std::string &room_id)
    {
        return lmdb::dbi::open(txn, std::string(room_id + "/msg2order").c_str(), MDB_CREATE);
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
        return lmdb::dbi::open(txn, std::string(room_id + "/invite_state").c_str(), MDB_CREATE);
    }

    lmdb::dbi getInviteMembersDb(lmdb::txn &txn, const std::string &room_id)
    {
        return lmdb::dbi::open(txn, std::string(room_id + "/invite_members").c_str(), MDB_CREATE);
    }

    lmdb::dbi getStatesDb(lmdb::txn &txn, const std::string &room_id)
    {
        return lmdb::dbi::open(txn, std::string(room_id + "/state").c_str(), MDB_CREATE);
    }

    lmdb::dbi getStatesKeyDb(lmdb::txn &txn, const std::string &room_id)
    {
        auto db = lmdb::dbi::open(
          txn, std::string(room_id + "/states_key").c_str(), MDB_CREATE | MDB_DUPSORT);
        lmdb::dbi_set_dupsort(txn, db, compare_state_key);
        return db;
    }

    lmdb::dbi getAccountDataDb(lmdb::txn &txn, const std::string &room_id)
    {
        return lmdb::dbi::open(txn, std::string(room_id + "/account_data").c_str(), MDB_CREATE);
    }

    lmdb::dbi getMembersDb(lmdb::txn &txn, const std::string &room_id)
    {
        return lmdb::dbi::open(txn, std::string(room_id + "/members").c_str(), MDB_CREATE);
    }

    lmdb::dbi getUserKeysDb(lmdb::txn &txn) { return lmdb::dbi::open(txn, "user_key", MDB_CREATE); }

    lmdb::dbi getVerificationDb(lmdb::txn &txn)
    {
        return lmdb::dbi::open(txn, "verified", MDB_CREATE);
    }

    QString getDisplayName(const mtx::events::StateEvent<mtx::events::state::Member> &event)
    {
        if (!event.content.display_name.empty())
            return QString::fromStdString(event.content.display_name);

        return QString::fromStdString(event.state_key);
    }

    std::optional<VerificationCache> verificationCache(const std::string &user_id, lmdb::txn &txn);
    VerificationStatus verificationStatus_(const std::string &user_id, lmdb::txn &txn);
    std::optional<UserKeyCache> userKeys_(const std::string &user_id, lmdb::txn &txn);

    void setNextBatchToken(lmdb::txn &txn, const std::string &token);

    lmdb::env env_;
    lmdb::dbi syncStateDb_;
    lmdb::dbi roomsDb_;
    lmdb::dbi spacesChildrenDb_, spacesParentsDb_;
    lmdb::dbi invitesDb_;
    lmdb::dbi readReceiptsDb_;
    lmdb::dbi notificationsDb_;
    lmdb::dbi presenceDb_;

    lmdb::dbi devicesDb_;
    lmdb::dbi deviceKeysDb_;

    lmdb::dbi inboundMegolmSessionDb_;
    lmdb::dbi outboundMegolmSessionDb_;
    lmdb::dbi megolmSessionDataDb_;
    lmdb::dbi olmSessionDb_;

    lmdb::dbi encryptedRooms_;

    lmdb::dbi eventExpiryBgJob_;

    QString localUserId_;
    QString cacheDirectory_;

    std::string pickle_secret_;

    VerificationStorage verification_storage;

    bool databaseReady_ = false;
};

namespace cache {
Cache *
client();
}

#define NHEKO_CACHE_GET_STATE_EVENT_FORWARD(Content)                                               \
    extern template std::optional<mtx::events::StateEvent<Content>> Cache::getStateEvent(          \
      lmdb::txn &txn, const std::string &room_id, std::string_view state_key);                     \
                                                                                                   \
    extern template std::vector<mtx::events::StateEvent<Content>> Cache::getStateEventsWithType(   \
      lmdb::txn &txn, const std::string &room_id, mtx::events::EventType type);

NHEKO_CACHE_GET_STATE_EVENT_FORWARD(mtx::events::state::Aliases)
NHEKO_CACHE_GET_STATE_EVENT_FORWARD(mtx::events::state::Avatar)
NHEKO_CACHE_GET_STATE_EVENT_FORWARD(mtx::events::state::CanonicalAlias)
NHEKO_CACHE_GET_STATE_EVENT_FORWARD(mtx::events::state::Create)
NHEKO_CACHE_GET_STATE_EVENT_FORWARD(mtx::events::state::Encryption)
NHEKO_CACHE_GET_STATE_EVENT_FORWARD(mtx::events::state::GuestAccess)
NHEKO_CACHE_GET_STATE_EVENT_FORWARD(mtx::events::state::HistoryVisibility)
NHEKO_CACHE_GET_STATE_EVENT_FORWARD(mtx::events::state::JoinRules)
NHEKO_CACHE_GET_STATE_EVENT_FORWARD(mtx::events::state::Member)
NHEKO_CACHE_GET_STATE_EVENT_FORWARD(mtx::events::state::Name)
NHEKO_CACHE_GET_STATE_EVENT_FORWARD(mtx::events::state::PinnedEvents)
NHEKO_CACHE_GET_STATE_EVENT_FORWARD(mtx::events::state::PowerLevels)
NHEKO_CACHE_GET_STATE_EVENT_FORWARD(mtx::events::state::Tombstone)
NHEKO_CACHE_GET_STATE_EVENT_FORWARD(mtx::events::state::ServerAcl)
NHEKO_CACHE_GET_STATE_EVENT_FORWARD(mtx::events::state::Topic)
NHEKO_CACHE_GET_STATE_EVENT_FORWARD(mtx::events::state::Widget)
NHEKO_CACHE_GET_STATE_EVENT_FORWARD(mtx::events::state::policy_rule::UserRule)
NHEKO_CACHE_GET_STATE_EVENT_FORWARD(mtx::events::state::policy_rule::RoomRule)
NHEKO_CACHE_GET_STATE_EVENT_FORWARD(mtx::events::state::policy_rule::ServerRule)
NHEKO_CACHE_GET_STATE_EVENT_FORWARD(mtx::events::state::space::Child)
NHEKO_CACHE_GET_STATE_EVENT_FORWARD(mtx::events::state::space::Parent)
NHEKO_CACHE_GET_STATE_EVENT_FORWARD(mtx::events::msc2545::ImagePack)
