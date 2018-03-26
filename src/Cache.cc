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

#include <stdexcept>

#include <QByteArray>
#include <QDebug>
#include <QFile>
#include <QStandardPaths>

#include <variant.hpp>

#include "Cache.h"
#include "RoomState.h"

static const std::string CURRENT_CACHE_FORMAT_VERSION("2018.01.14");

static const lmdb::val NEXT_BATCH_KEY("next_batch");
static const lmdb::val CACHE_FORMAT_VERSION_KEY("cache_format_version");

using CachedReceipts = std::multimap<uint64_t, std::string, std::greater<uint64_t>>;
using Receipts       = std::map<std::string, std::map<std::string, uint64_t>>;

Cache::Cache(const QString &userId, QObject *parent)
  : QObject{parent}
  , env_{nullptr}
  , stateDb_{0}
  , roomDb_{0}
  , invitesDb_{0}
  , imagesDb_{0}
  , readReceiptsDb_{0}
  , isMounted_{false}
  , userId_{userId}
{}

void
Cache::setup()
{
        qDebug() << "Setting up cache";

        auto statePath = QString("%1/%2/state")
                           .arg(QStandardPaths::writableLocation(QStandardPaths::CacheLocation))
                           .arg(QString::fromUtf8(userId_.toUtf8().toHex()));

        cacheDirectory_ = QString("%1/%2")
                            .arg(QStandardPaths::writableLocation(QStandardPaths::CacheLocation))
                            .arg(QString::fromUtf8(userId_.toUtf8().toHex()));

        bool isInitial = !QFile::exists(statePath);

        env_ = lmdb::env::create();
        env_.set_mapsize(256UL * 1024UL * 1024UL); /* 256 MB */
        env_.set_max_dbs(1024UL);

        if (isInitial) {
                qDebug() << "First time initializing LMDB";

                if (!QDir().mkpath(statePath)) {
                        throw std::runtime_error(
                          ("Unable to create state directory:" + statePath).toStdString().c_str());
                }
        }

        try {
                env_.open(statePath.toStdString().c_str());
        } catch (const lmdb::error &e) {
                if (e.code() != MDB_VERSION_MISMATCH && e.code() != MDB_INVALID) {
                        throw std::runtime_error("LMDB initialization failed" +
                                                 std::string(e.what()));
                }

                qWarning() << "Resetting cache due to LMDB version mismatch:" << e.what();

                QDir stateDir(statePath);

                for (const auto &file : stateDir.entryList(QDir::NoDotAndDotDot)) {
                        if (!stateDir.remove(file))
                                throw std::runtime_error(
                                  ("Unable to delete file " + file).toStdString().c_str());
                }

                env_.open(statePath.toStdString().c_str());
        }

        auto txn        = lmdb::txn::begin(env_);
        stateDb_        = lmdb::dbi::open(txn, "state", MDB_CREATE);
        roomDb_         = lmdb::dbi::open(txn, "rooms", MDB_CREATE);
        invitesDb_      = lmdb::dbi::open(txn, "invites", MDB_CREATE);
        imagesDb_       = lmdb::dbi::open(txn, "images", MDB_CREATE);
        readReceiptsDb_ = lmdb::dbi::open(txn, "read_receipts", MDB_CREATE);

        txn.commit();

        isMounted_ = true;
}

void
Cache::saveImage(const QString &url, const QByteArray &image)
{
        if (!isMounted_)
                return;

        auto key = url.toUtf8();

        try {
                auto txn = lmdb::txn::begin(env_);

                lmdb::dbi_put(txn,
                              imagesDb_,
                              lmdb::val(key.data(), key.size()),
                              lmdb::val(image.data(), image.size()));

                txn.commit();
        } catch (const lmdb::error &e) {
                qCritical() << "saveImage:" << e.what();
        }
}

QByteArray
Cache::image(const QString &url) const
{
        auto key = url.toUtf8();

        try {
                auto txn = lmdb::txn::begin(env_, nullptr, MDB_RDONLY);

                lmdb::val image;

                bool res = lmdb::dbi_get(txn, imagesDb_, lmdb::val(key.data(), key.size()), image);

                txn.commit();

                if (!res)
                        return QByteArray();

                return QByteArray(image.data(), image.size());
        } catch (const lmdb::error &e) {
                qCritical() << "image:" << e.what();
        }

        return QByteArray();
}

void
Cache::setState(const QString &nextBatchToken,
                const std::map<QString, QSharedPointer<RoomState>> &states)
{
        if (!isMounted_)
                return;

        try {
                auto txn = lmdb::txn::begin(env_);

                setNextBatchToken(txn, nextBatchToken);

                for (auto const &state : states)
                        insertRoomState(txn, state.first, state.second);

                txn.commit();
        } catch (const lmdb::error &e) {
                qCritical() << "The cache couldn't be updated: " << e.what();

                unmount();
                deleteData();
        }
}

