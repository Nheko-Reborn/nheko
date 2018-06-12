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

#include <QDir>
#include <QImage>

#include <json.hpp>
#include <lmdb++.h>
#include <mtx/events/join_rules.hpp>
#include <mtx/responses.hpp>
#include <mtxclient/crypto/client.hpp>
#include <mutex>

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

Q_DECLARE_METATYPE(SearchResult)
Q_DECLARE_METATYPE(QVector<SearchResult>)
Q_DECLARE_METATYPE(RoomMember)

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

        //! Representation to be used in a hash map.
        std::string to_hash() const { return room_id + session_id + sender_key; }
};

struct OlmSessionStorage
{
        std::map<std::string, mtx::crypto::OlmSessionPtr> outbound_sessions;
        std::map<std::string, mtx::crypto::InboundGroupSessionPtr> group_inbound_sessions;
        std::map<std::string, mtx::crypto::OutboundGroupSessionPtr> group_outbound_sessions;
        std::map<std::string, OutboundGroupSessionData> group_outbound_session_data;

        // Guards for accessing critical data.
        std::mutex outbound_mtx;
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
        QByteArray image(const std::string &url) const
        {
                return image(QString::fromStdString(url));
        }
        void saveImage(const std::string &url, const std::string &data);
        void saveImage(const QString &url, const QByteArray &data);

        RoomInfo singleRoomInfo(const std::string &room_id);
        std::vector<std::string> roomsWithStateUpdates(const mtx::responses::Sync &res);
        std::map<QString, RoomInfo> getRoomInfo(const std::vector<std::string> &rooms);
        std::map<QString, RoomInfo> roomUpdates(const mtx::responses::Sync &sync)
        {
                return getRoomInfo(roomsWithStateUpdates(sync));
        }

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

        //! Mark a room that uses e2e encryption.
        void setEncryptedRoom(const std::string &room_id);
        bool isRoomEncrypted(const std::string &room_id);

        //! Save the public keys for a device.
        void saveDeviceKeys(const std::string &device_id);
        void getDeviceKeys(const std::string &device_id);

        //! Save the device list for a user.
        void setDeviceList(const std::string &user_id, const std::vector<std::string> &devices);
        std::vector<std::string> getDeviceList(const std::string &user_id);

        //
        // Outbound Megolm Sessions
        //
        void saveOutboundMegolmSession(const MegolmSessionIndex &index,
                                       const OutboundGroupSessionData &data,
                                       mtx::crypto::OutboundGroupSessionPtr session);
        OutboundGroupSessionDataRef getOutboundMegolmSession(const MegolmSessionIndex &index);
        bool outboundMegolmSessionExists(const MegolmSessionIndex &index) noexcept;

        //
        // Inbound Megolm Sessions
        //
        void saveInboundMegolmSession(const MegolmSessionIndex &index,
                                      mtx::crypto::InboundGroupSessionPtr session);
        OlmInboundGroupSession *getInboundMegolmSession(const MegolmSessionIndex &index);
        bool inboundMegolmSessionExists(const MegolmSessionIndex &index) noexcept;

        //
        // Outbound Olm Sessions
        //
        void saveOutboundOlmSession(const std::string &curve25519,
                                    mtx::crypto::OlmSessionPtr session);
        OlmSession *getOutboundOlmSession(const std::string &curve25519);
        bool outboundOlmSessionsExists(const std::string &curve25519) noexcept;

        void saveOlmAccount(const std::string &pickled);
        std::string restoreOlmAccount();

        void restoreSessions();

        OlmSessionStorage session_storage;

private:
        //! Save an invited room.
        void saveInvite(lmdb::txn &txn,
                        lmdb::dbi &statesdb,
                        lmdb::dbi &membersdb,
                        const mtx::responses::InvitedRoom &room);

        QString getInviteRoomName(lmdb::txn &txn, lmdb::dbi &statesdb, lmdb::dbi &membersdb);
        QString getInviteRoomTopic(lmdb::txn &txn, lmdb::dbi &statesdb);
        QString getInviteRoomAvatarUrl(lmdb::txn &txn, lmdb::dbi &statesdb, lmdb::dbi &membersdb);

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

                if (mpark::holds_alternative<StateEvent<Member>>(event)) {
                        const auto e = mpark::get<StateEvent<Member>>(event);

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
                }

                if (!isStateEvent(event))
                        return;

                mpark::visit(
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

                return mpark::holds_alternative<StateEvent<Aliases>>(e) ||
                       mpark::holds_alternative<StateEvent<state::Avatar>>(e) ||
                       mpark::holds_alternative<StateEvent<CanonicalAlias>>(e) ||
                       mpark::holds_alternative<StateEvent<Create>>(e) ||
                       mpark::holds_alternative<StateEvent<GuestAccess>>(e) ||
                       mpark::holds_alternative<StateEvent<HistoryVisibility>>(e) ||
                       mpark::holds_alternative<StateEvent<JoinRules>>(e) ||
                       mpark::holds_alternative<StateEvent<Name>>(e) ||
                       mpark::holds_alternative<StateEvent<PowerLevels>>(e) ||
                       mpark::holds_alternative<StateEvent<Topic>>(e);
        }

        template<class T>
        bool containsStateUpdates(const T &e)
        {
                using namespace mtx::events;
                using namespace mtx::events::state;

                return mpark::holds_alternative<StateEvent<state::Avatar>>(e) ||
                       mpark::holds_alternative<StateEvent<CanonicalAlias>>(e) ||
                       mpark::holds_alternative<StateEvent<Name>>(e) ||
                       mpark::holds_alternative<StateEvent<Member>>(e) ||
                       mpark::holds_alternative<StateEvent<Topic>>(e);
        }

        bool containsStateUpdates(const mtx::events::collections::StrippedEvents &e)
        {
                using namespace mtx::events;
                using namespace mtx::events::state;

                return mpark::holds_alternative<StrippedEvent<state::Avatar>>(e) ||
                       mpark::holds_alternative<StrippedEvent<CanonicalAlias>>(e) ||
                       mpark::holds_alternative<StrippedEvent<Name>>(e) ||
                       mpark::holds_alternative<StrippedEvent<Member>>(e) ||
                       mpark::holds_alternative<StrippedEvent<Topic>>(e);
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
        lmdb::dbi outboundOlmSessionDb_;

        QString localUserId_;
        QString cacheDirectory_;
};

namespace cache {
void
init(const QString &user_id);

Cache *
client();
}
