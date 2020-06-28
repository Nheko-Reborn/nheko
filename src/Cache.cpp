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

#include <limits>
#include <stdexcept>
#include <variant>

#include <QByteArray>
#include <QCoreApplication>
#include <QFile>
#include <QHash>
#include <QMap>
#include <QSettings>
#include <QStandardPaths>

#include <mtx/responses/common.hpp>

#include "Cache.h"
#include "Cache_p.h"
#include "EventAccessors.h"
#include "Logging.h"
#include "Utils.h"

//! Should be changed when a breaking change occurs in the cache format.
//! This will reset client's data.
static const std::string CURRENT_CACHE_FORMAT_VERSION("2020.05.01");
static const std::string SECRET("secret");

static lmdb::val NEXT_BATCH_KEY("next_batch");
static lmdb::val OLM_ACCOUNT_KEY("olm_account");
static lmdb::val CACHE_FORMAT_VERSION_KEY("cache_format_version");

constexpr size_t MAX_RESTORED_MESSAGES = 30'000;

constexpr auto DB_SIZE = 32ULL * 1024ULL * 1024ULL * 1024ULL; // 32 GB
constexpr auto MAX_DBS = 8092UL;

//! Cache databases and their format.
//!
//! Contains UI information for the joined rooms. (i.e name, topic, avatar url etc).
//! Format: room_id -> RoomInfo
constexpr auto ROOMS_DB("rooms");
constexpr auto INVITES_DB("invites");
//! Keeps already downloaded media for reuse.
//! Format: matrix_url -> binary data.
constexpr auto MEDIA_DB("media");
//! Information that  must be kept between sync requests.
constexpr auto SYNC_STATE_DB("sync_state");
//! Read receipts per room/event.
constexpr auto READ_RECEIPTS_DB("read_receipts");
constexpr auto NOTIFICATIONS_DB("sent_notifications");
//! TODO: delete pending_receipts database on old cache versions

//! Encryption related databases.

//! user_id -> list of devices
constexpr auto DEVICES_DB("devices");
//! device_id -> device keys
constexpr auto DEVICE_KEYS_DB("device_keys");
//! room_ids that have encryption enabled.
constexpr auto ENCRYPTED_ROOMS_DB("encrypted_rooms");

//! room_id -> pickled OlmInboundGroupSession
constexpr auto INBOUND_MEGOLM_SESSIONS_DB("inbound_megolm_sessions");
//! MegolmSessionIndex -> pickled OlmOutboundGroupSession
constexpr auto OUTBOUND_MEGOLM_SESSIONS_DB("outbound_megolm_sessions");

using CachedReceipts = std::multimap<uint64_t, std::string, std::greater<uint64_t>>;
using Receipts       = std::map<std::string, std::map<std::string, uint64_t>>;

Q_DECLARE_METATYPE(SearchResult)
Q_DECLARE_METATYPE(std::vector<SearchResult>)
Q_DECLARE_METATYPE(RoomMember)
Q_DECLARE_METATYPE(mtx::responses::Timeline)
Q_DECLARE_METATYPE(RoomSearchResult)
Q_DECLARE_METATYPE(RoomInfo)

namespace {
std::unique_ptr<Cache> instance_ = nullptr;
}

int
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

Cache::Cache(const QString &userId, QObject *parent)
  : QObject{parent}
  , env_{nullptr}
  , syncStateDb_{0}
  , roomsDb_{0}
  , invitesDb_{0}
  , mediaDb_{0}
  , readReceiptsDb_{0}
  , notificationsDb_{0}
  , devicesDb_{0}
  , deviceKeysDb_{0}
  , inboundMegolmSessionDb_{0}
  , outboundMegolmSessionDb_{0}
  , localUserId_{userId}
{
        setup();
}

void
Cache::setup()
{
        nhlog::db()->debug("setting up cache");

        auto statePath = QString("%1/%2")
                           .arg(QStandardPaths::writableLocation(QStandardPaths::CacheLocation))
                           .arg(QString::fromUtf8(localUserId_.toUtf8().toHex()));

        cacheDirectory_ = QString("%1/%2")
                            .arg(QStandardPaths::writableLocation(QStandardPaths::CacheLocation))
                            .arg(QString::fromUtf8(localUserId_.toUtf8().toHex()));

        bool isInitial = !QFile::exists(statePath);

        env_ = lmdb::env::create();
        env_.set_mapsize(DB_SIZE);
        env_.set_max_dbs(MAX_DBS);

        if (isInitial) {
                nhlog::db()->info("initializing LMDB");

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

                nhlog::db()->warn("resetting cache due to LMDB version mismatch: {}", e.what());

                QDir stateDir(statePath);

                for (const auto &file : stateDir.entryList(QDir::NoDotAndDotDot)) {
                        if (!stateDir.remove(file))
                                throw std::runtime_error(
                                  ("Unable to delete file " + file).toStdString().c_str());
                }

                env_.open(statePath.toStdString().c_str());
        }

        auto txn         = lmdb::txn::begin(env_);
        syncStateDb_     = lmdb::dbi::open(txn, SYNC_STATE_DB, MDB_CREATE);
        roomsDb_         = lmdb::dbi::open(txn, ROOMS_DB, MDB_CREATE);
        invitesDb_       = lmdb::dbi::open(txn, INVITES_DB, MDB_CREATE);
        mediaDb_         = lmdb::dbi::open(txn, MEDIA_DB, MDB_CREATE);
        readReceiptsDb_  = lmdb::dbi::open(txn, READ_RECEIPTS_DB, MDB_CREATE);
        notificationsDb_ = lmdb::dbi::open(txn, NOTIFICATIONS_DB, MDB_CREATE);

        // Device management
        devicesDb_    = lmdb::dbi::open(txn, DEVICES_DB, MDB_CREATE);
        deviceKeysDb_ = lmdb::dbi::open(txn, DEVICE_KEYS_DB, MDB_CREATE);

        // Session management
        inboundMegolmSessionDb_  = lmdb::dbi::open(txn, INBOUND_MEGOLM_SESSIONS_DB, MDB_CREATE);
        outboundMegolmSessionDb_ = lmdb::dbi::open(txn, OUTBOUND_MEGOLM_SESSIONS_DB, MDB_CREATE);

        txn.commit();
}

void
Cache::setEncryptedRoom(lmdb::txn &txn, const std::string &room_id)
{
        nhlog::db()->info("mark room {} as encrypted", room_id);

        auto db = lmdb::dbi::open(txn, ENCRYPTED_ROOMS_DB, MDB_CREATE);
        lmdb::dbi_put(txn, db, lmdb::val(room_id), lmdb::val("0"));
}

bool
Cache::isRoomEncrypted(const std::string &room_id)
{
        lmdb::val unused;

        auto txn = lmdb::txn::begin(env_);
        auto db  = lmdb::dbi::open(txn, ENCRYPTED_ROOMS_DB, MDB_CREATE);
        auto res = lmdb::dbi_get(txn, db, lmdb::val(room_id), unused);
        txn.commit();

        return res;
}

mtx::crypto::ExportedSessionKeys
Cache::exportSessionKeys()
{
        using namespace mtx::crypto;

        ExportedSessionKeys keys;

        auto txn    = lmdb::txn::begin(env_, nullptr, MDB_RDONLY);
        auto cursor = lmdb::cursor::open(txn, inboundMegolmSessionDb_);

        std::string key, value;
        while (cursor.get(key, value, MDB_NEXT)) {
                ExportedSession exported;
                MegolmSessionIndex index;

                auto saved_session = unpickle<InboundSessionObject>(value, SECRET);

                try {
                        index = nlohmann::json::parse(key).get<MegolmSessionIndex>();
                } catch (const nlohmann::json::exception &e) {
                        nhlog::db()->critical("failed to export megolm session: {}", e.what());
                        continue;
                }

                exported.room_id     = index.room_id;
                exported.sender_key  = index.sender_key;
                exported.session_id  = index.session_id;
                exported.session_key = export_session(saved_session.get());

                keys.sessions.push_back(exported);
        }

        cursor.close();
        txn.commit();

        return keys;
}

void
Cache::importSessionKeys(const mtx::crypto::ExportedSessionKeys &keys)
{
        for (const auto &s : keys.sessions) {
                MegolmSessionIndex index;
                index.room_id    = s.room_id;
                index.session_id = s.session_id;
                index.sender_key = s.sender_key;

                auto exported_session = mtx::crypto::import_session(s.session_key);

                saveInboundMegolmSession(index, std::move(exported_session));
        }
}

//
// Session Management
//

void
Cache::saveInboundMegolmSession(const MegolmSessionIndex &index,
                                mtx::crypto::InboundGroupSessionPtr session)
{
        using namespace mtx::crypto;
        const auto key     = json(index).dump();
        const auto pickled = pickle<InboundSessionObject>(session.get(), SECRET);

        auto txn = lmdb::txn::begin(env_);
        lmdb::dbi_put(txn, inboundMegolmSessionDb_, lmdb::val(key), lmdb::val(pickled));
        txn.commit();

        {
                std::unique_lock<std::mutex> lock(session_storage.group_inbound_mtx);
                session_storage.group_inbound_sessions[key] = std::move(session);
        }
}

OlmInboundGroupSession *
Cache::getInboundMegolmSession(const MegolmSessionIndex &index)
{
        std::unique_lock<std::mutex> lock(session_storage.group_inbound_mtx);
        return session_storage.group_inbound_sessions[json(index).dump()].get();
}

bool
Cache::inboundMegolmSessionExists(const MegolmSessionIndex &index)
{
        std::unique_lock<std::mutex> lock(session_storage.group_inbound_mtx);
        return session_storage.group_inbound_sessions.find(json(index).dump()) !=
               session_storage.group_inbound_sessions.end();
}

void
Cache::updateOutboundMegolmSession(const std::string &room_id, int message_index)
{
        using namespace mtx::crypto;

        if (!outboundMegolmSessionExists(room_id))
                return;

        OutboundGroupSessionData data;
        OlmOutboundGroupSession *session;
        {
                std::unique_lock<std::mutex> lock(session_storage.group_outbound_mtx);
                data    = session_storage.group_outbound_session_data[room_id];
                session = session_storage.group_outbound_sessions[room_id].get();

                // Update with the current message.
                data.message_index                                   = message_index;
                session_storage.group_outbound_session_data[room_id] = data;
        }

        // Save the updated pickled data for the session.
        json j;
        j["data"]    = data;
        j["session"] = pickle<OutboundSessionObject>(session, SECRET);

        auto txn = lmdb::txn::begin(env_);
        lmdb::dbi_put(txn, outboundMegolmSessionDb_, lmdb::val(room_id), lmdb::val(j.dump()));
        txn.commit();
}