void
Cache::insertRoomState(lmdb::txn &txn,
                       const QString &roomid,
                       const QSharedPointer<RoomState> &state)
{
        auto stateEvents = state->serialize();
        auto id          = roomid.toUtf8();

        lmdb::dbi_put(txn, roomDb_, lmdb::val(id.data(), id.size()), lmdb::val(stateEvents));

        for (const auto &membership : state->memberships) {
                lmdb::dbi membersDb =
                  lmdb::dbi::open(txn, roomid.toStdString().c_str(), MDB_CREATE);

                // The user_id this membership event relates to, is used
                // as the index on the membership database.
                auto key = membership.second.state_key;

                // Serialize membership event.
                nlohmann::json data     = membership.second;
                std::string memberEvent = data.dump();

                switch (membership.second.content.membership) {
                // We add or update (e.g invite -> join) a new user to the membership
                // list.
                case mtx::events::state::Membership::Invite:
                case mtx::events::state::Membership::Join: {
                        lmdb::dbi_put(txn, membersDb, lmdb::val(key), lmdb::val(memberEvent));
                        break;
                }
                // We remove the user from the membership list.
                case mtx::events::state::Membership::Leave:
                case mtx::events::state::Membership::Ban: {
                        lmdb::dbi_del(txn, membersDb, lmdb::val(key), lmdb::val(memberEvent));
                        break;
                }
                case mtx::events::state::Membership::Knock: {
                        qWarning()
                          << "Skipping knock membership" << roomid << QString::fromStdString(key);
                        break;
                }
                }
        }
}

void
Cache::removeRoom(const QString &roomid)
{
        if (!isMounted_)
                return;

        auto txn = lmdb::txn::begin(env_, nullptr, 0);

        lmdb::dbi_del(txn, roomDb_, lmdb::val(roomid.toUtf8(), roomid.toUtf8().size()), nullptr);

        txn.commit();
}

void
Cache::removeInvite(const QString &room_id)
{
        if (!isMounted_)
                return;

        auto txn = lmdb::txn::begin(env_, nullptr, 0);

        lmdb::dbi_del(
          txn, invitesDb_, lmdb::val(room_id.toUtf8(), room_id.toUtf8().size()), nullptr);

        txn.commit();
}

void
Cache::states()
{
        std::map<QString, RoomState> states;

        auto txn    = lmdb::txn::begin(env_, nullptr, MDB_RDONLY);
        auto cursor = lmdb::cursor::open(txn, roomDb_);

        std::string room;
        std::string stateData;

        // Retrieve all the room names.
        while (cursor.get(room, stateData, MDB_NEXT)) {
                auto roomid = QString::fromUtf8(room.data(), room.size());
                auto json   = nlohmann::json::parse(stateData);

                RoomState state;
                state.parse(json);

                auto memberDb = lmdb::dbi::open(txn, roomid.toStdString().c_str(), MDB_CREATE);
                std::map<std::string, mtx::events::StateEvent<mtx::events::state::Member>> members;

                auto memberCursor = lmdb::cursor::open(txn, memberDb);

                std::string memberId;
                std::string memberContent;

                while (memberCursor.get(memberId, memberContent, MDB_NEXT)) {
                        auto userid = QString::fromStdString(memberId);

                        try {
                                auto data = nlohmann::json::parse(memberContent);
                                mtx::events::StateEvent<mtx::events::state::Member> member = data;
                                members.emplace(memberId, member);
                        } catch (std::exception &e) {
                                qWarning() << "Fault while parsing member event" << e.what()
                                           << QString::fromStdString(memberContent);
                                continue;
                        }
                }

                qDebug() << members.size() << "members for" << roomid;

                state.memberships = members;
                states.emplace(roomid, std::move(state));
        }

        qDebug() << "Retrieved" << states.size() << "rooms";

        cursor.close();

        txn.commit();

        emit statesLoaded(states);
}

void
Cache::setNextBatchToken(lmdb::txn &txn, const QString &token)
{
        auto value = token.toUtf8();

        lmdb::dbi_put(txn, stateDb_, NEXT_BATCH_KEY, lmdb::val(value.data(), value.size()));
}

bool
Cache::isInitialized() const
{
        auto txn = lmdb::txn::begin(env_, nullptr, MDB_RDONLY);
        lmdb::val token;

        bool res = lmdb::dbi_get(txn, stateDb_, NEXT_BATCH_KEY, token);

        txn.commit();

        return res;
}

QString
Cache::nextBatchToken() const
{
        auto txn = lmdb::txn::begin(env_, nullptr, MDB_RDONLY);
        lmdb::val token;

        lmdb::dbi_get(txn, stateDb_, NEXT_BATCH_KEY, token);

        txn.commit();

        return QString::fromUtf8(token.data(), token.size());
}

void
Cache::deleteData()
{
        qInfo() << "Deleting cache data";

        if (!cacheDirectory_.isEmpty())
                QDir(cacheDirectory_).removeRecursively();
}

