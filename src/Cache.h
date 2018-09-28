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

#include <boost/optional.hpp>

#include <QDateTime>
#include <QDir>
#include <QImage>
#include <QString>

#include <json.hpp>
#include <lmdb++.h>
#include <mtx/events/join_rules.hpp>
#include <mtx/responses.hpp>
#include <mtxclient/crypto/client.hpp>
#include <mutex>

#include "Logging.h"

using mtx::events::state::JoinRule;

struct RoomMember
{
        QString user_id;
        QString display_name;
        QImage avatar;
};

struct SearchResult
{
        QString user_id;
        QString display_name;
};

static int
numeric_key_comparison(const MDB_val *a, const MDB_val *b)
{
        auto lhs = std::stoull(std::string((char *)a->mv_data, a->mv_size));
        auto rhs = std::stoull(std::string((char *)b->mv_data, b->mv_size));

        if (lhs < rhs)
                return 1;
        else if (lhs == rhs)
                return 0;

        return -1;
}

Q_DECLARE_METATYPE(SearchResult)
Q_DECLARE_METATYPE(QVector<SearchResult>)
Q_DECLARE_METATYPE(RoomMember)
Q_DECLARE_METATYPE(mtx::responses::Timeline)

//! Used to uniquely identify a list of read receipts.
struct ReadReceiptKey
{
        std::string event_id;
        std::string room_id;
};

inline void
to_json(json &j, const ReadReceiptKey &key)
{
        j = json{{"event_id", key.event_id}, {"room_id", key.room_id}};
}

inline void
from_json(const json &j, ReadReceiptKey &key)
{
        key.event_id = j.at("event_id").get<std::string>();
        key.room_id  = j.at("room_id").get<std::string>();
}

struct DescInfo
{
        QString event_id;
        QString username;
        QString userid;
        QString body;
        QString timestamp;
        QDateTime datetime;
};

//! UI info associated with a room.
struct RoomInfo
{
        //! The calculated name of the room.
        std::string name;
        //! The topic of the room.
        std::string topic;
        //! The calculated avatar url of the room.
        std::string avatar_url;
        //! Whether or not the room is an invite.
        bool is_invite = false;
        //! Total number of members in the room.
        int16_t member_count = 0;
        //! Who can access to the room.
        JoinRule join_rule = JoinRule::Public;
        bool guest_access  = false;
        //! Metadata describing the last message in the timeline.
        DescInfo msgInfo;
        //! The list of tags associated with this room
        std::vector<std::string> tags;
};

inline void
to_json(json &j, const RoomInfo &info)
{
        j["name"]         = info.name;
        j["topic"]        = info.topic;
        j["avatar_url"]   = info.avatar_url;
        j["is_invite"]    = info.is_invite;
        j["join_rule"]    = info.join_rule;
        j["guest_access"] = info.guest_access;

        if (info.member_count != 0)
                j["member_count"] = info.member_count;

        if (info.tags.size() != 0)
                j["tags"] = info.tags;
}

inline void
from_json(const json &j, RoomInfo &info)
{
        info.name         = j.at("name");
        info.topic        = j.at("topic");
        info.avatar_url   = j.at("avatar_url");
        info.is_invite    = j.at("is_invite");
        info.join_rule    = j.at("join_rule");
        info.guest_access = j.at("guest_access");

        if (j.count("member_count"))
                info.member_count = j.at("member_count");

        if (j.count("tags"))
                info.tags = j.at("tags").get<std::vector<std::string>>();
}

//! Basic information per member;
struct MemberInfo
{
        std::string name;
        std::string avatar_url;
};

inline void
to_json(json &j, const MemberInfo &info)
{
        j["name"]       = info.name;
        j["avatar_url"] = info.avatar_url;
}

inline void
from_json(const json &j, MemberInfo &info)
{
        info.name       = j.at("name");
        info.avatar_url = j.at("avatar_url");
}

struct RoomSearchResult
{
        std::string room_id;
        RoomInfo info;
        QImage img;
};

