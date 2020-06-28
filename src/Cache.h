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

#include <QDateTime>
#include <QDir>
#include <QImage>
#include <QString>

#if __has_include(<lmdbxx/lmdb++.h>)
#include <lmdbxx/lmdb++.h>
#else
#include <lmdb++.h>
#endif

#include <mtx/responses.hpp>

#include "CacheCryptoStructs.h"
#include "CacheStructs.h"

namespace cache {
void
init(const QString &user_id);

std::string
displayName(const std::string &room_id, const std::string &user_id);
QString
displayName(const QString &room_id, const QString &user_id);
QString
avatarUrl(const QString &room_id, const QString &user_id);

void
removeDisplayName(const QString &room_id, const QString &user_id);
void
removeAvatarUrl(const QString &room_id, const QString &user_id);

void
insertDisplayName(const QString &room_id, const QString &user_id, const QString &display_name);
void
insertAvatarUrl(const QString &room_id, const QString &user_id, const QString &avatar_url);

// presence
mtx::presence::PresenceState
presenceState(const std::string &user_id);
std::string
statusMessage(const std::string &user_id);

//! user Cache
UserCache
getUserCache(const std::string &user_id);

int
setUserCache(const std::string &user_id, const UserCache &body);

int
deleteUserCache(const std::string &user_id);

//! verified Cache
DeviceVerifiedCache
getVerifiedCache(const std::string &user_id);

int
setVerifiedCache(const std::string &user_id, const DeviceVerifiedCache &body);

//! Load saved data for the display names & avatars.
void
populateMembers();
std::vector<std::string>
joinedRooms();

QMap<QString, RoomInfo>
roomInfo(bool withInvites = true);
std::map<QString, bool>
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
getRoomAvatarUrl(lmdb::txn &txn, lmdb::dbi &statesdb, lmdb::dbi &membersdb, const QString &room_id);
//! Retrieve the version of the room if any.
QString
getRoomVersion(lmdb::txn &txn, lmdb::dbi &statesdb);

//! Retrieve member info from a room.
std::vector<RoomMember>
getMembers(const std::string &room_id, std::size_t startIndex = 0, std::size_t len = 30);

void
saveState(const mtx::responses::Sync &res);
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

std::map<QString, mtx::responses::Timeline>
roomMessages();

QMap<QString, mtx::responses::Notifications>
getTimelineMentions();

//! Retrieve all the user ids from a room.
std::vector<std::string>
roomMembers(const std::string &room_id);

//! Check if the given user has power leve greater than than
//! lowest power level of the given events.
bool
hasEnoughPowerLevel(const std::vector<mtx::events::EventType> &eventTypes,
                    const std::string &room_id,
                    const std::string &user_id);

//! Retrieves the saved room avatar.
QImage
getRoomAvatar(const QString &id);
QImage
getRoomAvatar(const std::string &id);

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

QByteArray
image(const QString &url);
QByteArray
image(lmdb::txn &txn, const std::string &url);
inline QByteArray
image(const std::string &url)
{
        return image(QString::fromStdString(url));
}
void
saveImage(const std::string &url, const std::string &data);
void
saveImage(const QString &url, const QByteArray &data);

RoomInfo
singleRoomInfo(const std::string &room_id);
std::vector<std::string>
roomsWithStateUpdates(const mtx::responses::Sync &res);
std::vector<std::string>
roomsWithTagUpdates(const mtx::responses::Sync &res);
std::map<QString, RoomInfo>
getRoomInfo(const std::vector<std::string> &rooms);
inline std::map<QString, RoomInfo>
roomUpdates(const mtx::responses::Sync &sync)
{
        return getRoomInfo(roomsWithStateUpdates(sync));
}
inline std::map<QString, RoomInfo>
roomTagUpdates(const mtx::responses::Sync &sync)
{
        return getRoomInfo(roomsWithTagUpdates(sync));
}

//! Calculates which the read status of a room.
//! Whether all the events in the timeline have been read.
bool
calculateRoomReadStatus(const std::string &room_id);
void
calculateRoomReadStatus();

std::vector<SearchResult>
searchUsers(const std::string &room_id, const std::string &query, std::uint8_t max_items = 5);
std::vector<RoomSearchResult>
searchRooms(const std::string &query, std::uint8_t max_items = 5);

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
                          const OutboundGroupSessionData &data,
                          mtx::crypto::OutboundGroupSessionPtr session);
OutboundGroupSessionDataRef
getOutboundMegolmSession(const std::string &room_id);
bool
outboundMegolmSessionExists(const std::string &room_id) noexcept;
void
updateOutboundMegolmSession(const std::string &room_id, int message_index);

void
importSessionKeys(const mtx::crypto::ExportedSessionKeys &keys);
mtx::crypto::ExportedSessionKeys
exportSessionKeys();

//
// Inbound Megolm Sessions
//
void
saveInboundMegolmSession(const MegolmSessionIndex &index,
                         mtx::crypto::InboundGroupSessionPtr session);
OlmInboundGroupSession *
getInboundMegolmSession(const MegolmSessionIndex &index);
bool
inboundMegolmSessionExists(const MegolmSessionIndex &index);

//
// Olm Sessions
//
void
saveOlmSession(const std::string &curve25519, mtx::crypto::OlmSessionPtr session);
std::vector<std::string>
getOlmSessions(const std::string &curve25519);
std::optional<mtx::crypto::OlmSessionPtr>
getOlmSession(const std::string &curve25519, const std::string &session_id);

void
saveOlmAccount(const std::string &pickled);
std::string
restoreOlmAccount();

void
restoreSessions();
}