bool
Cache::isFormatValid()
{
        auto txn = lmdb::txn::begin(env_, nullptr, MDB_RDONLY);

        lmdb::val current_version;
        bool res = lmdb::dbi_get(txn, stateDb_, CACHE_FORMAT_VERSION_KEY, current_version);

        txn.commit();

        if (!res)
                return false;

        std::string stored_version(current_version.data(), current_version.size());

        if (stored_version != CURRENT_CACHE_FORMAT_VERSION) {
                qWarning() << "Stored format version" << QString::fromStdString(stored_version);
                qWarning() << "There are breaking changes in the cache format.";
                return false;
        }

        return true;
}

void
Cache::setCurrentFormat()
{
        auto txn = lmdb::txn::begin(env_);

        lmdb::dbi_put(
          txn,
          stateDb_,
          CACHE_FORMAT_VERSION_KEY,
          lmdb::val(CURRENT_CACHE_FORMAT_VERSION.data(), CURRENT_CACHE_FORMAT_VERSION.size()));

        txn.commit();
}

std::map<std::string, mtx::responses::InvitedRoom>
Cache::invites()
{
        std::map<std::string, mtx::responses::InvitedRoom> invites;

        auto txn    = lmdb::txn::begin(env_, nullptr, MDB_RDONLY);
        auto cursor = lmdb::cursor::open(txn, invitesDb_);

        std::string room_id;
        std::string payload;

        mtx::responses::InvitedRoom state;

        while (cursor.get(room_id, payload, MDB_NEXT)) {
                state = nlohmann::json::parse(payload);
                invites.emplace(room_id, state);
        }

        if (invites.size() > 0)
                qDebug() << "Retrieved" << invites.size() << "invites";

        cursor.close();

        txn.commit();

        return invites;
}

void
Cache::setInvites(const std::map<std::string, mtx::responses::InvitedRoom> &invites)
{
        if (!isMounted_)
                return;

        try {
                auto txn = lmdb::txn::begin(env_);

                for (auto it = invites.cbegin(); it != invites.cend(); ++it) {
                        nlohmann::json j;

                        for (const auto &e : it->second.invite_state) {
                                mpark::visit(
                                  [&j](auto msg) { j["invite_state"]["events"].push_back(msg); },
                                  e);
                        }

                        lmdb::dbi_put(txn, invitesDb_, lmdb::val(it->first), lmdb::val(j.dump()));
                }

                txn.commit();
        } catch (const lmdb::error &e) {
                qCritical() << "setInvitedRooms: " << e.what();
                unmount();
        }
}

CachedReceipts
Cache::readReceipts(const QString &event_id, const QString &room_id)
{
        CachedReceipts receipts;

        ReadReceiptKey receipt_key{event_id.toStdString(), room_id.toStdString()};
        nlohmann::json json_key = receipt_key;

        try {
                auto txn = lmdb::txn::begin(env_, nullptr, MDB_RDONLY);
                auto key = json_key.dump();

                lmdb::val value;

                bool res =
                  lmdb::dbi_get(txn, readReceiptsDb_, lmdb::val(key.data(), key.size()), value);

                txn.commit();

                if (res) {
                        auto json_response = json::parse(std::string(value.data(), value.size()));
                        auto values        = json_response.get<std::map<std::string, uint64_t>>();

                        for (const auto &v : values)
                                // timestamp, user_id
                                receipts.emplace(v.second, v.first);
                }

        } catch (const lmdb::error &e) {
                qCritical() << "readReceipts:" << e.what();
        }

        return receipts;
}

void
Cache::updateReadReceipt(const std::string &room_id, const Receipts &receipts)
{
        for (const auto &receipt : receipts) {
                const auto event_id = receipt.first;
                auto event_receipts = receipt.second;

                ReadReceiptKey receipt_key{event_id, room_id};
                nlohmann::json json_key = receipt_key;

                try {
                        auto read_txn  = lmdb::txn::begin(env_, nullptr, MDB_RDONLY);
                        const auto key = json_key.dump();

                        lmdb::val prev_value;

                        bool exists = lmdb::dbi_get(
                          read_txn, readReceiptsDb_, lmdb::val(key.data(), key.size()), prev_value);

                        read_txn.commit();

                        std::map<std::string, uint64_t> saved_receipts;

                        // If an entry for the event id already exists, we would
                        // merge the existing receipts with the new ones.
                        if (exists) {
                                auto json_value =
                                  json::parse(std::string(prev_value.data(), prev_value.size()));

                                // Retrieve the saved receipts.
                                saved_receipts = json_value.get<std::map<std::string, uint64_t>>();
                        }

                        // Append the new ones.
                        for (const auto &event_receipt : event_receipts)
                                saved_receipts.emplace(event_receipt.first, event_receipt.second);

                        // Save back the merged (or only the new) receipts.
                        nlohmann::json json_updated_value = saved_receipts;
                        std::string merged_receipts       = json_updated_value.dump();

                        auto txn = lmdb::txn::begin(env_);

                        lmdb::dbi_put(txn,
                                      readReceiptsDb_,
                                      lmdb::val(key.data(), key.size()),
                                      lmdb::val(merged_receipts.data(), merged_receipts.size()));

                        txn.commit();
                } catch (const lmdb::error &e) {
                        qCritical() << "updateReadReceipts:" << e.what();
                }
        }
}