Q_DECLARE_METATYPE(RoomSearchResult)
Q_DECLARE_METATYPE(RoomInfo)

// Extra information associated with an outbound megolm session.
struct OutboundGroupSessionData
{
        std::string session_id;
        std::string session_key;
        uint64_t message_index = 0;
};

inline void
to_json(nlohmann::json &obj, const OutboundGroupSessionData &msg)
{
        obj["session_id"]    = msg.session_id;
        obj["session_key"]   = msg.session_key;
        obj["message_index"] = msg.message_index;
}

inline void
from_json(const nlohmann::json &obj, OutboundGroupSessionData &msg)
{
        msg.session_id    = obj.at("session_id");
        msg.session_key   = obj.at("session_key");
        msg.message_index = obj.at("message_index");
}

struct OutboundGroupSessionDataRef
{
        OlmOutboundGroupSession *session;
        OutboundGroupSessionData data;
};

struct DevicePublicKeys
{
        std::string ed25519;
        std::string curve25519;
};

inline void
to_json(nlohmann::json &obj, const DevicePublicKeys &msg)
{
        obj["ed25519"]    = msg.ed25519;
        obj["curve25519"] = msg.curve25519;
}

inline void
from_json(const nlohmann::json &obj, DevicePublicKeys &msg)
{
        msg.ed25519    = obj.at("ed25519");
        msg.curve25519 = obj.at("curve25519");
}

//! Represents a unique megolm session identifier.
struct MegolmSessionIndex
{
        //! The room in which this session exists.
        std::string room_id;
        //! The session_id of the megolm session.
        std::string session_id;
        //! The curve25519 public key of the sender.
        std::string sender_key;
};

inline void
to_json(nlohmann::json &obj, const MegolmSessionIndex &msg)
{
        obj["room_id"]    = msg.room_id;
        obj["session_id"] = msg.session_id;
        obj["sender_key"] = msg.sender_key;
}

inline void
from_json(const nlohmann::json &obj, MegolmSessionIndex &msg)
{
        msg.room_id    = obj.at("room_id");
        msg.session_id = obj.at("session_id");
        msg.sender_key = obj.at("sender_key");
}

struct OlmSessionStorage
{
        // Megolm sessions
        std::map<std::string, mtx::crypto::InboundGroupSessionPtr> group_inbound_sessions;
        std::map<std::string, mtx::crypto::OutboundGroupSessionPtr> group_outbound_sessions;
        std::map<std::string, OutboundGroupSessionData> group_outbound_session_data;

        // Guards for accessing megolm sessions.
        std::mutex group_outbound_mtx;
        std::mutex group_inbound_mtx;
};

class Cache : public QObject
{
        Q_OBJECT

public:
        Cache(const QString &userId, QObject *parent = nullptr);

        static QHash<QString, QString> DisplayNames;
        static QHash<QString, QString> AvatarUrls;

        static std::string displayName(const std::string &room_id, const std::string &user_id);
        static QString displayName(const QString &room_id, const QString &user_id);
        static QString avatarUrl(const QString &room_id, const QString &user_id);

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
        JoinRule getRoomJoinRule(lmdb::txn &txn, lmdb::dbi &statesdb);
        bool getRoomGuestAccess(lmdb::txn &txn, lmdb::dbi &statesdb);
        //! Retrieve the topic of the room if any.
        QString getRoomTopic(lmdb::txn &txn, lmdb::dbi &statesdb);
        //! Retrieve the room avatar's url if any.
        QString getRoomAvatarUrl(lmdb::txn &txn,
                                 lmdb::dbi &statesdb,
                                 lmdb::dbi &membersdb,
                                 const QString &room_id);

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
        void removeRoom(const QString &roomid) { removeRoom(roomid.toStdString()); };
        void setup();

        bool isFormatValid();
        void setCurrentFormat();

        std::map<QString, mtx::responses::Timeline> roomMessages();

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

