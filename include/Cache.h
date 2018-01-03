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
#include <json.hpp>
#include <lmdb++.h>
#include <mtx/responses.hpp>

class RoomState;

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

//! Decribes a read receipt stored in cache.
struct ReadReceiptValue
{
        std::string user_id;
        uint64_t ts;
};

inline void
to_json(json &j, const ReadReceiptValue &value)
{
        j = json{{"user_id", value.user_id}, {"ts", value.ts}};
}

inline void
from_json(const json &j, ReadReceiptValue &value)
{
        value.user_id = j.at("user_id").get<std::string>();
        value.ts      = j.at("ts").get<uint64_t>();
}

class Cache
{
public:
        Cache(const QString &userId);

        void setState(const QString &nextBatchToken, const QMap<QString, RoomState> &states);
        bool isInitialized() const;

        QString nextBatchToken() const;
        QMap<QString, RoomState> states();

        using Invites = std::map<std::string, mtx::responses::InvitedRoom>;
        Invites invites();
        void setInvites(const Invites &invites);

        void deleteData();
        void unmount() { isMounted_ = false; };

        void removeRoom(const QString &roomid);
        void removeInvite(const QString &roomid);
        void setup();

        bool isFormatValid();
        void setCurrentFormat();

        //! Adds a user to the read list for the given event.
        //!
        //! There should be only one user id present in a receipt list per room.
        //! The user id should be removed from any other lists.
        using Receipts = std::map<std::string, std::map<std::string, uint64_t>>;
        void updateReadReceipt(const std::string &room_id, const Receipts &receipts);

        //! Retrieve all the read receipts for the given event id and room.
        //!
        //! Returns a map of user ids and the time of the read receipt in milliseconds.
        using UserReceipts = std::multimap<uint64_t, std::string>;
        UserReceipts readReceipts(const QString &event_id, const QString &room_id);

        QByteArray image(const QString &url) const;
        void saveImage(const QString &url, const QByteArray &data);

private:
        void setNextBatchToken(lmdb::txn &txn, const QString &token);
        void insertRoomState(lmdb::txn &txn, const QString &roomid, const RoomState &state);

        lmdb::env env_;
        lmdb::dbi stateDb_;
        lmdb::dbi roomDb_;
        lmdb::dbi invitesDb_;
        lmdb::dbi imagesDb_;
        lmdb::dbi readReceiptsDb_;

        bool isMounted_;

        QString userId_;
        QString cacheDirectory_;
};