void
Cache::saveOutboundMegolmSession(const std::string &room_id,
                                 const OutboundGroupSessionData &data,
                                 mtx::crypto::OutboundGroupSessionPtr session)
{
        using namespace mtx::crypto;
        const auto pickled = pickle<OutboundSessionObject>(session.get(), SECRET);

        json j;
        j["data"]    = data;
        j["session"] = pickled;

        auto txn = lmdb::txn::begin(env_);
        lmdb::dbi_put(txn, outboundMegolmSessionDb_, lmdb::val(room_id), lmdb::val(j.dump()));
        txn.commit();

        {
                std::unique_lock<std::mutex> lock(session_storage.group_outbound_mtx);
                session_storage.group_outbound_session_data[room_id] = data;
                session_storage.group_outbound_sessions[room_id]     = std::move(session);
        }
}

bool
Cache::outboundMegolmSessionExists(const std::string &room_id) noexcept
{
        std::unique_lock<std::mutex> lock(session_storage.group_outbound_mtx);
        return (session_storage.group_outbound_sessions.find(room_id) !=
                session_storage.group_outbound_sessions.end()) &&
               (session_storage.group_outbound_session_data.find(room_id) !=
                session_storage.group_outbound_session_data.end());
}

OutboundGroupSessionDataRef
Cache::getOutboundMegolmSession(const std::string &room_id)
{
        std::unique_lock<std::mutex> lock(session_storage.group_outbound_mtx);
        return OutboundGroupSessionDataRef{session_storage.group_outbound_sessions[room_id].get(),
                                           session_storage.group_outbound_session_data[room_id]};
}

//
// OLM sessions.
//

void
Cache::saveOlmSession(const std::string &curve25519, mtx::crypto::OlmSessionPtr session)
{
        using namespace mtx::crypto;

        auto txn = lmdb::txn::begin(env_);
        auto db  = getOlmSessionsDb(txn, curve25519);

        const auto pickled    = pickle<SessionObject>(session.get(), SECRET);
        const auto session_id = mtx::crypto::session_id(session.get());

        lmdb::dbi_put(txn, db, lmdb::val(session_id), lmdb::val(pickled));

        txn.commit();
}

std::optional<mtx::crypto::OlmSessionPtr>
Cache::getOlmSession(const std::string &curve25519, const std::string &session_id)
{
        using namespace mtx::crypto;

        auto txn = lmdb::txn::begin(env_);
        auto db  = getOlmSessionsDb(txn, curve25519);

        lmdb::val pickled;
        bool found = lmdb::dbi_get(txn, db, lmdb::val(session_id), pickled);

        txn.commit();

        if (found) {
                auto data = std::string(pickled.data(), pickled.size());
                return unpickle<SessionObject>(data, SECRET);
        }

        return std::nullopt;
}

std::vector<std::string>
Cache::getOlmSessions(const std::string &curve25519)
{
        using namespace mtx::crypto;

        auto txn = lmdb::txn::begin(env_);
        auto db  = getOlmSessionsDb(txn, curve25519);

        std::string session_id, unused;
        std::vector<std::string> res;

        auto cursor = lmdb::cursor::open(txn, db);
        while (cursor.get(session_id, unused, MDB_NEXT))
                res.emplace_back(session_id);
        cursor.close();

        txn.commit();

        return res;
}

void
Cache::saveOlmAccount(const std::string &data)
{
        auto txn = lmdb::txn::begin(env_);
        lmdb::dbi_put(txn, syncStateDb_, OLM_ACCOUNT_KEY, lmdb::val(data));
        txn.commit();
}

void
Cache::restoreSessions()
{
        using namespace mtx::crypto;

        auto txn = lmdb::txn::begin(env_, nullptr, MDB_RDONLY);
        std::string key, value;

        //
        // Inbound Megolm Sessions
        //
        {
                auto cursor = lmdb::cursor::open(txn, inboundMegolmSessionDb_);
                while (cursor.get(key, value, MDB_NEXT)) {
                        auto session = unpickle<InboundSessionObject>(value, SECRET);
                        session_storage.group_inbound_sessions[key] = std::move(session);
                }
                cursor.close();
        }

        //
        // Outbound Megolm Sessions
        //
        {
                auto cursor = lmdb::cursor::open(txn, outboundMegolmSessionDb_);
                while (cursor.get(key, value, MDB_NEXT)) {
                        json obj;

                        try {
                                obj = json::parse(value);

                                session_storage.group_outbound_session_data[key] =
                                  obj.at("data").get<OutboundGroupSessionData>();

                                auto session =
                                  unpickle<OutboundSessionObject>(obj.at("session"), SECRET);
                                session_storage.group_outbound_sessions[key] = std::move(session);
                        } catch (const nlohmann::json::exception &e) {
                                nhlog::db()->critical(
                                  "failed to parse outbound megolm session data: {}", e.what());
                        }
                }
                cursor.close();
        }

        txn.commit();

        nhlog::db()->info("sessions restored");
}

std::string
Cache::restoreOlmAccount()
{
        auto txn = lmdb::txn::begin(env_, nullptr, MDB_RDONLY);
        lmdb::val pickled;
        lmdb::dbi_get(txn, syncStateDb_, OLM_ACCOUNT_KEY, pickled);
        txn.commit();

        return std::string(pickled.data(), pickled.size());
}

//
// Media Management
//

void
Cache::saveImage(const std::string &url, const std::string &img_data)
{
        if (url.empty() || img_data.empty())
                return;

        try {
                auto txn = lmdb::txn::begin(env_);

                lmdb::dbi_put(txn,
                              mediaDb_,
                              lmdb::val(url.data(), url.size()),
                              lmdb::val(img_data.data(), img_data.size()));

                txn.commit();
        } catch (const lmdb::error &e) {
                nhlog::db()->critical("saveImage: {}", e.what());
        }
}

void
Cache::saveImage(const QString &url, const QByteArray &image)
{
        saveImage(url.toStdString(), std::string(image.constData(), image.length()));
}

QByteArray
Cache::image(lmdb::txn &txn, const std::string &url) const
{
        if (url.empty())
                return QByteArray();

        try {
                lmdb::val image;
                bool res = lmdb::dbi_get(txn, mediaDb_, lmdb::val(url), image);

                if (!res)
                        return QByteArray();

                return QByteArray(image.data(), image.size());
        } catch (const lmdb::error &e) {
                nhlog::db()->critical("image: {}, {}", e.what(), url);
        }

        return QByteArray();
}

QByteArray
Cache::image(const QString &url) const
{
        if (url.isEmpty())
                return QByteArray();

        auto key = url.toUtf8();

        try {
                auto txn = lmdb::txn::begin(env_, nullptr, MDB_RDONLY);

                lmdb::val image;

                bool res = lmdb::dbi_get(txn, mediaDb_, lmdb::val(key.data(), key.size()), image);

                txn.commit();

                if (!res)
                        return QByteArray();

                return QByteArray(image.data(), image.size());
        } catch (const lmdb::error &e) {
                nhlog::db()->critical("image: {} {}", e.what(), url.toStdString());
        }

        return QByteArray();
}

void
Cache::removeInvite(lmdb::txn &txn, const std::string &room_id)
{
        lmdb::dbi_del(txn, invitesDb_, lmdb::val(room_id), nullptr);
        lmdb::dbi_drop(txn, getInviteStatesDb(txn, room_id), true);
        lmdb::dbi_drop(txn, getInviteMembersDb(txn, room_id), true);
}

void
Cache::removeInvite(const std::string &room_id)
{
        auto txn = lmdb::txn::begin(env_);
        removeInvite(txn, room_id);
        txn.commit();
}

void
Cache::removeRoom(lmdb::txn &txn, const std::string &roomid)
{
        lmdb::dbi_del(txn, roomsDb_, lmdb::val(roomid), nullptr);
        lmdb::dbi_drop(txn, getStatesDb(txn, roomid), true);
        lmdb::dbi_drop(txn, getMembersDb(txn, roomid), true);
}

void
Cache::removeRoom(const std::string &roomid)
{
        auto txn = lmdb::txn::begin(env_, nullptr, 0);
        lmdb::dbi_del(txn, roomsDb_, lmdb::val(roomid), nullptr);
        txn.commit();
}

void
Cache::setNextBatchToken(lmdb::txn &txn, const std::string &token)
{
        lmdb::dbi_put(txn, syncStateDb_, NEXT_BATCH_KEY, lmdb::val(token.data(), token.size()));
}

void
Cache::setNextBatchToken(lmdb::txn &txn, const QString &token)
{
        setNextBatchToken(txn, token.toStdString());
}

bool
Cache::isInitialized() const
{
        auto txn = lmdb::txn::begin(env_, nullptr, MDB_RDONLY);
        lmdb::val token;

        bool res = lmdb::dbi_get(txn, syncStateDb_, NEXT_BATCH_KEY, token);

        txn.commit();

        return res;
}

std::string
Cache::nextBatchToken() const
{
        auto txn = lmdb::txn::begin(env_, nullptr, MDB_RDONLY);
        lmdb::val token;

        lmdb::dbi_get(txn, syncStateDb_, NEXT_BATCH_KEY, token);

        txn.commit();

        return std::string(token.data(), token.size());
}

void
Cache::deleteData()
{
        // TODO: We need to remove the env_ while not accepting new requests.
        if (!cacheDirectory_.isEmpty()) {
                QDir(cacheDirectory_).removeRecursively();
                nhlog::db()->info("deleted cache files from disk");
        }
}

//! migrates db to the current format
bool
Cache::runMigrations()
{
        auto txn = lmdb::txn::begin(env_, nullptr, MDB_RDONLY);

        lmdb::val current_version;
        bool res = lmdb::dbi_get(txn, syncStateDb_, CACHE_FORMAT_VERSION_KEY, current_version);

        txn.commit();

        if (!res)
                return false;

        std::string stored_version(current_version.data(), current_version.size());

        std::vector<std::pair<std::string, std::function<bool()>>> migrations{
          {"2020.05.01",
           [this]() {
                   try {
                           auto txn = lmdb::txn::begin(env_, nullptr);
                           auto pending_receipts =
                             lmdb::dbi::open(txn, "pending_receipts", MDB_CREATE);
                           lmdb::dbi_drop(txn, pending_receipts, true);
                           txn.commit();
                   } catch (const lmdb::error &) {
                           nhlog::db()->critical(
                             "Failed to delete pending_receipts database in migration!");
                           return false;
                   }

                   nhlog::db()->info("Successfully deleted pending receipts database.");
                   return true;
           }},
        };

        for (const auto &[target_version, migration] : migrations) {
                if (target_version > stored_version)
                        if (!migration()) {
                                nhlog::db()->critical("migration failure!");
                                return false;
                        }
        }

        setCurrentFormat();
        return true;
}

