// SPDX-FileCopyrightText: 2017 Konstantinos Sideris <siderisk@auth.gr>
// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QDateTime>
#include <QString>

#if __has_include(<lmdbxx/lmdb++.h>)
#include <lmdbxx/lmdb++.h>
#else
#include <lmdb++.h>
#endif

#include <mtx/events/event_type.hpp>
#include <mtx/events/presence.hpp>
#include <mtx/responses/crypto.hpp>
#include <mtxclient/crypto/types.hpp>

#include "CacheCryptoStructs.h"
#include "CacheStructs.h"

namespace mtx::responses {
struct Notifications;
}

namespace cache {
void
init(const QString &user_id);

std::string
displayName(const std::string &room_id, const std::string &user_id);
QString
displayName(const QString &room_id, const QString &user_id);
QString
avatarUrl(const QString &room_id, const QString &user_id);

// presence
mtx::events::presence::Presence
presence(const std::string &user_id);

// user cache stores user keys
std::optional<UserKeyCache>
userKeys(const std::string &user_id);
void
updateUserKeys(const std::string &sync_token, const mtx::responses::QueryKeys &keyQuery);

// device & user verification cache
std::optional<VerificationStatus>
verificationStatus(const std::string &user_id);
void
markDeviceVerified(const std::string &user_id, const std::string &device);
void
markDeviceUnverified(const std::string &user_id, const std::string &device);

std::vector<std::string>
joinedRooms();

QMap<QString, RoomInfo>
roomInfo(bool withInvites = true);
QHash<QString, RoomInfo>
invites();

//! Calculate & return the name of the room.
QString
getRoomName(lmdb::txn &txn, lmdb::dbi &statesdb, lmdb::dbi &membersdb);
//! Get room join rules
mtx::events::state::JoinRule
getRoomJoinRule(lmdb::txn &txn, lmdb::dbi &statesdb);
bool
getRoomGuestAccess(lmdb::txn &txn, lmdb::dbi &statesdb);
//! Retrieve the topic of the room if any.
QString
getRoomTopic(lmdb::txn &txn, lmdb::dbi &statesdb);
//! Retrieve the room avatar's url if any.
QString
getRoomAvatarUrl(lmdb::txn &txn, lmdb::dbi &statesdb, lmdb::dbi &membersdb);

//! Retrieve member info from a room.
std::vector<RoomMember>
getMembers(const std::string &room_id, std::size_t startIndex = 0, std::size_t len = 30);
//! Retrive member info from an invite.
std::vector<RoomMember>
getMembersFromInvite(const std::string &room_id, std::size_t start_index = 0, std::size_t len = 30);

bool
isInitialized();

std::string
nextBatchToken();

void
deleteData();

void
removeInvite(lmdb::txn &txn, const std::string &room_id);
void
removeInvite(const std::string &room_id);
void
removeRoom(lmdb::txn &txn, const std::string &roomid);
void
removeRoom(const std::string &roomid);
void
removeRoom(const QString &roomid);
void
setup();

//! returns if the format is current, older or newer
cache::CacheVersion
formatVersion();
//! set the format version to the current version
void
setCurrentFormat();
//! migrates db to the current format
bool
runMigrations();

QMap<QString, mtx::responses::Notifications>
getTimelineMentions();

//! Retrieve all the user ids from a room.
std::vector<std::string>
roomMembers(const std::string &room_id);

//! Check if the given user has power level greater than than
//! lowest power level of the given events.
bool
hasEnoughPowerLevel(const std::vector<mtx::events::EventType> &eventTypes,
                    const std::string &room_id,
                    const std::string &user_id);

//! Adds a user to the read list for the given event.
//!
//! There should be only one user id present in a receipt list per room.
//! The user id should be removed from any other lists.
using Receipts = std::map<std::string, std::map<std::string, uint64_t>>;
void
updateReadReceipt(lmdb::txn &txn, const std::string &room_id, const Receipts &receipts);

//! Retrieve all the read receipts for the given event id and room.
//!
//! Returns a map of user ids and the time of the read receipt in milliseconds.
using UserReceipts = std::multimap<uint64_t, std::string, std::greater<uint64_t>>;
UserReceipts
readReceipts(const QString &event_id, const QString &room_id);

//! get index of the event in the event db, not representing the visual index
std::optional<uint64_t>
getEventIndex(const std::string &room_id, std::string_view event_id);
std::optional<std::pair<uint64_t, std::string>>
lastInvisibleEventAfter(const std::string &room_id, std::string_view event_id);

RoomInfo
singleRoomInfo(const std::string &room_id);
std::map<QString, RoomInfo>
getRoomInfo(const std::vector<std::string> &rooms);

//! Calculates which the read status of a room.
//! Whether all the events in the timeline have been read.
bool
calculateRoomReadStatus(const std::string &room_id);
void
calculateRoomReadStatus();

void
markSentNotification(const std::string &event_id);
//! Removes an event from the sent notifications.
void
removeReadNotification(const std::string &event_id);
//! Check if we have sent a desktop notification for the given event id.
bool
isNotificationSent(const std::string &event_id);

//! Add all notifications containing a user mention to the db.
void
saveTimelineMentions(const mtx::responses::Notifications &res);

//! Remove old unused data.
void
deleteOldMessages();
void
deleteOldData() noexcept;
//! Retrieve all saved room ids.
std::vector<std::string>
getRoomIds(lmdb::txn &txn);

//! Mark a room that uses e2e encryption.
void
setEncryptedRoom(lmdb::txn &txn, const std::string &room_id);
bool
isRoomEncrypted(const std::string &room_id);

//! Check if a user is a member of the room.
bool
isRoomMember(const std::string &user_id, const std::string &room_id);

//
// Outbound Megolm Sessions
//
void
saveOutboundMegolmSession(const std::string &room_id,
                          const GroupSessionData &data,
                          mtx::crypto::OutboundGroupSessionPtr &session);
OutboundGroupSessionDataRef
getOutboundMegolmSession(const std::string &room_id);
bool
outboundMegolmSessionExists(const std::string &room_id) noexcept;
void
updateOutboundMegolmSession(const std::string &room_id,
                            const GroupSessionData &data,
                            mtx::crypto::OutboundGroupSessionPtr &session);
void
dropOutboundMegolmSession(const std::string &room_id);

void
importSessionKeys(const mtx::crypto::ExportedSessionKeys &keys);
mtx::crypto::ExportedSessionKeys
exportSessionKeys();

//
// Inbound Megolm Sessions
//
void
saveInboundMegolmSession(const MegolmSessionIndex &index,
                         mtx::crypto::InboundGroupSessionPtr session,
                         const GroupSessionData &data);
mtx::crypto::InboundGroupSessionPtr
getInboundMegolmSession(const MegolmSessionIndex &index);
bool
inboundMegolmSessionExists(const MegolmSessionIndex &index);
std::optional<GroupSessionData>
getMegolmSessionData(const MegolmSessionIndex &index);

//
// Olm Sessions
//
void
saveOlmSession(const std::string &curve25519,
               mtx::crypto::OlmSessionPtr session,
               uint64_t timestamp);
std::vector<std::string>
getOlmSessions(const std::string &curve25519);
std::optional<mtx::crypto::OlmSessionPtr>
getOlmSession(const std::string &curve25519, const std::string &session_id);
std::optional<mtx::crypto::OlmSessionPtr>
getLatestOlmSession(const std::string &curve25519);

void
saveOlmAccount(const std::string &pickled);

std::string
restoreOlmAccount();

void
storeSecret(const std::string &name, const std::string &secret);
std::optional<std::string>
secret(const std::string &name);
}