        //! Filter the events that have at least one read receipt.
        std::vector<QString> filterReadEvents(const QString &room_id,
                                              const std::vector<QString> &event_ids,
                                              const std::string &excluded_user);
        //! Add event for which we are expecting some read receipts.
        void addPendingReceipt(const QString &room_id, const QString &event_id);
        void removePendingReceipt(lmdb::txn &txn,
                                  const std::string &room_id,
                                  const std::string &event_id);
        void notifyForReadReceipts(const std::string &room_id);
        std::vector<QString> pendingReceiptsEvents(lmdb::txn &txn, const std::string &room_id);

        QByteArray image(const QString &url) const;
        QByteArray image(lmdb::txn &txn, const std::string &url) const;
        QByteArray image(const std::string &url) const
        {
                return image(QString::fromStdString(url));
        }
        void saveImage(const std::string &url, const std::string &data);
        void saveImage(const QString &url, const QByteArray &data);

        RoomInfo singleRoomInfo(const std::string &room_id);
        std::vector<std::string> roomsWithStateUpdates(const mtx::responses::Sync &res);
        std::vector<std::string> roomsWithTagUpdates(const mtx::responses::Sync &res);
        std::map<QString, RoomInfo> getRoomInfo(const std::vector<std::string> &rooms);
        std::map<QString, RoomInfo> roomUpdates(const mtx::responses::Sync &sync)
        {
                return getRoomInfo(roomsWithStateUpdates(sync));
        }
        std::map<QString, RoomInfo> roomTagUpdates(const mtx::responses::Sync &sync)
        {
                return getRoomInfo(roomsWithTagUpdates(sync));
        }

        //! Calculates which the read status of a room.
        //! Whether all the events in the timeline have been read.
        bool calculateRoomReadStatus(const std::string &room_id);
        void calculateRoomReadStatus();

        QVector<SearchResult> searchUsers(const std::string &room_id,
                                          const std::string &query,
                                          std::uint8_t max_items = 5);
        std::vector<RoomSearchResult> searchRooms(const std::string &query,
                                                  std::uint8_t max_items = 5);

        void markSentNotification(const std::string &event_id);
        //! Removes an event from the sent notifications.
        void removeReadNotification(const std::string &event_id);
        //! Check if we have sent a desktop notification for the given event id.
        bool isNotificationSent(const std::string &event_id);

        //! Remove old unused data.
        void deleteOldMessages();
        void deleteOldData() noexcept;
        //! Retrieve all saved room ids.
        std::vector<std::string> getRoomIds(lmdb::txn &txn);

        //! Mark a room that uses e2e encryption.
        void setEncryptedRoom(lmdb::txn &txn, const std::string &room_id);
        bool isRoomEncrypted(const std::string &room_id);

        //! Save the public keys for a device.
        void saveDeviceKeys(const std::string &device_id);
        void getDeviceKeys(const std::string &device_id);

        //! Save the device list for a user.
        void setDeviceList(const std::string &user_id, const std::vector<std::string> &devices);
        std::vector<std::string> getDeviceList(const std::string &user_id);

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
        boost::optional<mtx::crypto::OlmSessionPtr> getOlmSession(const std::string &curve25519,
                                                                  const std::string &session_id);

        void saveOlmAccount(const std::string &pickled);
        std::string restoreOlmAccount();

        void restoreSessions();

        OlmSessionStorage session_storage;

signals:
        void newReadReceipts(const QString &room_id, const std::vector<QString> &event_ids);
        void roomReadStatus(const std::map<QString, bool> &status);

private:
        //! Save an invited room.
        void saveInvite(lmdb::txn &txn,
                        lmdb::dbi &statesdb,
                        lmdb::dbi &membersdb,
                        const mtx::responses::InvitedRoom &room);

        QString getInviteRoomName(lmdb::txn &txn, lmdb::dbi &statesdb, lmdb::dbi &membersdb);
        QString getInviteRoomTopic(lmdb::txn &txn, lmdb::dbi &statesdb);
        QString getInviteRoomAvatarUrl(lmdb::txn &txn, lmdb::dbi &statesdb, lmdb::dbi &membersdb);

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