cache::CacheVersion
Cache::formatVersion()
{
        auto txn = lmdb::txn::begin(env_, nullptr, MDB_RDONLY);

        lmdb::val current_version;
        bool res = lmdb::dbi_get(txn, syncStateDb_, CACHE_FORMAT_VERSION_KEY, current_version);

        txn.commit();

        if (!res)
                return cache::CacheVersion::Older;

        std::string stored_version(current_version.data(), current_version.size());

        if (stored_version < CURRENT_CACHE_FORMAT_VERSION)
                return cache::CacheVersion::Older;
        else if (stored_version > CURRENT_CACHE_FORMAT_VERSION)
                return cache::CacheVersion::Older;
        else
                return cache::CacheVersion::Current;
}

void
Cache::setCurrentFormat()
{
        auto txn = lmdb::txn::begin(env_);

        lmdb::dbi_put(
          txn,
          syncStateDb_,
          CACHE_FORMAT_VERSION_KEY,
          lmdb::val(CURRENT_CACHE_FORMAT_VERSION.data(), CURRENT_CACHE_FORMAT_VERSION.size()));

        txn.commit();
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
                nhlog::db()->critical("readReceipts: {}", e.what());
        }

        return receipts;
}

void
Cache::updateReadReceipt(lmdb::txn &txn, const std::string &room_id, const Receipts &receipts)
{
        auto user_id = this->localUserId_.toStdString();
        for (const auto &receipt : receipts) {
                const auto event_id = receipt.first;
                auto event_receipts = receipt.second;

                ReadReceiptKey receipt_key{event_id, room_id};
                nlohmann::json json_key = receipt_key;

                try {
                        const auto key = json_key.dump();

                        lmdb::val prev_value;

                        bool exists = lmdb::dbi_get(
                          txn, readReceiptsDb_, lmdb::val(key.data(), key.size()), prev_value);

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
                        for (const auto &[read_by, timestamp] : event_receipts) {
                                if (read_by == user_id) {
                                        emit removeNotification(QString::fromStdString(room_id),
                                                                QString::fromStdString(event_id));
                                }
                                saved_receipts.emplace(read_by, timestamp);
                        }

                        // Save back the merged (or only the new) receipts.
                        nlohmann::json json_updated_value = saved_receipts;
                        std::string merged_receipts       = json_updated_value.dump();

                        lmdb::dbi_put(txn,
                                      readReceiptsDb_,
                                      lmdb::val(key.data(), key.size()),
                                      lmdb::val(merged_receipts.data(), merged_receipts.size()));

                } catch (const lmdb::error &e) {
                        nhlog::db()->critical("updateReadReceipts: {}", e.what());
                }
        }
}

void
Cache::calculateRoomReadStatus()
{
        const auto joined_rooms = joinedRooms();

        std::map<QString, bool> readStatus;

        for (const auto &room : joined_rooms)
                readStatus.emplace(QString::fromStdString(room), calculateRoomReadStatus(room));

        emit roomReadStatus(readStatus);
}

bool
Cache::calculateRoomReadStatus(const std::string &room_id)
{
        auto txn = lmdb::txn::begin(env_);

        // Get last event id on the room.
        const auto last_event_id = getLastEventId(txn, room_id);
        const auto localUser     = utils::localUser().toStdString();

        txn.commit();

        if (last_event_id.empty())
                return false;

        // Retrieve all read receipts for that event.
        const auto receipts =
          readReceipts(QString::fromStdString(last_event_id), QString::fromStdString(room_id));

        if (receipts.size() == 0)
                return true;

        // Check if the local user has a read receipt for it.
        for (auto it = receipts.cbegin(); it != receipts.cend(); it++) {
                if (it->second == localUser)
                        return false;
        }

        return true;
}

void
Cache::saveState(const mtx::responses::Sync &res)
{
        using namespace mtx::events;
        auto user_id = this->localUserId_.toStdString();

        auto txn = lmdb::txn::begin(env_);

        setNextBatchToken(txn, res.next_batch);

        // Save joined rooms
        for (const auto &room : res.rooms.join) {
                auto statesdb  = getStatesDb(txn, room.first);
                auto membersdb = getMembersDb(txn, room.first);

                saveStateEvents(txn, statesdb, membersdb, room.first, room.second.state.events);
                saveStateEvents(txn, statesdb, membersdb, room.first, room.second.timeline.events);

                saveTimelineMessages(txn, room.first, room.second.timeline);

                RoomInfo updatedInfo;
                updatedInfo.name  = getRoomName(txn, statesdb, membersdb).toStdString();
                updatedInfo.topic = getRoomTopic(txn, statesdb).toStdString();
                updatedInfo.avatar_url =
                  getRoomAvatarUrl(txn, statesdb, membersdb, QString::fromStdString(room.first))
                    .toStdString();
                updatedInfo.version = getRoomVersion(txn, statesdb).toStdString();

                // Process the account_data associated with this room
                bool has_new_tags = false;
                for (const auto &evt : room.second.account_data.events) {
                        // for now only fetch tag events
                        if (std::holds_alternative<Event<account_data::Tags>>(evt)) {
                                auto tags_evt = std::get<Event<account_data::Tags>>(evt);
                                has_new_tags  = true;
                                for (const auto &tag : tags_evt.content.tags) {
                                        updatedInfo.tags.push_back(tag.first);
                                }
                        }
                }
                if (!has_new_tags) {
                        // retrieve the old tags, they haven't changed
                        lmdb::val data;
                        if (lmdb::dbi_get(txn, roomsDb_, lmdb::val(room.first), data)) {
                                try {
                                        RoomInfo tmp =
                                          json::parse(std::string(data.data(), data.size()));
                                        updatedInfo.tags = tmp.tags;
                                } catch (const json::exception &e) {
                                        nhlog::db()->warn(
                                          "failed to parse room info: room_id ({}), {}",
                                          room.first,
                                          std::string(data.data(), data.size()));
                                }
                        }
                }

                lmdb::dbi_put(
                  txn, roomsDb_, lmdb::val(room.first), lmdb::val(json(updatedInfo).dump()));

                updateReadReceipt(txn, room.first, room.second.ephemeral.receipts);

                // Clean up non-valid invites.
                removeInvite(txn, room.first);
        }

        saveInvites(txn, res.rooms.invite);

        savePresence(txn, res.presence);

        removeLeftRooms(txn, res.rooms.leave);

        txn.commit();

        std::map<QString, bool> readStatus;

        for (const auto &room : res.rooms.join) {
                if (!room.second.ephemeral.receipts.empty()) {
                        std::vector<QString> receipts;
                        for (const auto &receipt : room.second.ephemeral.receipts) {
                                for (const auto &receiptUsersTs : receipt.second) {
                                        if (receiptUsersTs.first != user_id) {
                                                receipts.push_back(
                                                  QString::fromStdString(receipt.first));
                                                break;
                                        }
                                }
                        }
                        if (!receipts.empty())
                                emit newReadReceipts(QString::fromStdString(room.first), receipts);
                }
                readStatus.emplace(QString::fromStdString(room.first),
                                   calculateRoomReadStatus(room.first));
        }

        emit roomReadStatus(readStatus);
}

void
Cache::saveInvites(lmdb::txn &txn, const std::map<std::string, mtx::responses::InvitedRoom> &rooms)
{
        for (const auto &room : rooms) {
                auto statesdb  = getInviteStatesDb(txn, room.first);
                auto membersdb = getInviteMembersDb(txn, room.first);

                saveInvite(txn, statesdb, membersdb, room.second);

                RoomInfo updatedInfo;
                updatedInfo.name  = getInviteRoomName(txn, statesdb, membersdb).toStdString();
                updatedInfo.topic = getInviteRoomTopic(txn, statesdb).toStdString();
                updatedInfo.avatar_url =
                  getInviteRoomAvatarUrl(txn, statesdb, membersdb).toStdString();
                updatedInfo.is_invite = true;

                lmdb::dbi_put(
                  txn, invitesDb_, lmdb::val(room.first), lmdb::val(json(updatedInfo).dump()));
        }
}

void
Cache::saveInvite(lmdb::txn &txn,
                  lmdb::dbi &statesdb,
                  lmdb::dbi &membersdb,
                  const mtx::responses::InvitedRoom &room)
{
        using namespace mtx::events;
        using namespace mtx::events::state;

        for (const auto &e : room.invite_state) {
                if (auto msg = std::get_if<StrippedEvent<Member>>(&e)) {
                        auto display_name = msg->content.display_name.empty()
                                              ? msg->state_key
                                              : msg->content.display_name;

                        MemberInfo tmp{display_name, msg->content.avatar_url};

                        lmdb::dbi_put(
                          txn, membersdb, lmdb::val(msg->state_key), lmdb::val(json(tmp).dump()));
                } else {
                        std::visit(
                          [&txn, &statesdb](auto msg) {
                                  bool res = lmdb::dbi_put(txn,
                                                           statesdb,
                                                           lmdb::val(to_string(msg.type)),
                                                           lmdb::val(json(msg).dump()));

                                  if (!res)
                                          nhlog::db()->warn("couldn't save data: {}",
                                                            json(msg).dump());
                          },
                          e);
                }
        }
}

void
Cache::savePresence(
  lmdb::txn &txn,
  const std::vector<mtx::events::Event<mtx::events::presence::Presence>> &presenceUpdates)
{
        for (const auto &update : presenceUpdates) {
                auto presenceDb = getPresenceDb(txn);

                lmdb::dbi_put(txn,
                              presenceDb,
                              lmdb::val(update.sender),
                              lmdb::val(json(update.content).dump()));
        }
}

std::vector<std::string>
Cache::roomsWithStateUpdates(const mtx::responses::Sync &res)
{
        std::vector<std::string> rooms;
        for (const auto &room : res.rooms.join) {
                bool hasUpdates = false;
                for (const auto &s : room.second.state.events) {
                        if (containsStateUpdates(s)) {
                                hasUpdates = true;
                                break;
                        }
                }

                for (const auto &s : room.second.timeline.events) {
                        if (containsStateUpdates(s)) {
                                hasUpdates = true;
                                break;
                        }
                }

                if (hasUpdates)
                        rooms.emplace_back(room.first);
        }

        for (const auto &room : res.rooms.invite) {
                for (const auto &s : room.second.invite_state) {
                        if (containsStateUpdates(s)) {
                                rooms.emplace_back(room.first);
                                break;
                        }
                }
        }

        return rooms;
}

