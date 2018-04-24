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

#include <QDebug>
#include <QDir>
#include <json.hpp>
#include <lmdb++.h>
#include <mtx/responses.hpp>

#include "Utils.h"

struct SearchResult
{
        QString user_id;
        QString display_name;
};

Q_DECLARE_METATYPE(SearchResult)
Q_DECLARE_METATYPE(QVector<SearchResult>)

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
};

inline void
to_json(json &j, const RoomInfo &info)
{
        j["name"]       = info.name;
        j["topic"]      = info.topic;
        j["avatar_url"] = info.avatar_url;
        j["is_invite"]  = info.is_invite;
}

inline void
from_json(const json &j, RoomInfo &info)
{
        info.name       = j.at("name");
        info.topic      = j.at("topic");
        info.avatar_url = j.at("avatar_url");
        info.is_invite  = j.at("is_invite");
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

Q_DECLARE_METATYPE(RoomInfo)

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
        //! Retrieve the topic of the room if any.
        QString getRoomTopic(lmdb::txn &txn, lmdb::dbi &statesdb);
        //! Retrieve the room avatar's url if any.
        QString getRoomAvatarUrl(lmdb::txn &txn,
                                 lmdb::dbi &statesdb,
                                 lmdb::dbi &membersdb,
                                 const QString &room_id);

        void saveState(const mtx::responses::Sync &res);
        bool isInitialized() const;

        QString nextBatchToken() const;

        void deleteData();

        void removeInvite(lmdb::txn &txn, const std::string &room_id);
        void removeInvite(const std::string &room_id);
        void removeRoom(lmdb::txn &txn, const std::string &roomid);
        void removeRoom(const std::string &roomid);
        void removeRoom(const QString &roomid) { removeRoom(roomid.toStdString()); };
        void setup();

        bool isFormatValid();
        void setCurrentFormat();

        //! Retrieves the saved room avatar.
        QImage getRoomAvatar(const QString &id);

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
        void saveImage(const QString &url, const QByteArray &data);

        std::vector<std::string> roomsWithStateUpdates(const mtx::responses::Sync &res);
        std::map<QString, RoomInfo> getRoomInfo(const std::vector<std::string> &rooms);
        std::map<QString, RoomInfo> roomUpdates(const mtx::responses::Sync &sync)
        {
                return getRoomInfo(roomsWithStateUpdates(sync));
        }

        QVector<SearchResult> getAutocompleteMatches(const std::string &room_id,
                                                     const std::string &query,
                                                     std::uint8_t max_items = 5);

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

        QString localUserId_;
        QString cacheDirectory_;
};