                if (boost::get<StateEvent<Member>>(&event) != nullptr) {
                        const auto e = boost::get<StateEvent<Member>>(event);

                        switch (e.content.membership) {
                        //
                        // We only keep users with invite or join membership.
                        //
                        case Membership::Invite:
                        case Membership::Join: {
                                auto display_name = e.content.display_name.empty()
                                                      ? e.state_key
                                                      : e.content.display_name;

                                // Lightweight representation of a member.
                                MemberInfo tmp{display_name, e.content.avatar_url};

                                lmdb::dbi_put(txn,
                                              membersdb,
                                              lmdb::val(e.state_key),
                                              lmdb::val(json(tmp).dump()));

                                insertDisplayName(QString::fromStdString(room_id),
                                                  QString::fromStdString(e.state_key),
                                                  QString::fromStdString(display_name));

                                insertAvatarUrl(QString::fromStdString(room_id),
                                                QString::fromStdString(e.state_key),
                                                QString::fromStdString(e.content.avatar_url));

                                break;
                        }
                        default: {
                                lmdb::dbi_del(
                                  txn, membersdb, lmdb::val(e.state_key), lmdb::val(""));

                                removeDisplayName(QString::fromStdString(room_id),
                                                  QString::fromStdString(e.state_key));
                                removeAvatarUrl(QString::fromStdString(room_id),
                                                QString::fromStdString(e.state_key));

                                break;
                        }
                        }

                        return;
                } else if (boost::get<StateEvent<Encryption>>(&event) != nullptr) {
                        setEncryptedRoom(txn, room_id);
                        return;
                }

                if (!isStateEvent(event))
                        return;

                boost::apply_visitor(
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

                return boost::get<StateEvent<Aliases>>(&e) != nullptr ||
                       boost::get<StateEvent<state::Avatar>>(&e) != nullptr ||
                       boost::get<StateEvent<CanonicalAlias>>(&e) != nullptr ||
                       boost::get<StateEvent<Create>>(&e) != nullptr ||
                       boost::get<StateEvent<GuestAccess>>(&e) != nullptr ||
                       boost::get<StateEvent<HistoryVisibility>>(&e) != nullptr ||
                       boost::get<StateEvent<JoinRules>>(&e) != nullptr ||
                       boost::get<StateEvent<Name>>(&e) != nullptr ||
                       boost::get<StateEvent<Member>>(&e) != nullptr ||
                       boost::get<StateEvent<PowerLevels>>(&e) != nullptr ||
                       boost::get<StateEvent<Topic>>(&e) != nullptr;
        }

        template<class T>
        bool containsStateUpdates(const T &e)
        {
                using namespace mtx::events;
                using namespace mtx::events::state;

                return boost::get<StateEvent<state::Avatar>>(&e) != nullptr ||
                       boost::get<StateEvent<CanonicalAlias>>(&e) != nullptr ||
                       boost::get<StateEvent<Name>>(&e) != nullptr ||
                       boost::get<StateEvent<Member>>(&e) != nullptr ||
                       boost::get<StateEvent<Topic>>(&e) != nullptr;
        }

        bool containsStateUpdates(const mtx::events::collections::StrippedEvents &e)
        {
                using namespace mtx::events;
                using namespace mtx::events::state;

                return boost::get<StrippedEvent<state::Avatar>>(&e) != nullptr ||
                       boost::get<StrippedEvent<CanonicalAlias>>(&e) != nullptr ||
                       boost::get<StrippedEvent<Name>>(&e) != nullptr ||
                       boost::get<StrippedEvent<Member>>(&e) != nullptr ||
                       boost::get<StrippedEvent<Topic>>(&e) != nullptr;
        }

        void saveInvites(lmdb::txn &txn,
                         const std::map<std::string, mtx::responses::InvitedRoom> &rooms);

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
};

namespace cache {
void
init(const QString &user_id);

Cache *
client();
}