std::vector<std::string>
Cache::roomsWithTagUpdates(const mtx::responses::Sync &res)
{
        using namespace mtx::events;

        std::vector<std::string> rooms;
        for (const auto &room : res.rooms.join) {
                bool hasUpdates = false;
                for (const auto &evt : room.second.account_data.events) {
                        if (std::holds_alternative<Event<account_data::Tags>>(evt)) {
                                hasUpdates = true;
                        }
                }

                if (hasUpdates)
                        rooms.emplace_back(room.first);
        }

        return rooms;
}

RoomInfo
Cache::singleRoomInfo(const std::string &room_id)
{
        auto txn      = lmdb::txn::begin(env_, nullptr, MDB_RDONLY);
        auto statesdb = getStatesDb(txn, room_id);

        lmdb::val data;

        // Check if the room is joined.
        if (lmdb::dbi_get(txn, roomsDb_, lmdb::val(room_id), data)) {
                try {
                        RoomInfo tmp     = json::parse(std::string(data.data(), data.size()));
                        tmp.member_count = getMembersDb(txn, room_id).size(txn);
                        tmp.join_rule    = getRoomJoinRule(txn, statesdb);
                        tmp.guest_access = getRoomGuestAccess(txn, statesdb);

                        txn.commit();

                        return tmp;
                } catch (const json::exception &e) {
                        nhlog::db()->warn("failed to parse room info: room_id ({}), {}",
                                          room_id,
                                          std::string(data.data(), data.size()));
                }
        }

        txn.commit();

        return RoomInfo();
}

std::map<QString, RoomInfo>
Cache::getRoomInfo(const std::vector<std::string> &rooms)
{
        std::map<QString, RoomInfo> room_info;

        // TODO This should be read only.
        auto txn = lmdb::txn::begin(env_);

        for (const auto &room : rooms) {
                lmdb::val data;
                auto statesdb = getStatesDb(txn, room);

                // Check if the room is joined.
                if (lmdb::dbi_get(txn, roomsDb_, lmdb::val(room), data)) {
                        try {
                                RoomInfo tmp = json::parse(std::string(data.data(), data.size()));
                                tmp.member_count = getMembersDb(txn, room).size(txn);
                                tmp.join_rule    = getRoomJoinRule(txn, statesdb);
                                tmp.guest_access = getRoomGuestAccess(txn, statesdb);

                                room_info.emplace(QString::fromStdString(room), std::move(tmp));
                        } catch (const json::exception &e) {
                                nhlog::db()->warn("failed to parse room info: room_id ({}), {}",
                                                  room,
                                                  std::string(data.data(), data.size()));
                        }
                } else {
                        // Check if the room is an invite.
                        if (lmdb::dbi_get(txn, invitesDb_, lmdb::val(room), data)) {
                                try {
                                        RoomInfo tmp =
                                          json::parse(std::string(data.data(), data.size()));
                                        tmp.member_count = getInviteMembersDb(txn, room).size(txn);

                                        room_info.emplace(QString::fromStdString(room),
                                                          std::move(tmp));
                                } catch (const json::exception &e) {
                                        nhlog::db()->warn(
                                          "failed to parse room info for invite: room_id ({}), {}",
                                          room,
                                          std::string(data.data(), data.size()));
                                }
                        }
                }
        }

        txn.commit();

        return room_info;
}

std::map<QString, mtx::responses::Timeline>
Cache::roomMessages()
{
        auto txn = lmdb::txn::begin(env_, nullptr, MDB_RDONLY);

        std::map<QString, mtx::responses::Timeline> msgs;
        std::string room_id, unused;

        auto roomsCursor = lmdb::cursor::open(txn, roomsDb_);
        while (roomsCursor.get(room_id, unused, MDB_NEXT))
                msgs.emplace(QString::fromStdString(room_id), mtx::responses::Timeline());

        roomsCursor.close();
        txn.commit();

        return msgs;
}

QMap<QString, mtx::responses::Notifications>
Cache::getTimelineMentions()
{
        // TODO: Should be read-only, but getMentionsDb will attempt to create a DB
        // if it doesn't exist, throwing an error.
        auto txn = lmdb::txn::begin(env_, nullptr);

        QMap<QString, mtx::responses::Notifications> notifs;

        auto room_ids = getRoomIds(txn);

        for (const auto &room_id : room_ids) {
                auto roomNotifs                         = getTimelineMentionsForRoom(txn, room_id);
                notifs[QString::fromStdString(room_id)] = roomNotifs;
        }

        txn.commit();

        return notifs;
}

mtx::responses::Timeline
Cache::getTimelineMessages(lmdb::txn &txn, const std::string &room_id)
{
        // TODO(nico): Limit the messages returned by this maybe?
        auto db = getMessagesDb(txn, room_id);

        mtx::responses::Timeline timeline;
        std::string timestamp, msg;

        auto cursor = lmdb::cursor::open(txn, db);

        size_t index = 0;

        while (cursor.get(timestamp, msg, MDB_NEXT) && index < MAX_RESTORED_MESSAGES) {
                auto obj = json::parse(msg);

                if (obj.count("event") == 0 || obj.count("token") == 0)
                        continue;

                mtx::events::collections::TimelineEvent event;
                mtx::events::collections::from_json(obj.at("event"), event);

                index += 1;

                timeline.events.push_back(event.data);
                timeline.prev_batch = obj.at("token").get<std::string>();
        }
        cursor.close();

        std::reverse(timeline.events.begin(), timeline.events.end());

        return timeline;
}

QMap<QString, RoomInfo>
Cache::roomInfo(bool withInvites)
{
        QMap<QString, RoomInfo> result;

        auto txn = lmdb::txn::begin(env_, nullptr, MDB_RDONLY);

        std::string room_id;
        std::string room_data;

        // Gather info about the joined rooms.
        auto roomsCursor = lmdb::cursor::open(txn, roomsDb_);
        while (roomsCursor.get(room_id, room_data, MDB_NEXT)) {
                RoomInfo tmp     = json::parse(std::move(room_data));
                tmp.member_count = getMembersDb(txn, room_id).size(txn);
                tmp.msgInfo      = getLastMessageInfo(txn, room_id);

                result.insert(QString::fromStdString(std::move(room_id)), std::move(tmp));
        }
        roomsCursor.close();

        if (withInvites) {
                // Gather info about the invites.
                auto invitesCursor = lmdb::cursor::open(txn, invitesDb_);
                while (invitesCursor.get(room_id, room_data, MDB_NEXT)) {
                        RoomInfo tmp     = json::parse(room_data);
                        tmp.member_count = getInviteMembersDb(txn, room_id).size(txn);
                        result.insert(QString::fromStdString(std::move(room_id)), std::move(tmp));
                }
                invitesCursor.close();
        }

        txn.commit();

        return result;
}

std::string
Cache::getLastEventId(lmdb::txn &txn, const std::string &room_id)
{
        auto db = getMessagesDb(txn, room_id);

        if (db.size(txn) == 0)
                return {};

        std::string timestamp, msg;

        auto cursor = lmdb::cursor::open(txn, db);
        while (cursor.get(timestamp, msg, MDB_NEXT)) {
                auto obj = json::parse(msg);

                if (obj.count("event") == 0)
                        continue;

                cursor.close();
                return obj["event"]["event_id"];
        }
        cursor.close();

        return {};
}

DescInfo
Cache::getLastMessageInfo(lmdb::txn &txn, const std::string &room_id)
{
        auto db = getMessagesDb(txn, room_id);

        if (db.size(txn) == 0)
                return DescInfo{};

        std::string timestamp, msg;

        const auto local_user = utils::localUser();

        DescInfo fallbackDesc{};

        auto cursor = lmdb::cursor::open(txn, db);
        while (cursor.get(timestamp, msg, MDB_NEXT)) {
                auto obj = json::parse(msg);

                if (obj.count("event") == 0)
                        continue;

                if (fallbackDesc.event_id.isEmpty() && obj["event"]["type"] == "m.room.member" &&
                    obj["event"]["state_key"] == local_user.toStdString() &&
                    obj["event"]["content"]["membership"] == "join") {
                        uint64_t ts  = obj["event"]["origin_server_ts"];
                        auto time    = QDateTime::fromMSecsSinceEpoch(ts);
                        fallbackDesc = DescInfo{QString::fromStdString(obj["event"]["event_id"]),
                                                local_user,
                                                tr("You joined this room."),
                                                utils::descriptiveTime(time),
                                                ts,
                                                time};
                }

                if (!(obj["event"]["type"] == "m.room.message" ||
                      obj["event"]["type"] == "m.sticker" ||
                      obj["event"]["type"] == "m.room.encrypted"))
                        continue;

                mtx::events::collections::TimelineEvent event;
                mtx::events::collections::from_json(obj.at("event"), event);

                cursor.close();
                return utils::getMessageDescription(
                  event.data, local_user, QString::fromStdString(room_id));
        }
        cursor.close();

        return fallbackDesc;
}

std::map<QString, bool>
Cache::invites()
{
        std::map<QString, bool> result;

        auto txn    = lmdb::txn::begin(env_, nullptr, MDB_RDONLY);
        auto cursor = lmdb::cursor::open(txn, invitesDb_);

        std::string room_id, unused;

        while (cursor.get(room_id, unused, MDB_NEXT))
                result.emplace(QString::fromStdString(std::move(room_id)), true);

        cursor.close();
        txn.commit();

        return result;
}

QString
Cache::getRoomAvatarUrl(lmdb::txn &txn,
                        lmdb::dbi &statesdb,
                        lmdb::dbi &membersdb,
                        const QString &room_id)
{
        using namespace mtx::events;
        using namespace mtx::events::state;

        lmdb::val event;
        bool res = lmdb::dbi_get(
          txn, statesdb, lmdb::val(to_string(mtx::events::EventType::RoomAvatar)), event);

        if (res) {
                try {
                        StateEvent<Avatar> msg =
                          json::parse(std::string(event.data(), event.size()));

                        if (!msg.content.url.empty())
                                return QString::fromStdString(msg.content.url);
                } catch (const json::exception &e) {
                        nhlog::db()->warn("failed to parse m.room.avatar event: {}", e.what());
                }
        }

        // We don't use an avatar for group chats.
        if (membersdb.size(txn) > 2)
                return QString();

        auto cursor = lmdb::cursor::open(txn, membersdb);
        std::string user_id;
        std::string member_data;

        // Resolve avatar for 1-1 chats.
        while (cursor.get(user_id, member_data, MDB_NEXT)) {
                if (user_id == localUserId_.toStdString())
                        continue;

                try {
                        MemberInfo m = json::parse(member_data);

                        cursor.close();
                        return QString::fromStdString(m.avatar_url);
                } catch (const json::exception &e) {
                        nhlog::db()->warn("failed to parse member info: {}", e.what());
                }
        }

        cursor.close();

        // Default case when there is only one member.
        return avatarUrl(room_id, localUserId_);
}

QString
Cache::getRoomName(lmdb::txn &txn, lmdb::dbi &statesdb, lmdb::dbi &membersdb)
{
        using namespace mtx::events;
        using namespace mtx::events::state;

        lmdb::val event;
        bool res = lmdb::dbi_get(
          txn, statesdb, lmdb::val(to_string(mtx::events::EventType::RoomName)), event);

        if (res) {
                try {
                        StateEvent<Name> msg = json::parse(std::string(event.data(), event.size()));

                        if (!msg.content.name.empty())
                                return QString::fromStdString(msg.content.name);
                } catch (const json::exception &e) {
                        nhlog::db()->warn("failed to parse m.room.name event: {}", e.what());
                }
        }

        res = lmdb::dbi_get(
          txn, statesdb, lmdb::val(to_string(mtx::events::EventType::RoomCanonicalAlias)), event);

        if (res) {
                try {
                        StateEvent<CanonicalAlias> msg =
                          json::parse(std::string(event.data(), event.size()));

                        if (!msg.content.alias.empty())
                                return QString::fromStdString(msg.content.alias);
                } catch (const json::exception &e) {
                        nhlog::db()->warn("failed to parse m.room.canonical_alias event: {}",
                                          e.what());
                }
        }

        auto cursor     = lmdb::cursor::open(txn, membersdb);
        const int total = membersdb.size(txn);

        std::size_t ii = 0;
        std::string user_id;
        std::string member_data;
        std::map<std::string, MemberInfo> members;

        while (cursor.get(user_id, member_data, MDB_NEXT) && ii < 3) {
                try {
                        members.emplace(user_id, json::parse(member_data));
                } catch (const json::exception &e) {
                        nhlog::db()->warn("failed to parse member info: {}", e.what());
                }

                ii++;
        }

        cursor.close();

        if (total == 1 && !members.empty())
                return QString::fromStdString(members.begin()->second.name);

        auto first_member = [&members, this]() {
                for (const auto &m : members) {
                        if (m.first != localUserId_.toStdString())
                                return QString::fromStdString(m.second.name);
                }

                return localUserId_;
        }();

        if (total == 2)
                return first_member;
        else if (total > 2)
                return QString("%1 and %2 others").arg(first_member).arg(total - 1);

        return "Empty Room";
}

mtx::events::state::JoinRule
Cache::getRoomJoinRule(lmdb::txn &txn, lmdb::dbi &statesdb)
{
        using namespace mtx::events;
        using namespace mtx::events::state;

        lmdb::val event;
        bool res = lmdb::dbi_get(
          txn, statesdb, lmdb::val(to_string(mtx::events::EventType::RoomJoinRules)), event);

        if (res) {
                try {
                        StateEvent<state::JoinRules> msg =
                          json::parse(std::string(event.data(), event.size()));
                        return msg.content.join_rule;
                } catch (const json::exception &e) {
                        nhlog::db()->warn("failed to parse m.room.join_rule event: {}", e.what());
                }
        }
        return state::JoinRule::Knock;
}

bool
Cache::getRoomGuestAccess(lmdb::txn &txn, lmdb::dbi &statesdb)
{
        using namespace mtx::events;
        using namespace mtx::events::state;

        lmdb::val event;
        bool res = lmdb::dbi_get(
          txn, statesdb, lmdb::val(to_string(mtx::events::EventType::RoomGuestAccess)), event);

        if (res) {
                try {
                        StateEvent<GuestAccess> msg =
                          json::parse(std::string(event.data(), event.size()));
                        return msg.content.guest_access == AccessState::CanJoin;
                } catch (const json::exception &e) {
                        nhlog::db()->warn("failed to parse m.room.guest_access event: {}",
                                          e.what());
                }
        }
        return false;
}

QString
Cache::getRoomTopic(lmdb::txn &txn, lmdb::dbi &statesdb)
{
        using namespace mtx::events;
        using namespace mtx::events::state;

        lmdb::val event;
        bool res = lmdb::dbi_get(
          txn, statesdb, lmdb::val(to_string(mtx::events::EventType::RoomTopic)), event);

        if (res) {
                try {
                        StateEvent<Topic> msg =
                          json::parse(std::string(event.data(), event.size()));

                        if (!msg.content.topic.empty())
                                return QString::fromStdString(msg.content.topic);
                } catch (const json::exception &e) {
                        nhlog::db()->warn("failed to parse m.room.topic event: {}", e.what());
                }
        }

        return QString();
}

QString
Cache::getRoomVersion(lmdb::txn &txn, lmdb::dbi &statesdb)
{
        using namespace mtx::events;
        using namespace mtx::events::state;

        lmdb::val event;
        bool res = lmdb::dbi_get(
          txn, statesdb, lmdb::val(to_string(mtx::events::EventType::RoomCreate)), event);

        if (res) {
                try {
                        StateEvent<Create> msg =
                          json::parse(std::string(event.data(), event.size()));

                        if (!msg.content.room_version.empty())
                                return QString::fromStdString(msg.content.room_version);
                } catch (const json::exception &e) {
                        nhlog::db()->warn("failed to parse m.room.create event: {}", e.what());
                }
        }

        nhlog::db()->warn("m.room.create event is missing room version, assuming version \"1\"");
        return QString("1");
}

QString
Cache::getInviteRoomName(lmdb::txn &txn, lmdb::dbi &statesdb, lmdb::dbi &membersdb)
{
        using namespace mtx::events;
        using namespace mtx::events::state;

        lmdb::val event;
        bool res = lmdb::dbi_get(
          txn, statesdb, lmdb::val(to_string(mtx::events::EventType::RoomName)), event);

        if (res) {
                try {
                        StrippedEvent<state::Name> msg =
                          json::parse(std::string(event.data(), event.size()));
                        return QString::fromStdString(msg.content.name);
                } catch (const json::exception &e) {
                        nhlog::db()->warn("failed to parse m.room.name event: {}", e.what());
                }
        }

        auto cursor = lmdb::cursor::open(txn, membersdb);
        std::string user_id, member_data;

        while (cursor.get(user_id, member_data, MDB_NEXT)) {
                if (user_id == localUserId_.toStdString())
                        continue;

                try {
                        MemberInfo tmp = json::parse(member_data);
                        cursor.close();

                        return QString::fromStdString(tmp.name);
                } catch (const json::exception &e) {
                        nhlog::db()->warn("failed to parse member info: {}", e.what());
                }
        }

        cursor.close();

        return QString("Empty Room");
}

QString
Cache::getInviteRoomAvatarUrl(lmdb::txn &txn, lmdb::dbi &statesdb, lmdb::dbi &membersdb)
{
        using namespace mtx::events;
        using namespace mtx::events::state;

        lmdb::val event;
        bool res = lmdb::dbi_get(
          txn, statesdb, lmdb::val(to_string(mtx::events::EventType::RoomAvatar)), event);

        if (res) {
                try {
                        StrippedEvent<state::Avatar> msg =
                          json::parse(std::string(event.data(), event.size()));
                        return QString::fromStdString(msg.content.url);
                } catch (const json::exception &e) {
                        nhlog::db()->warn("failed to parse m.room.avatar event: {}", e.what());
                }
        }

        auto cursor = lmdb::cursor::open(txn, membersdb);
        std::string user_id, member_data;

        while (cursor.get(user_id, member_data, MDB_NEXT)) {
                if (user_id == localUserId_.toStdString())
                        continue;

                try {
                        MemberInfo tmp = json::parse(member_data);
                        cursor.close();

                        return QString::fromStdString(tmp.avatar_url);
                } catch (const json::exception &e) {
                        nhlog::db()->warn("failed to parse member info: {}", e.what());
                }
        }

        cursor.close();

        return QString();
}

QString
Cache::getInviteRoomTopic(lmdb::txn &txn, lmdb::dbi &db)
{
        using namespace mtx::events;
        using namespace mtx::events::state;

        lmdb::val event;
        bool res =
          lmdb::dbi_get(txn, db, lmdb::val(to_string(mtx::events::EventType::RoomTopic)), event);

        if (res) {
                try {
                        StrippedEvent<Topic> msg =
                          json::parse(std::string(event.data(), event.size()));
                        return QString::fromStdString(msg.content.topic);
                } catch (const json::exception &e) {
                        nhlog::db()->warn("failed to parse m.room.topic event: {}", e.what());
                }
        }

        return QString();
}

QImage
Cache::getRoomAvatar(const QString &room_id)
{
        return getRoomAvatar(room_id.toStdString());
}

QImage
Cache::getRoomAvatar(const std::string &room_id)
{
        auto txn = lmdb::txn::begin(env_, nullptr, MDB_RDONLY);

        lmdb::val response;

        if (!lmdb::dbi_get(txn, roomsDb_, lmdb::val(room_id), response)) {
                txn.commit();
                return QImage();
        }

        std::string media_url;

        try {
                RoomInfo info = json::parse(std::string(response.data(), response.size()));
                media_url     = std::move(info.avatar_url);

                if (media_url.empty()) {
                        txn.commit();
                        return QImage();
                }
        } catch (const json::exception &e) {
                nhlog::db()->warn("failed to parse room info: {}, {}",
                                  e.what(),
                                  std::string(response.data(), response.size()));
        }

        if (!lmdb::dbi_get(txn, mediaDb_, lmdb::val(media_url), response)) {
                txn.commit();
                return QImage();
        }

        txn.commit();

        return QImage::fromData(QByteArray(response.data(), response.size()));
}

std::vector<std::string>
Cache::joinedRooms()
{
        auto txn         = lmdb::txn::begin(env_, nullptr, MDB_RDONLY);
        auto roomsCursor = lmdb::cursor::open(txn, roomsDb_);

        std::string id, data;
        std::vector<std::string> room_ids;

        // Gather the room ids for the joined rooms.
        while (roomsCursor.get(id, data, MDB_NEXT))
                room_ids.emplace_back(id);

        roomsCursor.close();
        txn.commit();

        return room_ids;
}

void
Cache::populateMembers()
{
        auto rooms = joinedRooms();
        nhlog::db()->info("loading {} rooms", rooms.size());

        auto txn = lmdb::txn::begin(env_);

        for (const auto &room : rooms) {
                const auto roomid = QString::fromStdString(room);

                auto membersdb = getMembersDb(txn, room);
                auto cursor    = lmdb::cursor::open(txn, membersdb);

                std::string user_id, info;
                while (cursor.get(user_id, info, MDB_NEXT)) {
                        MemberInfo m = json::parse(info);

                        const auto userid = QString::fromStdString(user_id);

                        insertDisplayName(roomid, userid, QString::fromStdString(m.name));
                        insertAvatarUrl(roomid, userid, QString::fromStdString(m.avatar_url));
                }

                cursor.close();
        }

        txn.commit();
}

std::vector<RoomSearchResult>
Cache::searchRooms(const std::string &query, std::uint8_t max_items)
{
        std::multimap<int, std::pair<std::string, RoomInfo>> items;

        auto txn    = lmdb::txn::begin(env_, nullptr, MDB_RDONLY);
        auto cursor = lmdb::cursor::open(txn, roomsDb_);

        std::string room_id, room_data;
        while (cursor.get(room_id, room_data, MDB_NEXT)) {
                RoomInfo tmp = json::parse(std::move(room_data));

                const int score = utils::levenshtein_distance(
                  query, QString::fromStdString(tmp.name).toLower().toStdString());
                items.emplace(score, std::make_pair(room_id, tmp));
        }

        cursor.close();

        auto end = items.begin();

        if (items.size() >= max_items)
                std::advance(end, max_items);
        else if (items.size() > 0)
                std::advance(end, items.size());

        std::vector<RoomSearchResult> results;
        for (auto it = items.begin(); it != end; it++) {
                results.push_back(RoomSearchResult{it->second.first, it->second.second});
        }

        txn.commit();

        return results;
}

std::vector<SearchResult>
Cache::searchUsers(const std::string &room_id, const std::string &query, std::uint8_t max_items)
{
        std::multimap<int, std::pair<std::string, std::string>> items;

        auto txn    = lmdb::txn::begin(env_, nullptr, MDB_RDONLY);
        auto cursor = lmdb::cursor::open(txn, getMembersDb(txn, room_id));

        std::string user_id, user_data;
        while (cursor.get(user_id, user_data, MDB_NEXT)) {
                const auto display_name = displayName(room_id, user_id);
                const int score         = utils::levenshtein_distance(query, display_name);

                items.emplace(score, std::make_pair(user_id, display_name));
        }

        auto end = items.begin();

        if (items.size() >= max_items)
                std::advance(end, max_items);
        else if (items.size() > 0)
                std::advance(end, items.size());

        std::vector<SearchResult> results;
        for (auto it = items.begin(); it != end; it++) {
                const auto user = it->second;
                results.push_back(SearchResult{QString::fromStdString(user.first),
                                               QString::fromStdString(user.second)});
        }

        return results;
}

std::vector<RoomMember>
Cache::getMembers(const std::string &room_id, std::size_t startIndex, std::size_t len)
{
        auto txn    = lmdb::txn::begin(env_, nullptr, MDB_RDONLY);
        auto db     = getMembersDb(txn, room_id);
        auto cursor = lmdb::cursor::open(txn, db);

        std::size_t currentIndex = 0;

        const auto endIndex = std::min(startIndex + len, db.size(txn));

        std::vector<RoomMember> members;

        std::string user_id, user_data;
        while (cursor.get(user_id, user_data, MDB_NEXT)) {
                if (currentIndex < startIndex) {
                        currentIndex += 1;
                        continue;
                }

                if (currentIndex >= endIndex)
                        break;

                try {
                        MemberInfo tmp = json::parse(user_data);
                        members.emplace_back(
                          RoomMember{QString::fromStdString(user_id),
                                     QString::fromStdString(tmp.name),
                                     QImage::fromData(image(txn, tmp.avatar_url))});
                } catch (const json::exception &e) {
                        nhlog::db()->warn("{}", e.what());
                }

                currentIndex += 1;
        }

        cursor.close();
        txn.commit();

        return members;
}

bool
Cache::isRoomMember(const std::string &user_id, const std::string &room_id)
{
        auto txn = lmdb::txn::begin(env_);
        auto db  = getMembersDb(txn, room_id);

        lmdb::val value;
        bool res = lmdb::dbi_get(txn, db, lmdb::val(user_id), value);
        txn.commit();

        return res;
}

void
Cache::saveTimelineMessages(lmdb::txn &txn,
                            const std::string &room_id,
                            const mtx::responses::Timeline &res)
{
        auto db = getMessagesDb(txn, room_id);

        using namespace mtx::events;
        using namespace mtx::events::state;

        for (const auto &e : res.events) {
                if (std::holds_alternative<RedactionEvent<msg::Redaction>>(e))
                        continue;

                json obj = json::object();

                obj["event"] = mtx::accessors::serialize_event(e);
                obj["token"] = res.prev_batch;

                lmdb::dbi_put(
                  txn,
                  db,
                  lmdb::val(std::to_string(obj["event"]["origin_server_ts"].get<uint64_t>())),
                  lmdb::val(obj.dump()));
        }
}

mtx::responses::Notifications
Cache::getTimelineMentionsForRoom(lmdb::txn &txn, const std::string &room_id)
{
        auto db = getMentionsDb(txn, room_id);

        if (db.size(txn) == 0) {
                return mtx::responses::Notifications{};
        }

        mtx::responses::Notifications notif;
        std::string event_id, msg;

        auto cursor = lmdb::cursor::open(txn, db);

        while (cursor.get(event_id, msg, MDB_NEXT)) {
                auto obj = json::parse(msg);

                if (obj.count("event") == 0)
                        continue;

                mtx::responses::Notification notification;
                mtx::responses::from_json(obj, notification);

                notif.notifications.push_back(notification);
        }
        cursor.close();

        std::reverse(notif.notifications.begin(), notif.notifications.end());

        return notif;
}

//! Add all notifications containing a user mention to the db.
void
Cache::saveTimelineMentions(const mtx::responses::Notifications &res)
{
        QMap<std::string, QList<mtx::responses::Notification>> notifsByRoom;

        // Sort into room-specific 'buckets'
        for (const auto &notif : res.notifications) {
                json val = notif;
                notifsByRoom[notif.room_id].push_back(notif);
        }

        auto txn = lmdb::txn::begin(env_);
        // Insert the entire set of mentions for each room at a time.
        QMap<std::string, QList<mtx::responses::Notification>>::const_iterator it =
          notifsByRoom.constBegin();
        auto end = notifsByRoom.constEnd();
        while (it != end) {
                nhlog::db()->debug("Storing notifications for " + it.key());
                saveTimelineMentions(txn, it.key(), std::move(it.value()));
                ++it;
        }

        txn.commit();
}

void
Cache::saveTimelineMentions(lmdb::txn &txn,
                            const std::string &room_id,
                            const QList<mtx::responses::Notification> &res)
{
        auto db = getMentionsDb(txn, room_id);

        using namespace mtx::events;
        using namespace mtx::events::state;

        for (const auto &notif : res) {
                const auto event_id = mtx::accessors::event_id(notif.event);

                // double check that we have the correct room_id...
                if (room_id.compare(notif.room_id) != 0) {
                        return;
                }

                json obj = notif;

                lmdb::dbi_put(txn, db, lmdb::val(event_id), lmdb::val(obj.dump()));
        }
}

void
Cache::markSentNotification(const std::string &event_id)
{
        auto txn = lmdb::txn::begin(env_);
        lmdb::dbi_put(txn, notificationsDb_, lmdb::val(event_id), lmdb::val(std::string("")));
        txn.commit();
}

void
Cache::removeReadNotification(const std::string &event_id)
{
        auto txn = lmdb::txn::begin(env_);

        lmdb::dbi_del(txn, notificationsDb_, lmdb::val(event_id), nullptr);

        txn.commit();
}

bool
Cache::isNotificationSent(const std::string &event_id)
{
        auto txn = lmdb::txn::begin(env_, nullptr, MDB_RDONLY);

        lmdb::val value;
        bool res = lmdb::dbi_get(txn, notificationsDb_, lmdb::val(event_id), value);
        txn.commit();

        return res;
}

std::vector<std::string>
Cache::getRoomIds(lmdb::txn &txn)
{
        auto db     = lmdb::dbi::open(txn, ROOMS_DB, MDB_CREATE);
        auto cursor = lmdb::cursor::open(txn, db);

        std::vector<std::string> rooms;

        std::string room_id, _unused;
        while (cursor.get(room_id, _unused, MDB_NEXT))
                rooms.emplace_back(room_id);

        cursor.close();

        return rooms;
}

void
Cache::deleteOldMessages()
{
        auto txn      = lmdb::txn::begin(env_);
        auto room_ids = getRoomIds(txn);

        for (const auto &id : room_ids) {
                auto msg_db = getMessagesDb(txn, id);

                std::string ts, event;
                uint64_t idx = 0;

                const auto db_size = msg_db.size(txn);
                if (db_size <= 3 * MAX_RESTORED_MESSAGES)
                        continue;

                nhlog::db()->info("[{}] message count: {}", id, db_size);

                auto cursor = lmdb::cursor::open(txn, msg_db);
                while (cursor.get(ts, event, MDB_NEXT)) {
                        idx += 1;

                        if (idx > MAX_RESTORED_MESSAGES)
                                lmdb::cursor_del(cursor);
                }

                cursor.close();

                nhlog::db()->info("[{}] updated message count: {}", id, msg_db.size(txn));
        }

        txn.commit();
}

void
Cache::deleteOldData() noexcept
{
        try {
                deleteOldMessages();
        } catch (const lmdb::error &e) {
                nhlog::db()->error("failed to delete old messages: {}", e.what());
        }
}

bool
Cache::hasEnoughPowerLevel(const std::vector<mtx::events::EventType> &eventTypes,
                           const std::string &room_id,
                           const std::string &user_id)
{
        using namespace mtx::events;
        using namespace mtx::events::state;

        auto txn = lmdb::txn::begin(env_);
        auto db  = getStatesDb(txn, room_id);

        uint16_t min_event_level = std::numeric_limits<uint16_t>::max();
        uint16_t user_level      = std::numeric_limits<uint16_t>::min();

        lmdb::val event;
        bool res = lmdb::dbi_get(txn, db, lmdb::val(to_string(EventType::RoomPowerLevels)), event);

        if (res) {
                try {
                        StateEvent<PowerLevels> msg =
                          json::parse(std::string(event.data(), event.size()));

                        user_level = msg.content.user_level(user_id);

                        for (const auto &ty : eventTypes)
                                min_event_level =
                                  std::min(min_event_level,
                                           (uint16_t)msg.content.state_level(to_string(ty)));
                } catch (const json::exception &e) {
                        nhlog::db()->warn("failed to parse m.room.power_levels event: {}",
                                          e.what());
                }
        }

        txn.commit();

        return user_level >= min_event_level;
}

std::vector<std::string>
Cache::roomMembers(const std::string &room_id)
{
        auto txn = lmdb::txn::begin(env_, nullptr, MDB_RDONLY);

        std::vector<std::string> members;
        std::string user_id, unused;

        auto db = getMembersDb(txn, room_id);

        auto cursor = lmdb::cursor::open(txn, db);
        while (cursor.get(user_id, unused, MDB_NEXT))
                members.emplace_back(std::move(user_id));
        cursor.close();

        txn.commit();

        return members;
}

QHash<QString, QString> Cache::DisplayNames;
QHash<QString, QString> Cache::AvatarUrls;

QString
Cache::displayName(const QString &room_id, const QString &user_id)
{
        auto fmt = QString("%1 %2").arg(room_id).arg(user_id);
        if (DisplayNames.contains(fmt))
                return DisplayNames[fmt];

        return user_id;
}

std::string
Cache::displayName(const std::string &room_id, const std::string &user_id)
{
        auto fmt = QString::fromStdString(room_id + " " + user_id);
        if (DisplayNames.contains(fmt))
                return DisplayNames[fmt].toStdString();

        return user_id;
}

QString
Cache::avatarUrl(const QString &room_id, const QString &user_id)
{
        auto fmt = QString("%1 %2").arg(room_id).arg(user_id);
        if (AvatarUrls.contains(fmt))
                return AvatarUrls[fmt];

        return QString();
}

void
Cache::insertDisplayName(const QString &room_id,
                         const QString &user_id,
                         const QString &display_name)
{
        auto fmt = QString("%1 %2").arg(room_id).arg(user_id);
        DisplayNames.insert(fmt, display_name);
}

void
Cache::removeDisplayName(const QString &room_id, const QString &user_id)
{
        auto fmt = QString("%1 %2").arg(room_id).arg(user_id);
        DisplayNames.remove(fmt);
}

void
Cache::insertAvatarUrl(const QString &room_id, const QString &user_id, const QString &avatar_url)
{
        auto fmt = QString("%1 %2").arg(room_id).arg(user_id);
        AvatarUrls.insert(fmt, avatar_url);
}

void
Cache::removeAvatarUrl(const QString &room_id, const QString &user_id)
{
        auto fmt = QString("%1 %2").arg(room_id).arg(user_id);
        AvatarUrls.remove(fmt);
}

mtx::presence::PresenceState
Cache::presenceState(const std::string &user_id)
{
        lmdb::val presenceVal;

        auto txn = lmdb::txn::begin(env_);
        auto db  = getPresenceDb(txn);
        auto res = lmdb::dbi_get(txn, db, lmdb::val(user_id), presenceVal);

        mtx::presence::PresenceState state = mtx::presence::offline;

        if (res) {
                mtx::events::presence::Presence presence =
                  json::parse(std::string(presenceVal.data(), presenceVal.size()));
                state = presence.presence;
        }

        txn.commit();

        return state;
}

std::string
Cache::statusMessage(const std::string &user_id)
{
        lmdb::val presenceVal;

        auto txn = lmdb::txn::begin(env_);
        auto db  = getPresenceDb(txn);
        auto res = lmdb::dbi_get(txn, db, lmdb::val(user_id), presenceVal);

        std::string status_msg;

        if (res) {
                mtx::events::presence::Presence presence =
                  json::parse(std::string(presenceVal.data(), presenceVal.size()));
                status_msg = presence.status_msg;
        }

        txn.commit();

        return status_msg;
}

void
to_json(json &j, const UserCache &info)
{
        j["user_id"]          = info.user_id;
        j["is_user_verified"] = info.is_user_verified;
        j["cross_verified"]   = info.cross_verified;
        j["keys"]             = info.keys;
}

void
from_json(const json &j, UserCache &info)
{
        info.user_id          = j.at("user_id");
        info.is_user_verified = j.at("is_user_verified");
        info.cross_verified   = j.at("cross_verified").get<std::vector<std::string>>();
        info.keys             = j.at("keys").get<mtx::responses::QueryKeys>();
}

UserCache
Cache::getUserCache(const std::string &user_id)
{
        lmdb::val verifiedVal;

        auto txn = lmdb::txn::begin(env_);
        auto db  = getUserCacheDb(txn);
        auto res = lmdb::dbi_get(txn, db, lmdb::val(user_id), verifiedVal);

        UserCache verified_state;
        if (res) {
                verified_state = json::parse(std::string(verifiedVal.data(), verifiedVal.size()));
        }

        txn.commit();

        return verified_state;
}

//! be careful when using make sure is_user_verified is not changed
int
Cache::setUserCache(const std::string &user_id, const UserCache &body)
{
        auto txn = lmdb::txn::begin(env_);
        auto db  = getUserCacheDb(txn);

        auto res = lmdb::dbi_put(txn, db, lmdb::val(user_id), lmdb::val(json(body).dump()));

        txn.commit();

        return res;
}

int
Cache::deleteUserCache(const std::string &user_id)
{
        auto txn = lmdb::txn::begin(env_);
        auto db  = getUserCacheDb(txn);
        auto res = lmdb::dbi_del(txn, db, lmdb::val(user_id), nullptr);

        txn.commit();

        return res;
}

void
to_json(json &j, const DeviceVerifiedCache &info)
{
        j["user_id"]         = info.user_id;
        j["device_verified"] = info.device_verified;
}

void
from_json(const json &j, DeviceVerifiedCache &info)
{
        info.user_id         = j.at("user_id");
        info.device_verified = j.at("device_verified").get<std::vector<std::string>>();
}

DeviceVerifiedCache
Cache::getVerifiedCache(const std::string &user_id)
{
        lmdb::val verifiedVal;

        auto txn = lmdb::txn::begin(env_);
        auto db  = getDeviceVerifiedDb(txn);
        auto res = lmdb::dbi_get(txn, db, lmdb::val(user_id), verifiedVal);

        DeviceVerifiedCache verified_state;
        if (res) {
                verified_state = json::parse(std::string(verifiedVal.data(), verifiedVal.size()));
        }

        txn.commit();

        return verified_state;
}

int
Cache::setVerifiedCache(const std::string &user_id, const DeviceVerifiedCache &body)
{
        auto txn = lmdb::txn::begin(env_);
        auto db  = getDeviceVerifiedDb(txn);

        auto res = lmdb::dbi_put(txn, db, lmdb::val(user_id), lmdb::val(json(body).dump()));

        txn.commit();

        return res;
}

void
to_json(json &j, const RoomInfo &info)
{
        j["name"]         = info.name;
        j["topic"]        = info.topic;
        j["avatar_url"]   = info.avatar_url;
        j["version"]      = info.version;
        j["is_invite"]    = info.is_invite;
        j["join_rule"]    = info.join_rule;
        j["guest_access"] = info.guest_access;

        if (info.member_count != 0)
                j["member_count"] = info.member_count;

        if (info.tags.size() != 0)
                j["tags"] = info.tags;
}

void
from_json(const json &j, RoomInfo &info)
{
        info.name       = j.at("name");
        info.topic      = j.at("topic");
        info.avatar_url = j.at("avatar_url");
        info.version    = j.value(
          "version", QCoreApplication::translate("RoomInfo", "no version stored").toStdString());
        info.is_invite    = j.at("is_invite");
        info.join_rule    = j.at("join_rule");
        info.guest_access = j.at("guest_access");

        if (j.count("member_count"))
                info.member_count = j.at("member_count");

        if (j.count("tags"))
                info.tags = j.at("tags").get<std::vector<std::string>>();
}

void
to_json(json &j, const ReadReceiptKey &key)
{
        j = json{{"event_id", key.event_id}, {"room_id", key.room_id}};
}

void
from_json(const json &j, ReadReceiptKey &key)
{
        key.event_id = j.at("event_id").get<std::string>();
        key.room_id  = j.at("room_id").get<std::string>();
}

void
to_json(json &j, const MemberInfo &info)
{
        j["name"]       = info.name;
        j["avatar_url"] = info.avatar_url;
}

void
from_json(const json &j, MemberInfo &info)
{
        info.name       = j.at("name");
        info.avatar_url = j.at("avatar_url");
}

void
to_json(nlohmann::json &obj, const OutboundGroupSessionData &msg)
{
        obj["session_id"]    = msg.session_id;
        obj["session_key"]   = msg.session_key;
        obj["message_index"] = msg.message_index;
}

void
from_json(const nlohmann::json &obj, OutboundGroupSessionData &msg)
{
        msg.session_id    = obj.at("session_id");
        msg.session_key   = obj.at("session_key");
        msg.message_index = obj.at("message_index");
}

void
to_json(nlohmann::json &obj, const DevicePublicKeys &msg)
{
        obj["ed25519"]    = msg.ed25519;
        obj["curve25519"] = msg.curve25519;
}

void
from_json(const nlohmann::json &obj, DevicePublicKeys &msg)
{
        msg.ed25519    = obj.at("ed25519");
        msg.curve25519 = obj.at("curve25519");
}

void
to_json(nlohmann::json &obj, const MegolmSessionIndex &msg)
{
        obj["room_id"]    = msg.room_id;
        obj["session_id"] = msg.session_id;
        obj["sender_key"] = msg.sender_key;
}

void
from_json(const nlohmann::json &obj, MegolmSessionIndex &msg)
{
        msg.room_id    = obj.at("room_id");
        msg.session_id = obj.at("session_id");
        msg.sender_key = obj.at("sender_key");
}

namespace cache {
void
init(const QString &user_id)
{
        qRegisterMetaType<SearchResult>();
        qRegisterMetaType<std::vector<SearchResult>>();
        qRegisterMetaType<RoomMember>();
        qRegisterMetaType<RoomSearchResult>();
        qRegisterMetaType<RoomInfo>();
        qRegisterMetaType<QMap<QString, RoomInfo>>();
        qRegisterMetaType<std::map<QString, RoomInfo>>();
        qRegisterMetaType<std::map<QString, mtx::responses::Timeline>>();

        instance_ = std::make_unique<Cache>(user_id);
}

Cache *
client()
{
        return instance_.get();
}

std::string
displayName(const std::string &room_id, const std::string &user_id)
{
        return instance_->displayName(room_id, user_id);
}

QString
displayName(const QString &room_id, const QString &user_id)
{
        return instance_->displayName(room_id, user_id);
}
QString
avatarUrl(const QString &room_id, const QString &user_id)
{
        return instance_->avatarUrl(room_id, user_id);
}

void
removeDisplayName(const QString &room_id, const QString &user_id)
{
        instance_->removeDisplayName(room_id, user_id);
}
void
removeAvatarUrl(const QString &room_id, const QString &user_id)
{
        instance_->removeAvatarUrl(room_id, user_id);
}

void
insertDisplayName(const QString &room_id, const QString &user_id, const QString &display_name)
{
        instance_->insertDisplayName(room_id, user_id, display_name);
}
void
insertAvatarUrl(const QString &room_id, const QString &user_id, const QString &avatar_url)
{
        instance_->insertAvatarUrl(room_id, user_id, avatar_url);
}

mtx::presence::PresenceState
presenceState(const std::string &user_id)
{
        return instance_->presenceState(user_id);
}
std::string
statusMessage(const std::string &user_id)
{
        return instance_->statusMessage(user_id);
}
UserCache
getUserCache(const std::string &user_id)
{
        return instance_->getUserCache(user_id);
}

int
setUserCache(const std::string &user_id, const UserCache &body)
{
        return instance_->setUserCache(user_id, body);
}

int
deleteUserCache(const std::string &user_id)
{
        return instance_->deleteUserCache(user_id);
}

DeviceVerifiedCache
getVerifiedCache(const std::string &user_id)
{
        return instance_->getVerifiedCache(user_id);
}

int
setVerifiedCache(const std::string &user_id, const DeviceVerifiedCache &body)
{
        return instance_->setVerifiedCache(user_id, body);
}

//! Load saved data for the display names & avatars.
void
populateMembers()
{
        instance_->populateMembers();
}

std::vector<std::string>
joinedRooms()
{
        return instance_->joinedRooms();
}

QMap<QString, RoomInfo>
roomInfo(bool withInvites)
{
        return instance_->roomInfo(withInvites);
}
std::map<QString, bool>
invites()
{
        return instance_->invites();
}

QString
getRoomName(lmdb::txn &txn, lmdb::dbi &statesdb, lmdb::dbi &membersdb)
{
        return instance_->getRoomName(txn, statesdb, membersdb);
}
mtx::events::state::JoinRule
getRoomJoinRule(lmdb::txn &txn, lmdb::dbi &statesdb)
{
        return instance_->getRoomJoinRule(txn, statesdb);
}
bool
getRoomGuestAccess(lmdb::txn &txn, lmdb::dbi &statesdb)
{
        return instance_->getRoomGuestAccess(txn, statesdb);
}
QString
getRoomTopic(lmdb::txn &txn, lmdb::dbi &statesdb)
{
        return instance_->getRoomTopic(txn, statesdb);
}
QString
getRoomAvatarUrl(lmdb::txn &txn, lmdb::dbi &statesdb, lmdb::dbi &membersdb, const QString &room_id)
{
        return instance_->getRoomAvatarUrl(txn, statesdb, membersdb, room_id);
}

QString
getRoomVersion(lmdb::txn &txn, lmdb::dbi &statesdb)
{
        return instance_->getRoomVersion(txn, statesdb);
}

std::vector<RoomMember>
getMembers(const std::string &room_id, std::size_t startIndex, std::size_t len)
{
        return instance_->getMembers(room_id, startIndex, len);
}

void
saveState(const mtx::responses::Sync &res)
{
        instance_->saveState(res);
}
bool
isInitialized()
{
        return instance_->isInitialized();
}

std::string
nextBatchToken()
{
        return instance_->nextBatchToken();
}

void
deleteData()
{
        instance_->deleteData();
}

void
removeInvite(lmdb::txn &txn, const std::string &room_id)
{
        instance_->removeInvite(txn, room_id);
}
void
removeInvite(const std::string &room_id)
{
        instance_->removeInvite(room_id);
}
void
removeRoom(lmdb::txn &txn, const std::string &roomid)
{
        instance_->removeRoom(txn, roomid);
}
void
removeRoom(const std::string &roomid)
{
        instance_->removeRoom(roomid);
}
void
removeRoom(const QString &roomid)
{
        instance_->removeRoom(roomid.toStdString());
}
void
setup()
{
        instance_->setup();
}

bool
runMigrations()
{
        return instance_->runMigrations();
}

cache::CacheVersion
formatVersion()
{
        return instance_->formatVersion();
}

void
setCurrentFormat()
{
        instance_->setCurrentFormat();
}

std::map<QString, mtx::responses::Timeline>
roomMessages()
{
        return instance_->roomMessages();
}

QMap<QString, mtx::responses::Notifications>
getTimelineMentions()
{
        return instance_->getTimelineMentions();
}

//! Retrieve all the user ids from a room.
std::vector<std::string>
roomMembers(const std::string &room_id)
{
        return instance_->roomMembers(room_id);
}

//! Check if the given user has power leve greater than than
//! lowest power level of the given events.
bool
hasEnoughPowerLevel(const std::vector<mtx::events::EventType> &eventTypes,
                    const std::string &room_id,
                    const std::string &user_id)
{
        return instance_->hasEnoughPowerLevel(eventTypes, room_id, user_id);
}

//! Retrieves the saved room avatar.
QImage
getRoomAvatar(const QString &id)
{
        return instance_->getRoomAvatar(id);
}
QImage
getRoomAvatar(const std::string &id)
{
        return instance_->getRoomAvatar(id);
}

void
updateReadReceipt(lmdb::txn &txn, const std::string &room_id, const Receipts &receipts)
{
        instance_->updateReadReceipt(txn, room_id, receipts);
}

UserReceipts
readReceipts(const QString &event_id, const QString &room_id)
{
        return instance_->readReceipts(event_id, room_id);
}

QByteArray
image(const QString &url)
{
        return instance_->image(url);
}
QByteArray
image(lmdb::txn &txn, const std::string &url)
{
        return instance_->image(txn, url);
}
void
saveImage(const std::string &url, const std::string &data)
{
        instance_->saveImage(url, data);
}
void
saveImage(const QString &url, const QByteArray &data)
{
        instance_->saveImage(url, data);
}

RoomInfo
singleRoomInfo(const std::string &room_id)
{
        return instance_->singleRoomInfo(room_id);
}
std::vector<std::string>
roomsWithStateUpdates(const mtx::responses::Sync &res)
{
        return instance_->roomsWithStateUpdates(res);
}
std::vector<std::string>
roomsWithTagUpdates(const mtx::responses::Sync &res)
{
        return instance_->roomsWithTagUpdates(res);
}
std::map<QString, RoomInfo>
getRoomInfo(const std::vector<std::string> &rooms)
{
        return instance_->getRoomInfo(rooms);
}

//! Calculates which the read status of a room.
//! Whether all the events in the timeline have been read.
bool
calculateRoomReadStatus(const std::string &room_id)
{
        return instance_->calculateRoomReadStatus(room_id);
}
void
calculateRoomReadStatus()
{
        instance_->calculateRoomReadStatus();
}

std::vector<SearchResult>
searchUsers(const std::string &room_id, const std::string &query, std::uint8_t max_items)
{
        return instance_->searchUsers(room_id, query, max_items);
}
std::vector<RoomSearchResult>
searchRooms(const std::string &query, std::uint8_t max_items)
{
        return instance_->searchRooms(query, max_items);
}

void
markSentNotification(const std::string &event_id)
{
        instance_->markSentNotification(event_id);
}
//! Removes an event from the sent notifications.
void
removeReadNotification(const std::string &event_id)
{
        instance_->removeReadNotification(event_id);
}
//! Check if we have sent a desktop notification for the given event id.
bool
isNotificationSent(const std::string &event_id)
{
        return instance_->isNotificationSent(event_id);
}

//! Add all notifications containing a user mention to the db.
void
saveTimelineMentions(const mtx::responses::Notifications &res)
{
        instance_->saveTimelineMentions(res);
}

//! Remove old unused data.
void
deleteOldMessages()
{
        instance_->deleteOldMessages();
}
void
deleteOldData() noexcept
{
        instance_->deleteOldData();
}
//! Retrieve all saved room ids.
std::vector<std::string>
getRoomIds(lmdb::txn &txn)
{
        return instance_->getRoomIds(txn);
}

//! Mark a room that uses e2e encryption.
void
setEncryptedRoom(lmdb::txn &txn, const std::string &room_id)
{
        instance_->setEncryptedRoom(txn, room_id);
}
bool
isRoomEncrypted(const std::string &room_id)
{
        return instance_->isRoomEncrypted(room_id);
}

//! Check if a user is a member of the room.
bool
isRoomMember(const std::string &user_id, const std::string &room_id)
{
        return instance_->isRoomMember(user_id, room_id);
}

//
// Outbound Megolm Sessions
//
void
saveOutboundMegolmSession(const std::string &room_id,
                          const OutboundGroupSessionData &data,
                          mtx::crypto::OutboundGroupSessionPtr session)
{
        instance_->saveOutboundMegolmSession(room_id, data, std::move(session));
}
OutboundGroupSessionDataRef
getOutboundMegolmSession(const std::string &room_id)
{
        return instance_->getOutboundMegolmSession(room_id);
}
bool
outboundMegolmSessionExists(const std::string &room_id) noexcept
{
        return instance_->outboundMegolmSessionExists(room_id);
}
void
updateOutboundMegolmSession(const std::string &room_id, int message_index)
{
        instance_->updateOutboundMegolmSession(room_id, message_index);
}

void
importSessionKeys(const mtx::crypto::ExportedSessionKeys &keys)
{
        instance_->importSessionKeys(keys);
}
mtx::crypto::ExportedSessionKeys
exportSessionKeys()
{
        return instance_->exportSessionKeys();
}

//
// Inbound Megolm Sessions
//
void
saveInboundMegolmSession(const MegolmSessionIndex &index,
                         mtx::crypto::InboundGroupSessionPtr session)
{
        instance_->saveInboundMegolmSession(index, std::move(session));
}
OlmInboundGroupSession *
getInboundMegolmSession(const MegolmSessionIndex &index)
{
        return instance_->getInboundMegolmSession(index);
}
bool
inboundMegolmSessionExists(const MegolmSessionIndex &index)
{
        return instance_->inboundMegolmSessionExists(index);
}

//
// Olm Sessions
//
void
saveOlmSession(const std::string &curve25519, mtx::crypto::OlmSessionPtr session)
{
        instance_->saveOlmSession(curve25519, std::move(session));
}
std::vector<std::string>
getOlmSessions(const std::string &curve25519)
{
        return instance_->getOlmSessions(curve25519);
}
std::optional<mtx::crypto::OlmSessionPtr>
getOlmSession(const std::string &curve25519, const std::string &session_id)
{
        return instance_->getOlmSession(curve25519, session_id);
}

void
saveOlmAccount(const std::string &pickled)
{
        instance_->saveOlmAccount(pickled);
}
std::string
restoreOlmAccount()
{
        return instance_->restoreOlmAccount();
}

void
restoreSessions()
{
        return instance_->restoreSessions();
}
} // namespace cache
