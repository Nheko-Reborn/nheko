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
#include <QCryptographicHash>
#include <QFile>
#include <QHash>
#include <QMap>
#include <QStandardPaths>

#if __has_include(<keychain.h>)
#include <keychain.h>
#else
#include <qt5keychain/keychain.h>
#endif

#include <mtx/responses/common.hpp>

#include "Cache.h"
#include "Cache_p.h"
#include "ChatPage.h"
#include "EventAccessors.h"
#include "Logging.h"
#include "MatrixClient.h"
#include "Olm.h"
#include "UserSettingsPage.h"
#include "Utils.h"

//! Should be changed when a breaking change occurs in the cache format.
//! This will reset client's data.
static const std::string CURRENT_CACHE_FORMAT_VERSION("2020.10.20");
static const std::string SECRET("secret");

static lmdb::val NEXT_BATCH_KEY("next_batch");
static lmdb::val OLM_ACCOUNT_KEY("olm_account");
static lmdb::val CACHE_FORMAT_VERSION_KEY("cache_format_version");

constexpr size_t MAX_RESTORED_MESSAGES = 30'000;

constexpr auto DB_SIZE    = 32ULL * 1024ULL * 1024ULL * 1024ULL; // 32 GB
constexpr auto MAX_DBS    = 32384UL;
constexpr auto BATCH_SIZE = 100;

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

Q_DECLARE_METATYPE(RoomMember)
Q_DECLARE_METATYPE(mtx::responses::Timeline)
Q_DECLARE_METATYPE(RoomSearchResult)
Q_DECLARE_METATYPE(RoomInfo)
Q_DECLARE_METATYPE(mtx::responses::QueryKeys)

namespace {
std::unique_ptr<Cache> instance_ = nullptr;
}

bool
Cache::isHiddenEvent(lmdb::txn &txn,
                     mtx::events::collections::TimelineEvents e,
                     const std::string &room_id)
{
        using namespace mtx::events;
        if (auto encryptedEvent = std::get_if<EncryptedEvent<msg::Encrypted>>(&e)) {
                MegolmSessionIndex index;
                index.room_id    = room_id;
                index.session_id = encryptedEvent->content.session_id;
                index.sender_key = encryptedEvent->content.sender_key;

                auto result = olm::decryptEvent(index, *encryptedEvent);
                if (!result.error)
                        e = result.event.value();
        }

        mtx::events::account_data::nheko_extensions::HiddenEvents hiddenEvents;
        hiddenEvents.hidden_event_types = {
          EventType::Reaction, EventType::CallCandidates, EventType::Unsupported};

        if (auto temp = getAccountData(txn, mtx::events::EventType::NhekoHiddenEvents, ""))
                hiddenEvents =
                  std::move(std::get<mtx::events::AccountDataEvent<
                              mtx::events::account_data::nheko_extensions::HiddenEvents>>(*temp)
                              .content);
        if (auto temp = getAccountData(txn, mtx::events::EventType::NhekoHiddenEvents, room_id))
                hiddenEvents =
                  std::move(std::get<mtx::events::AccountDataEvent<
                              mtx::events::account_data::nheko_extensions::HiddenEvents>>(*temp)
                              .content);

        return std::visit(
          [hiddenEvents](const auto &ev) {
                  return std::any_of(hiddenEvents.hidden_event_types.begin(),
                                     hiddenEvents.hidden_event_types.end(),
                                     [ev](EventType type) { return type == ev.type; });
          },
          e);
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
        connect(this, &Cache::userKeysUpdate, this, &Cache::updateUserKeys, Qt::QueuedConnection);
}

void
Cache::setup()
{
        auto settings = UserSettings::instance();

        nhlog::db()->debug("setting up cache");

        // Previous location of the cache directory
        auto oldCache = QString("%1/%2%3")
                          .arg(QStandardPaths::writableLocation(QStandardPaths::CacheLocation))
                          .arg(QString::fromUtf8(localUserId_.toUtf8().toHex()))
                          .arg(QString::fromUtf8(settings->profile().toUtf8().toHex()));

        cacheDirectory_ = QString("%1/%2%3")
                            .arg(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
                            .arg(QString::fromUtf8(localUserId_.toUtf8().toHex()))
                            .arg(QString::fromUtf8(settings->profile().toUtf8().toHex()));

        bool isInitial = !QFile::exists(cacheDirectory_);

        // NOTE: If both cache directories exist it's better to do nothing: it
        // could mean a previous migration failed or was interrupted.
        bool needsMigration = isInitial && QFile::exists(oldCache);

        if (needsMigration) {
                nhlog::db()->info("found old state directory, migrating");
                if (!QDir().rename(oldCache, cacheDirectory_)) {
                        throw std::runtime_error(("Unable to migrate the old state directory (" +
                                                  oldCache + ") to the new location (" +
                                                  cacheDirectory_ + ")")
                                                   .toStdString()
                                                   .c_str());
                }
                nhlog::db()->info("completed state migration");
        }

        env_ = lmdb::env::create();
        env_.set_mapsize(DB_SIZE);
        env_.set_max_dbs(MAX_DBS);

        if (isInitial) {
                nhlog::db()->info("initializing LMDB");

                if (!QDir().mkpath(cacheDirectory_)) {
                        throw std::runtime_error(
                          ("Unable to create state directory:" + cacheDirectory_)
                            .toStdString()
                            .c_str());
                }
        }

        try {
                // NOTE(Nico): We may want to use (MDB_MAPASYNC | MDB_WRITEMAP) in the future, but
                // it can really mess up our database, so we shouldn't. For now, hopefully
                // NOMETASYNC is fast enough.
                env_.open(cacheDirectory_.toStdString().c_str(), MDB_NOMETASYNC | MDB_NOSYNC);
        } catch (const lmdb::error &e) {
                if (e.code() != MDB_VERSION_MISMATCH && e.code() != MDB_INVALID) {
                        throw std::runtime_error("LMDB initialization failed" +
                                                 std::string(e.what()));
                }

                nhlog::db()->warn("resetting cache due to LMDB version mismatch: {}", e.what());

                QDir stateDir(cacheDirectory_);

                for (const auto &file : stateDir.entryList(QDir::NoDotAndDotDot)) {
                        if (!stateDir.remove(file))
                                throw std::runtime_error(
                                  ("Unable to delete file " + file).toStdString().c_str());
                }
                env_.open(cacheDirectory_.toStdString().c_str());
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
}

mtx::crypto::InboundGroupSessionPtr
Cache::getInboundMegolmSession(const MegolmSessionIndex &index)
{
        using namespace mtx::crypto;

        try {
                auto txn        = lmdb::txn::begin(env_, nullptr, MDB_RDONLY);
                std::string key = json(index).dump();
                lmdb::val value;

                if (lmdb::dbi_get(txn, inboundMegolmSessionDb_, lmdb::val(key), value)) {
                        auto session = unpickle<InboundSessionObject>(
                          std::string(value.data(), value.size()), SECRET);
                        return session;
                }
        } catch (std::exception &e) {
                nhlog::db()->error("Failed to get inbound megolm session {}", e.what());
        }

        return nullptr;
}

bool
Cache::inboundMegolmSessionExists(const MegolmSessionIndex &index)
{
        using namespace mtx::crypto;

        try {
                auto txn        = lmdb::txn::begin(env_, nullptr, MDB_RDONLY);
                std::string key = json(index).dump();
                lmdb::val value;

                return lmdb::dbi_get(txn, inboundMegolmSessionDb_, lmdb::val(key), value);
        } catch (std::exception &e) {
                nhlog::db()->error("Failed to get inbound megolm session {}", e.what());
        }

        return false;
}

void
Cache::updateOutboundMegolmSession(const std::string &room_id,
                                   const OutboundGroupSessionData &data_,
                                   mtx::crypto::OutboundGroupSessionPtr &ptr)
{
        using namespace mtx::crypto;

        if (!outboundMegolmSessionExists(room_id))
                return;

        OutboundGroupSessionData data = data_;
        data.message_index            = olm_outbound_group_session_message_index(ptr.get());
        data.session_id               = mtx::crypto::session_id(ptr.get());
        data.session_key              = mtx::crypto::session_key(ptr.get());

        // Save the updated pickled data for the session.
        json j;
        j["data"]    = data;
        j["session"] = pickle<OutboundSessionObject>(ptr.get(), SECRET);

        auto txn = lmdb::txn::begin(env_);
        lmdb::dbi_put(txn, outboundMegolmSessionDb_, lmdb::val(room_id), lmdb::val(j.dump()));
        txn.commit();
}

void
Cache::dropOutboundMegolmSession(const std::string &room_id)
{
        using namespace mtx::crypto;

        if (!outboundMegolmSessionExists(room_id))
                return;

        {
                auto txn = lmdb::txn::begin(env_);
                lmdb::dbi_del(txn, outboundMegolmSessionDb_, lmdb::val(room_id), nullptr);
                txn.commit();
        }
}

void
Cache::saveOutboundMegolmSession(const std::string &room_id,
                                 const OutboundGroupSessionData &data,
                                 mtx::crypto::OutboundGroupSessionPtr &session)
{
        using namespace mtx::crypto;
        const auto pickled = pickle<OutboundSessionObject>(session.get(), SECRET);

        json j;
        j["data"]    = data;
        j["session"] = pickled;

        auto txn = lmdb::txn::begin(env_);
        lmdb::dbi_put(txn, outboundMegolmSessionDb_, lmdb::val(room_id), lmdb::val(j.dump()));
        txn.commit();
}

bool
Cache::outboundMegolmSessionExists(const std::string &room_id) noexcept
{
        try {
                auto txn = lmdb::txn::begin(env_, nullptr, MDB_RDONLY);
                lmdb::val value;
                return lmdb::dbi_get(txn, outboundMegolmSessionDb_, lmdb::val(room_id), value);
        } catch (std::exception &e) {
                nhlog::db()->error("Failed to retrieve outbound Megolm Session: {}", e.what());
                return false;
        }
}

OutboundGroupSessionDataRef
Cache::getOutboundMegolmSession(const std::string &room_id)
{
        try {
                using namespace mtx::crypto;

                auto txn = lmdb::txn::begin(env_, nullptr, MDB_RDONLY);
                lmdb::val value;
                lmdb::dbi_get(txn, outboundMegolmSessionDb_, lmdb::val(room_id), value);
                auto obj = json::parse(std::string_view(value.data(), value.size()));

                OutboundGroupSessionDataRef ref{};
                ref.data    = obj.at("data").get<OutboundGroupSessionData>();
                ref.session = unpickle<OutboundSessionObject>(obj.at("session"), SECRET);
                return ref;
        } catch (std::exception &e) {
                nhlog::db()->error("Failed to retrieve outbound Megolm Session: {}", e.what());
                return {};
        }
}

//
// OLM sessions.
//

void
Cache::saveOlmSession(const std::string &curve25519,
                      mtx::crypto::OlmSessionPtr session,
                      uint64_t timestamp)
{
        using namespace mtx::crypto;

        auto txn = lmdb::txn::begin(env_);
        auto db  = getOlmSessionsDb(txn, curve25519);

        const auto pickled    = pickle<SessionObject>(session.get(), SECRET);
        const auto session_id = mtx::crypto::session_id(session.get());

        StoredOlmSession stored_session;
        stored_session.pickled_session = pickled;
        stored_session.last_message_ts = timestamp;

        lmdb::dbi_put(txn, db, lmdb::val(session_id), lmdb::val(json(stored_session).dump()));

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
                std::string_view raw(pickled.data(), pickled.size());
                auto data = json::parse(raw).get<StoredOlmSession>();
                return unpickle<SessionObject>(data.pickled_session, SECRET);
        }

        return std::nullopt;
}

std::optional<mtx::crypto::OlmSessionPtr>
Cache::getLatestOlmSession(const std::string &curve25519)
{
        using namespace mtx::crypto;

        auto txn = lmdb::txn::begin(env_);
        auto db  = getOlmSessionsDb(txn, curve25519);

        std::string session_id, pickled_session;

        std::optional<StoredOlmSession> currentNewest;

        auto cursor = lmdb::cursor::open(txn, db);
        while (cursor.get(session_id, pickled_session, MDB_NEXT)) {
                auto data =
                  json::parse(std::string_view(pickled_session.data(), pickled_session.size()))
                    .get<StoredOlmSession>();
                if (!currentNewest || currentNewest->last_message_ts < data.last_message_ts)
                        currentNewest = data;
        }
        cursor.close();

        txn.commit();

        return currentNewest
                 ? std::optional(unpickle<SessionObject>(currentNewest->pickled_session, SECRET))
                 : std::nullopt;
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

std::string
Cache::restoreOlmAccount()
{
        auto txn = lmdb::txn::begin(env_, nullptr, MDB_RDONLY);
        lmdb::val pickled;
        lmdb::dbi_get(txn, syncStateDb_, OLM_ACCOUNT_KEY, pickled);
        txn.commit();

        return std::string(pickled.data(), pickled.size());
}

void
Cache::storeSecret(const std::string &name, const std::string &secret)
{
        auto settings = UserSettings::instance();
        QKeychain::WritePasswordJob job(QCoreApplication::applicationName());
        job.setAutoDelete(false);
        job.setInsecureFallback(true);
        job.setKey("matrix." +
                   QString(QCryptographicHash::hash(settings->profile().toUtf8(),
                                                    QCryptographicHash::Sha256)) +
                   "." + name.c_str());
        job.setTextData(QString::fromStdString(secret));
        QEventLoop loop;
        job.connect(&job, &QKeychain::Job::finished, &loop, &QEventLoop::quit);
        job.start();
        loop.exec();

        if (job.error()) {
                nhlog::db()->warn(
                  "Storing secret '{}' failed: {}", name, job.errorString().toStdString());
        } else {
                emit secretChanged(name);
        }
}

void
Cache::deleteSecret(const std::string &name)
{
        auto settings = UserSettings::instance();
        QKeychain::DeletePasswordJob job(QCoreApplication::applicationName());
        job.setAutoDelete(false);
        job.setInsecureFallback(true);
        job.setKey("matrix." +
                   QString(QCryptographicHash::hash(settings->profile().toUtf8(),
                                                    QCryptographicHash::Sha256)) +
                   "." + name.c_str());
        QEventLoop loop;
        job.connect(&job, &QKeychain::Job::finished, &loop, &QEventLoop::quit);
        job.start();
        loop.exec();

        emit secretChanged(name);
}

std::optional<std::string>
Cache::secret(const std::string &name)
{
        auto settings = UserSettings::instance();
        QKeychain::ReadPasswordJob job(QCoreApplication::applicationName());
        job.setAutoDelete(false);
        job.setInsecureFallback(true);
        job.setKey("matrix." +
                   QString(QCryptographicHash::hash(settings->profile().toUtf8(),
                                                    QCryptographicHash::Sha256)) +
                   "." + name.c_str());
        QEventLoop loop;
        job.connect(&job, &QKeychain::Job::finished, &loop, &QEventLoop::quit);
        job.start();
        loop.exec();

        const QString secret = job.textData();
        if (job.error()) {
                nhlog::db()->debug(
                  "Restoring secret '{}' failed: {}", name, job.errorString().toStdString());
                return std::nullopt;
        }

        return secret.toStdString();
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

                return QByteArray(image.data(), (int)image.size());
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

                return QByteArray(image.data(), (int)image.size());
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
        lmdb::dbi_drop(txn, getAccountDataDb(txn, roomid), true);
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

        auto result = lmdb::dbi_get(txn, syncStateDb_, NEXT_BATCH_KEY, token);

        txn.commit();

        if (result)
                return std::string(token.data(), token.size());
        else
                return "";
}

void
Cache::deleteData()
{
        // TODO: We need to remove the env_ while not accepting new requests.
        lmdb::dbi_close(env_, syncStateDb_);
        lmdb::dbi_close(env_, roomsDb_);
        lmdb::dbi_close(env_, invitesDb_);
        lmdb::dbi_close(env_, mediaDb_);
        lmdb::dbi_close(env_, readReceiptsDb_);
        lmdb::dbi_close(env_, notificationsDb_);

        lmdb::dbi_close(env_, devicesDb_);
        lmdb::dbi_close(env_, deviceKeysDb_);

        lmdb::dbi_close(env_, inboundMegolmSessionDb_);
        lmdb::dbi_close(env_, outboundMegolmSessionDb_);

        env_.close();

        verification_storage.status.clear();

        if (!cacheDirectory_.isEmpty()) {
                QDir(cacheDirectory_).removeRecursively();
                nhlog::db()->info("deleted cache files from disk");
        }

        deleteSecret(mtx::secret_storage::secrets::megolm_backup_v1);
        deleteSecret(mtx::secret_storage::secrets::cross_signing_master);
        deleteSecret(mtx::secret_storage::secrets::cross_signing_user_signing);
        deleteSecret(mtx::secret_storage::secrets::cross_signing_self_signing);
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
          {"2020.07.05",
           [this]() {
                   try {
                           auto txn      = lmdb::txn::begin(env_, nullptr);
                           auto room_ids = getRoomIds(txn);

                           for (const auto &room_id : room_ids) {
                                   try {
                                           auto messagesDb = lmdb::dbi::open(
                                             txn, std::string(room_id + "/messages").c_str());

                                           // keep some old messages and batch token
                                           {
                                                   auto roomsCursor =
                                                     lmdb::cursor::open(txn, messagesDb);
                                                   lmdb::val ts, stored_message;
                                                   bool start = true;
                                                   mtx::responses::Timeline oldMessages;
                                                   while (roomsCursor.get(ts,
                                                                          stored_message,
                                                                          start ? MDB_FIRST
                                                                                : MDB_NEXT)) {
                                                           start = false;

                                                           auto j = json::parse(std::string_view(
                                                             stored_message.data(),
                                                             stored_message.size()));

                                                           if (oldMessages.prev_batch.empty())
                                                                   oldMessages.prev_batch =
                                                                     j["token"].get<std::string>();
                                                           else if (j["token"] !=
                                                                    oldMessages.prev_batch)
                                                                   break;

                                                           mtx::events::collections::TimelineEvent
                                                             te;
                                                           mtx::events::collections::from_json(
                                                             j["event"], te);
                                                           oldMessages.events.push_back(te.data);
                                                   }
                                                   // messages were stored in reverse order, so we
                                                   // need to reverse them
                                                   std::reverse(oldMessages.events.begin(),
                                                                oldMessages.events.end());
                                                   // save messages using the new method
                                                   saveTimelineMessages(txn, room_id, oldMessages);
                                           }

                                           // delete old messages db
                                           lmdb::dbi_drop(txn, messagesDb, true);
                                   } catch (std::exception &e) {
                                           nhlog::db()->error(
                                             "While migrating messages from {}, ignoring error {}",
                                             room_id,
                                             e.what());
                                   }
                           }
                           txn.commit();
                   } catch (const lmdb::error &) {
                           nhlog::db()->critical(
                             "Failed to delete messages database in migration!");
                           return false;
                   }

                   nhlog::db()->info("Successfully deleted pending receipts database.");
                   return true;
           }},
          {"2020.10.20",
           [this]() {
                   try {
                           using namespace mtx::crypto;

                           auto txn = lmdb::txn::begin(env_);

                           auto mainDb = lmdb::dbi::open(txn, nullptr);

                           std::string dbName, ignored;
                           auto olmDbCursor = lmdb::cursor::open(txn, mainDb);
                           while (olmDbCursor.get(dbName, ignored, MDB_NEXT)) {
                                   // skip every db but olm session dbs
                                   nhlog::db()->debug("Db {}", dbName);
                                   if (dbName.find("olm_sessions/") != 0)
                                           continue;

                                   nhlog::db()->debug("Migrating {}", dbName);

                                   auto olmDb = lmdb::dbi::open(txn, dbName.c_str());

                                   std::string session_id, session_value;

                                   std::vector<std::pair<std::string, StoredOlmSession>> sessions;

                                   auto cursor = lmdb::cursor::open(txn, olmDb);
                                   while (cursor.get(session_id, session_value, MDB_NEXT)) {
                                           nhlog::db()->debug("session_id {}, session_value {}",
                                                              session_id,
                                                              session_value);
                                           StoredOlmSession session;
                                           bool invalid = false;
                                           for (auto c : session_value)
                                                   if (!isprint(c)) {
                                                           invalid = true;
                                                           break;
                                                   }
                                           if (invalid)
                                                   continue;

                                           nhlog::db()->debug("Not skipped");

                                           session.pickled_session = session_value;
                                           sessions.emplace_back(session_id, session);
                                   }
                                   cursor.close();

                                   olmDb.drop(txn, true);

                                   auto newDbName = dbName;
                                   newDbName.erase(0, sizeof("olm_sessions") - 1);
                                   newDbName = "olm_sessions.v2" + newDbName;

                                   auto newDb = lmdb::dbi::open(txn, newDbName.c_str(), MDB_CREATE);

                                   for (const auto &[key, value] : sessions) {
                                           nhlog::db()->debug("{}\n{}", key, json(value).dump());
                                           lmdb::dbi_put(txn,
                                                         newDb,
                                                         lmdb::val(key),
                                                         lmdb::val(json(value).dump()));
                                   }
                           }
                           olmDbCursor.close();

                           txn.commit();
                   } catch (const lmdb::error &) {
                           nhlog::db()->critical("Failed to migrate olm sessions,");
                           return false;
                   }

                   nhlog::db()->info("Successfully migrated olm sessions.");
                   return true;
           }},
        };

        nhlog::db()->info("Running migrations, this may take a while!");
        for (const auto &[target_version, migration] : migrations) {
                if (target_version > stored_version)
                        if (!migration()) {
                                nhlog::db()->critical("migration failure!");
                                return false;
                        }
        }
        nhlog::db()->info("Migrations finished.");

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
                        auto json_response =
                          json::parse(std::string_view(value.data(), value.size()));
                        auto values = json_response.get<std::map<std::string, uint64_t>>();

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
                                auto json_value = json::parse(
                                  std::string_view(prev_value.data(), prev_value.size()));

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
        auto local_user_id = this->localUserId_.toStdString();

        auto currentBatchToken = nextBatchToken();

        auto txn = lmdb::txn::begin(env_);

        setNextBatchToken(txn, res.next_batch);

        if (!res.account_data.events.empty()) {
                auto accountDataDb = getAccountDataDb(txn, "");
                for (const auto &ev : res.account_data.events)
                        std::visit(
                          [&txn, &accountDataDb](const auto &event) {
                                  auto j = json(event);
                                  lmdb::dbi_put(txn,
                                                accountDataDb,
                                                lmdb::val(j["type"].get<std::string>()),
                                                lmdb::val(j.dump()));
                          },
                          ev);
        }

        auto userKeyCacheDb = getUserKeysDb(txn);

        // Save joined rooms
        for (const auto &room : res.rooms.join) {
                auto statesdb  = getStatesDb(txn, room.first);
                auto membersdb = getMembersDb(txn, room.first);

                saveStateEvents(txn, statesdb, membersdb, room.first, room.second.state.events);
                saveStateEvents(txn, statesdb, membersdb, room.first, room.second.timeline.events);

                saveTimelineMessages(txn, room.first, room.second.timeline);

                RoomInfo updatedInfo;
                updatedInfo.name       = getRoomName(txn, statesdb, membersdb).toStdString();
                updatedInfo.topic      = getRoomTopic(txn, statesdb).toStdString();
                updatedInfo.avatar_url = getRoomAvatarUrl(txn, statesdb, membersdb).toStdString();
                updatedInfo.version    = getRoomVersion(txn, statesdb).toStdString();

                bool has_new_tags = false;
                // Process the account_data associated with this room
                if (!room.second.account_data.events.empty()) {
                        auto accountDataDb = getAccountDataDb(txn, room.first);

                        for (const auto &evt : room.second.account_data.events) {
                                std::visit(
                                  [&txn, &accountDataDb](const auto &event) {
                                          auto j = json(event);
                                          lmdb::dbi_put(txn,
                                                        accountDataDb,
                                                        lmdb::val(j["type"].get<std::string>()),
                                                        lmdb::val(j.dump()));
                                  },
                                  evt);

                                // for tag events
                                if (std::holds_alternative<AccountDataEvent<account_data::Tags>>(
                                      evt)) {
                                        auto tags_evt =
                                          std::get<AccountDataEvent<account_data::Tags>>(evt);
                                        has_new_tags = true;
                                        for (const auto &tag : tags_evt.content.tags) {
                                                updatedInfo.tags.push_back(tag.first);
                                        }
                                }
                                if (auto fr = std::get_if<mtx::events::AccountDataEvent<
                                      mtx::events::account_data::FullyRead>>(&evt)) {
                                        nhlog::db()->debug("Fully read: {}", fr->content.event_id);
                                }
                        }
                }
                if (!has_new_tags) {
                        // retrieve the old tags, they haven't changed
                        lmdb::val data;
                        if (lmdb::dbi_get(txn, roomsDb_, lmdb::val(room.first), data)) {
                                try {
                                        RoomInfo tmp =
                                          json::parse(std::string_view(data.data(), data.size()));
                                        updatedInfo.tags = tmp.tags;
                                } catch (const json::exception &e) {
                                        nhlog::db()->warn(
                                          "failed to parse room info: room_id ({}), {}: {}",
                                          room.first,
                                          std::string(data.data(), data.size()),
                                          e.what());
                                }
                        }
                }

                lmdb::dbi_put(
                  txn, roomsDb_, lmdb::val(room.first), lmdb::val(json(updatedInfo).dump()));

                for (const auto &e : room.second.ephemeral.events) {
                        if (auto receiptsEv = std::get_if<
                              mtx::events::EphemeralEvent<mtx::events::ephemeral::Receipt>>(&e)) {
                                Receipts receipts;

                                for (const auto &[event_id, userReceipts] :
                                     receiptsEv->content.receipts) {
                                        for (const auto &[user_id, receipt] : userReceipts.users) {
                                                receipts[event_id][user_id] = receipt.ts;
                                        }
                                }
                                updateReadReceipt(txn, room.first, receipts);
                        }
                }

                // Clean up non-valid invites.
                removeInvite(txn, room.first);
        }

        saveInvites(txn, res.rooms.invite);

        savePresence(txn, res.presence);

        markUserKeysOutOfDate(txn, userKeyCacheDb, res.device_lists.changed, currentBatchToken);
        deleteUserKeys(txn, userKeyCacheDb, res.device_lists.left);

        removeLeftRooms(txn, res.rooms.leave);

        txn.commit();

        std::map<QString, bool> readStatus;

        for (const auto &room : res.rooms.join) {
                for (const auto &e : room.second.ephemeral.events) {
                        if (auto receiptsEv = std::get_if<
                              mtx::events::EphemeralEvent<mtx::events::ephemeral::Receipt>>(&e)) {
                                std::vector<QString> receipts;

                                for (const auto &[event_id, userReceipts] :
                                     receiptsEv->content.receipts) {
                                        for (const auto &[user_id, receipt] : userReceipts.users) {
                                                (void)receipt;

                                                if (user_id != local_user_id) {
                                                        receipts.push_back(
                                                          QString::fromStdString(event_id));
                                                        break;
                                                }
                                        }
                                }
                                if (!receipts.empty())
                                        emit newReadReceipts(QString::fromStdString(room.first),
                                                             receipts);
                        }
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
                                  auto j   = json(msg);
                                  bool res = lmdb::dbi_put(txn,
                                                           statesdb,
                                                           lmdb::val(j["type"].get<std::string>()),
                                                           lmdb::val(j.dump()));

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
                        if (std::holds_alternative<AccountDataEvent<account_data::Tags>>(evt)) {
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
                        RoomInfo tmp     = json::parse(std::string_view(data.data(), data.size()));
                        tmp.member_count = getMembersDb(txn, room_id).size(txn);
                        tmp.join_rule    = getRoomJoinRule(txn, statesdb);
                        tmp.guest_access = getRoomGuestAccess(txn, statesdb);

                        txn.commit();

                        return tmp;
                } catch (const json::exception &e) {
                        nhlog::db()->warn("failed to parse room info: room_id ({}), {}: {}",
                                          room_id,
                                          std::string(data.data(), data.size()),
                                          e.what());
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
                                RoomInfo tmp =
                                  json::parse(std::string_view(data.data(), data.size()));
                                tmp.member_count = getMembersDb(txn, room).size(txn);
                                tmp.join_rule    = getRoomJoinRule(txn, statesdb);
                                tmp.guest_access = getRoomGuestAccess(txn, statesdb);

                                room_info.emplace(QString::fromStdString(room), std::move(tmp));
                        } catch (const json::exception &e) {
                                nhlog::db()->warn("failed to parse room info: room_id ({}), {}: {}",
                                                  room,
                                                  std::string(data.data(), data.size()),
                                                  e.what());
                        }
                } else {
                        // Check if the room is an invite.
                        if (lmdb::dbi_get(txn, invitesDb_, lmdb::val(room), data)) {
                                try {
                                        RoomInfo tmp =
                                          json::parse(std::string_view(data.data(), data.size()));
                                        tmp.member_count = getInviteMembersDb(txn, room).size(txn);

                                        room_info.emplace(QString::fromStdString(room),
                                                          std::move(tmp));
                                } catch (const json::exception &e) {
                                        nhlog::db()->warn("failed to parse room info for invite: "
                                                          "room_id ({}), {}: {}",
                                                          room,
                                                          std::string(data.data(), data.size()),
                                                          e.what());
                                }
                        }
                }
        }

        txn.commit();

        return room_info;
}

std::vector<QString>
Cache::roomIds()
{
        auto txn = lmdb::txn::begin(env_, nullptr, MDB_RDONLY);

        std::vector<QString> rooms;
        std::string room_id, unused;

        auto roomsCursor = lmdb::cursor::open(txn, roomsDb_);
        while (roomsCursor.get(room_id, unused, MDB_NEXT))
                rooms.push_back(QString::fromStdString(room_id));

        roomsCursor.close();
        txn.commit();

        return rooms;
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

std::string
Cache::previousBatchToken(const std::string &room_id)
{
        auto txn     = lmdb::txn::begin(env_, nullptr);
        auto orderDb = getEventOrderDb(txn, room_id);

        auto cursor = lmdb::cursor::open(txn, orderDb);
        lmdb::val indexVal, val;
        if (!cursor.get(indexVal, val, MDB_FIRST)) {
                return "";
        }

        auto j = json::parse(std::string_view(val.data(), val.size()));

        return j.value("prev_batch", "");
}

Cache::Messages
Cache::getTimelineMessages(lmdb::txn &txn, const std::string &room_id, uint64_t index, bool forward)
{
        // TODO(nico): Limit the messages returned by this maybe?
        auto orderDb  = getOrderToMessageDb(txn, room_id);
        auto eventsDb = getEventsDb(txn, room_id);

        Messages messages{};

        lmdb::val indexVal, event_id;

        auto cursor = lmdb::cursor::open(txn, orderDb);
        if (index == std::numeric_limits<uint64_t>::max()) {
                if (cursor.get(indexVal, event_id, forward ? MDB_FIRST : MDB_LAST)) {
                        index = *indexVal.data<uint64_t>();
                } else {
                        messages.end_of_cache = true;
                        return messages;
                }
        } else {
                if (cursor.get(indexVal, event_id, MDB_SET)) {
                        index = *indexVal.data<uint64_t>();
                } else {
                        messages.end_of_cache = true;
                        return messages;
                }
        }

        int counter = 0;

        bool ret;
        while ((ret = cursor.get(indexVal,
                                 event_id,
                                 counter == 0 ? (forward ? MDB_FIRST : MDB_LAST)
                                              : (forward ? MDB_NEXT : MDB_PREV))) &&
               counter++ < BATCH_SIZE) {
                lmdb::val event;
                bool success = lmdb::dbi_get(txn, eventsDb, event_id, event);
                if (!success)
                        continue;

                mtx::events::collections::TimelineEvent te;
                try {
                        mtx::events::collections::from_json(
                          json::parse(std::string_view(event.data(), event.size())), te);
                } catch (std::exception &e) {
                        nhlog::db()->error("Failed to parse message from cache {}", e.what());
                        continue;
                }

                messages.timeline.events.push_back(std::move(te.data));
        }
        cursor.close();

        // std::reverse(timeline.events.begin(), timeline.events.end());
        messages.next_index   = *indexVal.data<uint64_t>();
        messages.end_of_cache = !ret;

        return messages;
}

std::optional<mtx::events::collections::TimelineEvent>
Cache::getEvent(const std::string &room_id, const std::string &event_id)
{
        auto txn      = lmdb::txn::begin(env_, nullptr, MDB_RDONLY);
        auto eventsDb = getEventsDb(txn, room_id);

        lmdb::val event{};
        bool success = lmdb::dbi_get(txn, eventsDb, lmdb::val(event_id), event);
        if (!success)
                return {};

        mtx::events::collections::TimelineEvent te;
        try {
                mtx::events::collections::from_json(
                  json::parse(std::string_view(event.data(), event.size())), te);
        } catch (std::exception &e) {
                nhlog::db()->error("Failed to parse message from cache {}", e.what());
                return std::nullopt;
        }

        return te;
}
void
Cache::storeEvent(const std::string &room_id,
                  const std::string &event_id,
                  const mtx::events::collections::TimelineEvent &event)
{
        auto txn        = lmdb::txn::begin(env_);
        auto eventsDb   = getEventsDb(txn, room_id);
        auto event_json = mtx::accessors::serialize_event(event.data);
        lmdb::dbi_put(txn, eventsDb, lmdb::val(event_id), lmdb::val(event_json.dump()));
        txn.commit();
}

std::vector<std::string>
Cache::relatedEvents(const std::string &room_id, const std::string &event_id)
{
        auto txn         = lmdb::txn::begin(env_, nullptr, MDB_RDONLY);
        auto relationsDb = getRelationsDb(txn, room_id);

        std::vector<std::string> related_ids;

        auto related_cursor  = lmdb::cursor::open(txn, relationsDb);
        lmdb::val related_to = event_id, related_event;
        bool first           = true;

        try {
                if (!related_cursor.get(related_to, related_event, MDB_SET))
                        return {};

                while (related_cursor.get(
                  related_to, related_event, first ? MDB_FIRST_DUP : MDB_NEXT_DUP)) {
                        first = false;
                        if (event_id != std::string_view(related_to.data(), related_to.size()))
                                break;

                        related_ids.emplace_back(related_event.data(), related_event.size());
                }
        } catch (const lmdb::error &e) {
                nhlog::db()->error("related events error: {}", e.what());
        }

        return related_ids;
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
        lmdb::dbi orderDb{0};
        try {
                orderDb = getOrderToMessageDb(txn, room_id);
        } catch (lmdb::runtime_error &e) {
                nhlog::db()->error("Can't open db for room '{}', probably doesn't exist yet. ({})",
                                   room_id,
                                   e.what());
                return {};
        }

        lmdb::val indexVal, val;

        auto cursor = lmdb::cursor::open(txn, orderDb);
        if (!cursor.get(indexVal, val, MDB_LAST)) {
                return {};
        }

        return std::string(val.data(), val.size());
}

std::optional<Cache::TimelineRange>
Cache::getTimelineRange(const std::string &room_id)
{
        auto txn = lmdb::txn::begin(env_, nullptr, MDB_RDONLY);
        lmdb::dbi orderDb{0};
        try {
                orderDb = getOrderToMessageDb(txn, room_id);
        } catch (lmdb::runtime_error &e) {
                nhlog::db()->error("Can't open db for room '{}', probably doesn't exist yet. ({})",
                                   room_id,
                                   e.what());
                return {};
        }

        lmdb::val indexVal, val;

        auto cursor = lmdb::cursor::open(txn, orderDb);
        if (!cursor.get(indexVal, val, MDB_LAST)) {
                return {};
        }

        TimelineRange range{};
        range.last = *indexVal.data<uint64_t>();

        if (!cursor.get(indexVal, val, MDB_FIRST)) {
                return {};
        }
        range.first = *indexVal.data<uint64_t>();

        return range;
}
std::optional<uint64_t>
Cache::getTimelineIndex(const std::string &room_id, std::string_view event_id)
{
        auto txn = lmdb::txn::begin(env_, nullptr, MDB_RDONLY);

        lmdb::dbi orderDb{0};
        try {
                orderDb = getMessageToOrderDb(txn, room_id);
        } catch (lmdb::runtime_error &e) {
                nhlog::db()->error("Can't open db for room '{}', probably doesn't exist yet. ({})",
                                   room_id,
                                   e.what());
                return {};
        }

        lmdb::val indexVal{event_id.data(), event_id.size()}, val;

        bool success = lmdb::dbi_get(txn, orderDb, indexVal, val);
        if (!success) {
                return {};
        }

        return *val.data<uint64_t>();
}

std::optional<std::string>
Cache::getTimelineEventId(const std::string &room_id, uint64_t index)
{
        auto txn = lmdb::txn::begin(env_, nullptr, MDB_RDONLY);
        lmdb::dbi orderDb{0};
        try {
                orderDb = getOrderToMessageDb(txn, room_id);
        } catch (lmdb::runtime_error &e) {
                nhlog::db()->error("Can't open db for room '{}', probably doesn't exist yet. ({})",
                                   room_id,
                                   e.what());
                return {};
        }

        lmdb::val indexVal{&index, sizeof(index)}, val;

        bool success = lmdb::dbi_get(txn, orderDb, indexVal, val);
        if (!success) {
                return {};
        }

        return std::string(val.data(), val.size());
}

DescInfo
Cache::getLastMessageInfo(lmdb::txn &txn, const std::string &room_id)
{
        lmdb::dbi orderDb{0};
        try {
                orderDb = getOrderToMessageDb(txn, room_id);
        } catch (lmdb::runtime_error &e) {
                nhlog::db()->error("Can't open db for room '{}', probably doesn't exist yet. ({})",
                                   room_id,
                                   e.what());
                return {};
        }
        lmdb::dbi eventsDb{0};
        try {
                eventsDb = getEventsDb(txn, room_id);
        } catch (lmdb::runtime_error &e) {
                nhlog::db()->error("Can't open db for room '{}', probably doesn't exist yet. ({})",
                                   room_id,
                                   e.what());
                return {};
        }
        auto membersdb{0};

        try {
                membersdb = getMembersDb(txn, room_id);
        } catch (lmdb::runtime_error &e) {
                nhlog::db()->error("Can't open db for room '{}', probably doesn't exist yet. ({})",
                                   room_id,
                                   e.what());
                return {};
        }

        if (orderDb.size(txn) == 0)
                return DescInfo{};

        const auto local_user = utils::localUser();

        DescInfo fallbackDesc{};

        lmdb::val indexVal, event_id;

        auto cursor = lmdb::cursor::open(txn, orderDb);
        bool first  = true;
        while (cursor.get(indexVal, event_id, first ? MDB_LAST : MDB_PREV)) {
                first = false;

                lmdb::val event;
                bool success = lmdb::dbi_get(txn, eventsDb, event_id, event);
                if (!success)
                        continue;

                auto obj = json::parse(std::string_view(event.data(), event.size()));

                if (fallbackDesc.event_id.isEmpty() && obj["type"] == "m.room.member" &&
                    obj["state_key"] == local_user.toStdString() &&
                    obj["content"]["membership"] == "join") {
                        uint64_t ts  = obj["origin_server_ts"];
                        auto time    = QDateTime::fromMSecsSinceEpoch(ts);
                        fallbackDesc = DescInfo{QString::fromStdString(obj["event_id"]),
                                                local_user,
                                                tr("You joined this room."),
                                                utils::descriptiveTime(time),
                                                ts,
                                                time};
                }

                if (!(obj["type"] == "m.room.message" || obj["type"] == "m.sticker" ||
                      obj["type"] == "m.call.invite" || obj["type"] == "m.call.answer" ||
                      obj["type"] == "m.call.hangup" || obj["type"] == "m.room.encrypted"))
                        continue;

                mtx::events::collections::TimelineEvent te;
                mtx::events::collections::from_json(obj, te);

                lmdb::val info;
                MemberInfo m;
                if (lmdb::dbi_get(
                      txn, membersdb, lmdb::val(obj["sender"].get<std::string>()), info)) {
                        m = json::parse(std::string_view(info.data(), info.size()));
                }

                cursor.close();
                return utils::getMessageDescription(
                  te.data, local_user, QString::fromStdString(m.name));
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
Cache::getRoomAvatarUrl(lmdb::txn &txn, lmdb::dbi &statesdb, lmdb::dbi &membersdb)
{
        using namespace mtx::events;
        using namespace mtx::events::state;

        lmdb::val event;
        bool res = lmdb::dbi_get(
          txn, statesdb, lmdb::val(to_string(mtx::events::EventType::RoomAvatar)), event);

        if (res) {
                try {
                        StateEvent<Avatar> msg =
                          json::parse(std::string_view(event.data(), event.size()));

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
        std::string fallback_url;

        // Resolve avatar for 1-1 chats.
        while (cursor.get(user_id, member_data, MDB_NEXT)) {
                try {
                        MemberInfo m = json::parse(member_data);
                        if (user_id == localUserId_.toStdString()) {
                                fallback_url = m.avatar_url;
                                continue;
                        }

                        cursor.close();
                        return QString::fromStdString(m.avatar_url);
                } catch (const json::exception &e) {
                        nhlog::db()->warn("failed to parse member info: {}", e.what());
                }
        }

        cursor.close();

        // Default case when there is only one member.
        return QString::fromStdString(fallback_url);
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
                        StateEvent<Name> msg =
                          json::parse(std::string_view(event.data(), event.size()));

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
                          json::parse(std::string_view(event.data(), event.size()));

                        if (!msg.content.alias.empty())
                                return QString::fromStdString(msg.content.alias);
                } catch (const json::exception &e) {
                        nhlog::db()->warn("failed to parse m.room.canonical_alias event: {}",
                                          e.what());
                }
        }

        auto cursor      = lmdb::cursor::open(txn, membersdb);
        const auto total = membersdb.size(txn);

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
                          json::parse(std::string_view(event.data(), event.size()));
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
                          json::parse(std::string_view(event.data(), event.size()));
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
                          json::parse(std::string_view(event.data(), event.size()));

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
                          json::parse(std::string_view(event.data(), event.size()));

                        if (!msg.content.room_version.empty())
                                return QString::fromStdString(msg.content.room_version);
                } catch (const json::exception &e) {
                        nhlog::db()->warn("failed to parse m.room.create event: {}", e.what());
                }
        }

        nhlog::db()->warn("m.room.create event is missing room version, assuming version \"1\"");
        return QString("1");
}

std::optional<mtx::events::state::CanonicalAlias>
Cache::getRoomAliases(const std::string &roomid)
{
        using namespace mtx::events;
        using namespace mtx::events::state;

        auto txn      = lmdb::txn::begin(env_, nullptr, MDB_RDONLY);
        auto statesdb = getStatesDb(txn, roomid);

        lmdb::val event;
        bool res = lmdb::dbi_get(
          txn, statesdb, lmdb::val(to_string(mtx::events::EventType::RoomCanonicalAlias)), event);

        if (res) {
                try {
                        StateEvent<CanonicalAlias> msg =
                          json::parse(std::string_view(event.data(), event.size()));

                        return msg.content;
                } catch (const json::exception &e) {
                        nhlog::db()->warn("failed to parse m.room.canonical_alias event: {}",
                                          e.what());
                }
        }

        return std::nullopt;
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
                          json::parse(std::string_view(event.data(), event.size()));
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
                          json::parse(std::string_view(event.data(), event.size()));
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
                          json::parse(std::string_view(event.data(), event.size()));
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
                RoomInfo info = json::parse(std::string_view(response.data(), response.size()));
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

        return QImage::fromData(QByteArray(response.data(), (int)response.size()));
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

std::optional<MemberInfo>
Cache::getMember(const std::string &room_id, const std::string &user_id)
{
        try {
                auto txn = lmdb::txn::begin(env_, nullptr, MDB_RDONLY);

                auto membersdb = getMembersDb(txn, room_id);

                lmdb::val info;
                if (lmdb::dbi_get(txn, membersdb, lmdb::val(user_id), info)) {
                        MemberInfo m = json::parse(std::string_view(info.data(), info.size()));
                        return m;
                }
        } catch (std::exception &e) {
                nhlog::db()->warn("Failed to read member ({}): {}", user_id, e.what());
        }
        return std::nullopt;
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
Cache::savePendingMessage(const std::string &room_id,
                          const mtx::events::collections::TimelineEvent &message)
{
        auto txn = lmdb::txn::begin(env_);

        mtx::responses::Timeline timeline;
        timeline.events.push_back(message.data);
        saveTimelineMessages(txn, room_id, timeline);

        auto pending = getPendingMessagesDb(txn, room_id);

        int64_t now = QDateTime::currentMSecsSinceEpoch();
        lmdb::dbi_put(txn,
                      pending,
                      lmdb::val(&now, sizeof(now)),
                      lmdb::val(mtx::accessors::event_id(message.data)));

        txn.commit();
}

std::optional<mtx::events::collections::TimelineEvent>
Cache::firstPendingMessage(const std::string &room_id)
{
        auto txn     = lmdb::txn::begin(env_);
        auto pending = getPendingMessagesDb(txn, room_id);

        {
                auto pendingCursor = lmdb::cursor::open(txn, pending);
                lmdb::val tsIgnored, pendingTxn;
                while (pendingCursor.get(tsIgnored, pendingTxn, MDB_NEXT)) {
                        auto eventsDb = getEventsDb(txn, room_id);
                        lmdb::val event;
                        if (!lmdb::dbi_get(txn, eventsDb, pendingTxn, event)) {
                                lmdb::dbi_del(txn, pending, tsIgnored, pendingTxn);
                                continue;
                        }

                        try {
                                mtx::events::collections::TimelineEvent te;
                                mtx::events::collections::from_json(
                                  json::parse(std::string_view(event.data(), event.size())), te);

                                pendingCursor.close();
                                txn.commit();
                                return te;
                        } catch (std::exception &e) {
                                nhlog::db()->error("Failed to parse message from cache {}",
                                                   e.what());
                                lmdb::dbi_del(txn, pending, tsIgnored, pendingTxn);
                                continue;
                        }
                }
        }

        txn.commit();

        return std::nullopt;
}

void
Cache::removePendingStatus(const std::string &room_id, const std::string &txn_id)
{
        auto txn     = lmdb::txn::begin(env_);
        auto pending = getPendingMessagesDb(txn, room_id);

        {
                auto pendingCursor = lmdb::cursor::open(txn, pending);
                lmdb::val tsIgnored, pendingTxn;
                while (pendingCursor.get(tsIgnored, pendingTxn, MDB_NEXT)) {
                        if (std::string_view(pendingTxn.data(), pendingTxn.size()) == txn_id)
                                lmdb::cursor_del(pendingCursor);
                }
        }

        txn.commit();
}

void
Cache::saveTimelineMessages(lmdb::txn &txn,
                            const std::string &room_id,
                            const mtx::responses::Timeline &res)
{
        if (res.events.empty())
                return;

        auto eventsDb    = getEventsDb(txn, room_id);
        auto relationsDb = getRelationsDb(txn, room_id);

        auto orderDb     = getEventOrderDb(txn, room_id);
        auto evToOrderDb = getEventToOrderDb(txn, room_id);
        auto msg2orderDb = getMessageToOrderDb(txn, room_id);
        auto order2msgDb = getOrderToMessageDb(txn, room_id);
        auto pending     = getPendingMessagesDb(txn, room_id);

        if (res.limited) {
                lmdb::dbi_drop(txn, orderDb, false);
                lmdb::dbi_drop(txn, evToOrderDb, false);
                lmdb::dbi_drop(txn, msg2orderDb, false);
                lmdb::dbi_drop(txn, order2msgDb, false);
                lmdb::dbi_drop(txn, pending, true);
        }

        using namespace mtx::events;
        using namespace mtx::events::state;

        lmdb::val indexVal, val;
        uint64_t index = std::numeric_limits<uint64_t>::max() / 2;
        auto cursor    = lmdb::cursor::open(txn, orderDb);
        if (cursor.get(indexVal, val, MDB_LAST)) {
                index = *indexVal.data<int64_t>();
        }

        uint64_t msgIndex = std::numeric_limits<uint64_t>::max() / 2;
        auto msgCursor    = lmdb::cursor::open(txn, order2msgDb);
        if (msgCursor.get(indexVal, val, MDB_LAST)) {
                msgIndex = *indexVal.data<uint64_t>();
        }

        bool first = true;
        for (const auto &e : res.events) {
                auto event  = mtx::accessors::serialize_event(e);
                auto txn_id = mtx::accessors::transaction_id(e);

                std::string event_id_val = event.value("event_id", "");
                if (event_id_val.empty()) {
                        nhlog::db()->error("Event without id!");
                        continue;
                }

                lmdb::val event_id = event_id_val;

                json orderEntry        = json::object();
                orderEntry["event_id"] = event_id_val;
                if (first && !res.prev_batch.empty())
                        orderEntry["prev_batch"] = res.prev_batch;

                lmdb::val txn_order;
                if (!txn_id.empty() &&
                    lmdb::dbi_get(txn, evToOrderDb, lmdb::val(txn_id), txn_order)) {
                        lmdb::dbi_put(txn, eventsDb, event_id, lmdb::val(event.dump()));
                        lmdb::dbi_del(txn, eventsDb, lmdb::val(txn_id));

                        lmdb::val msg_txn_order;
                        if (lmdb::dbi_get(txn, msg2orderDb, lmdb::val(txn_id), msg_txn_order)) {
                                lmdb::dbi_put(txn, order2msgDb, msg_txn_order, event_id);
                                lmdb::dbi_put(txn, msg2orderDb, event_id, msg_txn_order);
                                lmdb::dbi_del(txn, msg2orderDb, lmdb::val(txn_id));
                        }

                        lmdb::dbi_put(txn, orderDb, txn_order, lmdb::val(orderEntry.dump()));
                        lmdb::dbi_put(txn, evToOrderDb, event_id, txn_order);
                        lmdb::dbi_del(txn, evToOrderDb, lmdb::val(txn_id));

                        if (event.contains("content") &&
                            event["content"].contains("m.relates_to")) {
                                auto temp         = event["content"]["m.relates_to"];
                                json relates_to_j = temp.contains("m.in_reply_to") &&
                                                        temp["m.in_reply_to"].is_object()
                                                      ? temp["m.in_reply_to"]["event_id"]
                                                      : temp["event_id"];
                                std::string relates_to =
                                  relates_to_j.is_string() ? relates_to_j.get<std::string>() : "";

                                if (!relates_to.empty()) {
                                        lmdb::dbi_del(txn,
                                                      relationsDb,
                                                      lmdb::val(relates_to),
                                                      lmdb::val(txn_id));
                                        lmdb::dbi_put(
                                          txn, relationsDb, lmdb::val(relates_to), event_id);
                                }
                        }

                        auto pendingCursor = lmdb::cursor::open(txn, pending);
                        lmdb::val tsIgnored, pendingTxn;
                        while (pendingCursor.get(tsIgnored, pendingTxn, MDB_NEXT)) {
                                if (std::string_view(pendingTxn.data(), pendingTxn.size()) ==
                                    txn_id)
                                        lmdb::cursor_del(pendingCursor);
                        }
                } else if (auto redaction =
                             std::get_if<mtx::events::RedactionEvent<mtx::events::msg::Redaction>>(
                               &e)) {
                        if (redaction->redacts.empty())
                                continue;

                        lmdb::val oldEvent;
                        bool success =
                          lmdb::dbi_get(txn, eventsDb, lmdb::val(redaction->redacts), oldEvent);
                        if (!success)
                                continue;

                        mtx::events::collections::TimelineEvent te;
                        try {
                                mtx::events::collections::from_json(
                                  json::parse(std::string_view(oldEvent.data(), oldEvent.size())),
                                  te);
                                // overwrite the content and add redation data
                                std::visit(
                                  [redaction](auto &ev) {
                                          ev.unsigned_data.redacted_because = *redaction;
                                          ev.unsigned_data.redacted_by      = redaction->event_id;
                                  },
                                  te.data);
                                event = mtx::accessors::serialize_event(te.data);
                                event["content"].clear();

                        } catch (std::exception &e) {
                                nhlog::db()->error("Failed to parse message from cache {}",
                                                   e.what());
                                continue;
                        }

                        lmdb::dbi_put(
                          txn, eventsDb, lmdb::val(redaction->redacts), lmdb::val(event.dump()));
                        lmdb::dbi_put(txn,
                                      eventsDb,
                                      lmdb::val(redaction->event_id),
                                      lmdb::val(json(*redaction).dump()));
                } else {
                        lmdb::dbi_put(txn, eventsDb, event_id, lmdb::val(event.dump()));

                        ++index;

                        first = false;

                        nhlog::db()->debug("saving '{}'", orderEntry.dump());

                        lmdb::cursor_put(cursor.handle(),
                                         lmdb::val(&index, sizeof(index)),
                                         lmdb::val(orderEntry.dump()),
                                         MDB_APPEND);
                        lmdb::dbi_put(txn, evToOrderDb, event_id, lmdb::val(&index, sizeof(index)));

                        // TODO(Nico): Allow blacklisting more event types in UI
                        if (!isHiddenEvent(txn, e, room_id)) {
                                ++msgIndex;
                                lmdb::cursor_put(msgCursor.handle(),
                                                 lmdb::val(&msgIndex, sizeof(msgIndex)),
                                                 event_id,
                                                 MDB_APPEND);

                                lmdb::dbi_put(txn,
                                              msg2orderDb,
                                              event_id,
                                              lmdb::val(&msgIndex, sizeof(msgIndex)));
                        }

                        if (event.contains("content") &&
                            event["content"].contains("m.relates_to")) {
                                auto temp         = event["content"]["m.relates_to"];
                                json relates_to_j = temp.contains("m.in_reply_to") &&
                                                        temp["m.in_reply_to"].is_object()
                                                      ? temp["m.in_reply_to"]["event_id"]
                                                      : temp["event_id"];
                                std::string relates_to =
                                  relates_to_j.is_string() ? relates_to_j.get<std::string>() : "";

                                if (!relates_to.empty())
                                        lmdb::dbi_put(
                                          txn, relationsDb, lmdb::val(relates_to), event_id);
                        }
                }
        }
}

uint64_t
Cache::saveOldMessages(const std::string &room_id, const mtx::responses::Messages &res)
{
        auto txn         = lmdb::txn::begin(env_);
        auto eventsDb    = getEventsDb(txn, room_id);
        auto relationsDb = getRelationsDb(txn, room_id);

        auto orderDb     = getEventOrderDb(txn, room_id);
        auto evToOrderDb = getEventToOrderDb(txn, room_id);
        auto msg2orderDb = getMessageToOrderDb(txn, room_id);
        auto order2msgDb = getOrderToMessageDb(txn, room_id);

        lmdb::val indexVal, val;
        uint64_t index = std::numeric_limits<uint64_t>::max() / 2;
        {
                auto cursor = lmdb::cursor::open(txn, orderDb);
                if (cursor.get(indexVal, val, MDB_FIRST)) {
                        index = *indexVal.data<uint64_t>();
                }
        }

        uint64_t msgIndex = std::numeric_limits<uint64_t>::max() / 2;
        {
                auto msgCursor = lmdb::cursor::open(txn, order2msgDb);
                if (msgCursor.get(indexVal, val, MDB_FIRST)) {
                        msgIndex = *indexVal.data<uint64_t>();
                }
        }

        if (res.chunk.empty()) {
                if (lmdb::dbi_get(txn, orderDb, lmdb::val(&index, sizeof(index)), val)) {
                        auto orderEntry = json::parse(std::string_view(val.data(), val.size()));
                        orderEntry["prev_batch"] = res.end;
                        lmdb::dbi_put(txn,
                                      orderDb,
                                      lmdb::val(&index, sizeof(index)),
                                      lmdb::val(orderEntry.dump()));
                        nhlog::db()->debug("saving '{}'", orderEntry.dump());
                        txn.commit();
                }
                return index;
        }

        std::string event_id_val;
        for (const auto &e : res.chunk) {
                if (std::holds_alternative<
                      mtx::events::RedactionEvent<mtx::events::msg::Redaction>>(e))
                        continue;

                auto event         = mtx::accessors::serialize_event(e);
                event_id_val       = event["event_id"].get<std::string>();
                lmdb::val event_id = event_id_val;
                lmdb::dbi_put(txn, eventsDb, event_id, lmdb::val(event.dump()));

                --index;

                json orderEntry        = json::object();
                orderEntry["event_id"] = event_id_val;

                nhlog::db()->debug("saving '{}'", orderEntry.dump());

                lmdb::dbi_put(
                  txn, orderDb, lmdb::val(&index, sizeof(index)), lmdb::val(orderEntry.dump()));
                lmdb::dbi_put(txn, evToOrderDb, event_id, lmdb::val(&index, sizeof(index)));

                // TODO(Nico): Allow blacklisting more event types in UI
                if (!isHiddenEvent(txn, e, room_id)) {
                        --msgIndex;
                        lmdb::dbi_put(
                          txn, order2msgDb, lmdb::val(&msgIndex, sizeof(msgIndex)), event_id);

                        lmdb::dbi_put(
                          txn, msg2orderDb, event_id, lmdb::val(&msgIndex, sizeof(msgIndex)));
                }

                if (event.contains("content") && event["content"].contains("m.relates_to")) {
                        auto temp = event["content"]["m.relates_to"];
                        json relates_to_j =
                          temp.contains("m.in_reply_to") && temp["m.in_reply_to"].is_object()
                            ? temp["m.in_reply_to"]["event_id"]
                            : temp["event_id"];
                        std::string relates_to =
                          relates_to_j.is_string() ? relates_to_j.get<std::string>() : "";

                        if (!relates_to.empty())
                                lmdb::dbi_put(txn, relationsDb, lmdb::val(relates_to), event_id);
                }
        }

        json orderEntry          = json::object();
        orderEntry["event_id"]   = event_id_val;
        orderEntry["prev_batch"] = res.end;
        lmdb::dbi_put(txn, orderDb, lmdb::val(&index, sizeof(index)), lmdb::val(orderEntry.dump()));
        nhlog::db()->debug("saving '{}'", orderEntry.dump());

        txn.commit();

        return msgIndex;
}

void
Cache::clearTimeline(const std::string &room_id)
{
        auto txn         = lmdb::txn::begin(env_);
        auto eventsDb    = getEventsDb(txn, room_id);
        auto relationsDb = getRelationsDb(txn, room_id);

        auto orderDb     = getEventOrderDb(txn, room_id);
        auto evToOrderDb = getEventToOrderDb(txn, room_id);
        auto msg2orderDb = getMessageToOrderDb(txn, room_id);
        auto order2msgDb = getOrderToMessageDb(txn, room_id);

        lmdb::val indexVal, val;
        auto cursor = lmdb::cursor::open(txn, orderDb);

        bool start                   = true;
        bool passed_pagination_token = false;
        while (cursor.get(indexVal, val, start ? MDB_LAST : MDB_PREV)) {
                start = false;
                json obj;

                try {
                        obj = json::parse(std::string_view(val.data(), val.size()));
                } catch (std::exception &) {
                        // workaround bug in the initial db format, where we sometimes didn't store
                        // json...
                        obj = {{"event_id", std::string(val.data(), val.size())}};
                }

                if (passed_pagination_token) {
                        if (obj.count("event_id") != 0) {
                                lmdb::val event_id = obj["event_id"].get<std::string>();
                                lmdb::dbi_del(txn, evToOrderDb, event_id);
                                lmdb::dbi_del(txn, eventsDb, event_id);

                                lmdb::dbi_del(txn, relationsDb, event_id);

                                lmdb::val order{};
                                bool exists = lmdb::dbi_get(txn, msg2orderDb, event_id, order);
                                if (exists) {
                                        lmdb::dbi_del(txn, order2msgDb, order);
                                        lmdb::dbi_del(txn, msg2orderDb, event_id);
                                }
                        }
                        lmdb::cursor_del(cursor);
                } else {
                        if (obj.count("prev_batch") != 0)
                                passed_pagination_token = true;
                }
        }

        auto msgCursor = lmdb::cursor::open(txn, order2msgDb);
        start          = true;
        while (msgCursor.get(indexVal, val, start ? MDB_LAST : MDB_PREV)) {
                start = false;

                lmdb::val eventId;
                bool innerStart = true;
                bool found      = false;
                while (cursor.get(indexVal, eventId, innerStart ? MDB_LAST : MDB_PREV)) {
                        innerStart = false;

                        json obj;
                        try {
                                obj = json::parse(std::string_view(eventId.data(), eventId.size()));
                        } catch (std::exception &) {
                                obj = {{"event_id", std::string(eventId.data(), eventId.size())}};
                        }

                        if (obj["event_id"] == std::string(val.data(), val.size())) {
                                found = true;
                                break;
                        }
                }

                if (!found)
                        break;
        }

        do {
                lmdb::cursor_del(msgCursor);
        } while (msgCursor.get(indexVal, val, MDB_PREV));

        cursor.close();
        msgCursor.close();
        txn.commit();
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
        lmdb::val indexVal, val;

        auto txn      = lmdb::txn::begin(env_);
        auto room_ids = getRoomIds(txn);

        for (const auto &room_id : room_ids) {
                auto orderDb     = getEventOrderDb(txn, room_id);
                auto evToOrderDb = getEventToOrderDb(txn, room_id);
                auto o2m         = getOrderToMessageDb(txn, room_id);
                auto m2o         = getMessageToOrderDb(txn, room_id);
                auto eventsDb    = getEventsDb(txn, room_id);
                auto relationsDb = getRelationsDb(txn, room_id);
                auto cursor      = lmdb::cursor::open(txn, orderDb);

                uint64_t first, last;
                if (cursor.get(indexVal, val, MDB_LAST)) {
                        last = *indexVal.data<uint64_t>();
                } else {
                        continue;
                }
                if (cursor.get(indexVal, val, MDB_FIRST)) {
                        first = *indexVal.data<uint64_t>();
                } else {
                        continue;
                }

                size_t message_count = static_cast<size_t>(last - first);
                if (message_count < MAX_RESTORED_MESSAGES)
                        continue;

                bool start = true;
                while (cursor.get(indexVal, val, start ? MDB_FIRST : MDB_NEXT) &&
                       message_count-- > MAX_RESTORED_MESSAGES) {
                        start    = false;
                        auto obj = json::parse(std::string_view(val.data(), val.size()));

                        if (obj.count("event_id") != 0) {
                                lmdb::val event_id = obj["event_id"].get<std::string>();
                                lmdb::dbi_del(txn, evToOrderDb, event_id);
                                lmdb::dbi_del(txn, eventsDb, event_id);

                                lmdb::dbi_del(txn, relationsDb, event_id);

                                lmdb::val order{};
                                bool exists = lmdb::dbi_get(txn, m2o, event_id, order);
                                if (exists) {
                                        lmdb::dbi_del(txn, o2m, order);
                                        lmdb::dbi_del(txn, m2o, event_id);
                                }
                        }
                        lmdb::cursor_del(cursor);
                }
                cursor.close();
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

std::optional<mtx::events::collections::RoomAccountDataEvents>
Cache::getAccountData(lmdb::txn &txn, mtx::events::EventType type, const std::string &room_id)
{
        try {
                auto db = getAccountDataDb(txn, room_id);

                lmdb::val data;
                if (lmdb::dbi_get(txn, db, lmdb::val(to_string(type)), data)) {
                        mtx::responses::utils::RoomAccountDataEvents events;
                        mtx::responses::utils::parse_room_account_data_events(
                          std::string_view(data.data(), data.size()), events);
                        return events.front();
                }
        } catch (...) {
        }
        return std::nullopt;
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
                          json::parse(std::string_view(event.data(), event.size()));

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

std::map<std::string, std::optional<UserKeyCache>>
Cache::getMembersWithKeys(const std::string &room_id)
{
        lmdb::val keys;

        try {
                auto txn = lmdb::txn::begin(env_, nullptr, MDB_RDONLY);
                std::map<std::string, std::optional<UserKeyCache>> members;

                auto db     = getMembersDb(txn, room_id);
                auto keysDb = getUserKeysDb(txn);

                std::string user_id, unused;
                auto cursor = lmdb::cursor::open(txn, db);
                while (cursor.get(user_id, unused, MDB_NEXT)) {
                        auto res = lmdb::dbi_get(txn, keysDb, lmdb::val(user_id), keys);

                        if (res) {
                                members[user_id] =
                                  json::parse(std::string_view(keys.data(), keys.size()))
                                    .get<UserKeyCache>();
                        } else {
                                members[user_id] = {};
                        }
                }
                cursor.close();

                return members;
        } catch (std::exception &) {
                return {};
        }
}

QString
Cache::displayName(const QString &room_id, const QString &user_id)
{
        if (auto info = getMember(room_id.toStdString(), user_id.toStdString());
            info && !info->name.empty())
                return QString::fromStdString(info->name);

        return user_id;
}

std::string
Cache::displayName(const std::string &room_id, const std::string &user_id)
{
        if (auto info = getMember(room_id, user_id); info && !info->name.empty())
                return info->name;

        return user_id;
}

QString
Cache::avatarUrl(const QString &room_id, const QString &user_id)
{
        if (auto info = getMember(room_id.toStdString(), user_id.toStdString());
            info && !info->avatar_url.empty())
                return QString::fromStdString(info->avatar_url);

        return "";
}

mtx::presence::PresenceState
Cache::presenceState(const std::string &user_id)
{
        if (user_id.empty())
                return {};

        lmdb::val presenceVal;

        auto txn = lmdb::txn::begin(env_);
        auto db  = getPresenceDb(txn);
        auto res = lmdb::dbi_get(txn, db, lmdb::val(user_id), presenceVal);

        mtx::presence::PresenceState state = mtx::presence::offline;

        if (res) {
                mtx::events::presence::Presence presence =
                  json::parse(std::string_view(presenceVal.data(), presenceVal.size()));
                state = presence.presence;
        }

        txn.commit();

        return state;
}

std::string
Cache::statusMessage(const std::string &user_id)
{
        if (user_id.empty())
                return {};

        lmdb::val presenceVal;

        auto txn = lmdb::txn::begin(env_);
        auto db  = getPresenceDb(txn);
        auto res = lmdb::dbi_get(txn, db, lmdb::val(user_id), presenceVal);

        std::string status_msg;

        if (res) {
                mtx::events::presence::Presence presence =
                  json::parse(std::string_view(presenceVal.data(), presenceVal.size()));
                status_msg = presence.status_msg;
        }

        txn.commit();

        return status_msg;
}

void
to_json(json &j, const UserKeyCache &info)
{
        j["device_keys"]       = info.device_keys;
        j["master_keys"]       = info.master_keys;
        j["user_signing_keys"] = info.user_signing_keys;
        j["self_signing_keys"] = info.self_signing_keys;
        j["updated_at"]        = info.updated_at;
        j["last_changed"]      = info.last_changed;
}

void
from_json(const json &j, UserKeyCache &info)
{
        info.device_keys = j.value("device_keys", std::map<std::string, mtx::crypto::DeviceKeys>{});
        info.master_keys = j.value("master_keys", mtx::crypto::CrossSigningKeys{});
        info.user_signing_keys = j.value("user_signing_keys", mtx::crypto::CrossSigningKeys{});
        info.self_signing_keys = j.value("self_signing_keys", mtx::crypto::CrossSigningKeys{});
        info.updated_at        = j.value("updated_at", "");
        info.last_changed      = j.value("last_changed", "");
}

std::optional<UserKeyCache>
Cache::userKeys(const std::string &user_id)
{
        lmdb::val keys;

        try {
                auto txn = lmdb::txn::begin(env_, nullptr, MDB_RDONLY);
                auto db  = getUserKeysDb(txn);
                auto res = lmdb::dbi_get(txn, db, lmdb::val(user_id), keys);

                if (res) {
                        return json::parse(std::string_view(keys.data(), keys.size()))
                          .get<UserKeyCache>();
                } else {
                        return {};
                }
        } catch (std::exception &) {
                return {};
        }
}

void
Cache::updateUserKeys(const std::string &sync_token, const mtx::responses::QueryKeys &keyQuery)
{
        auto txn = lmdb::txn::begin(env_);
        auto db  = getUserKeysDb(txn);

        std::map<std::string, UserKeyCache> updates;

        for (const auto &[user, keys] : keyQuery.device_keys)
                updates[user].device_keys = keys;
        for (const auto &[user, keys] : keyQuery.master_keys)
                updates[user].master_keys = keys;
        for (const auto &[user, keys] : keyQuery.user_signing_keys)
                updates[user].user_signing_keys = keys;
        for (const auto &[user, keys] : keyQuery.self_signing_keys)
                updates[user].self_signing_keys = keys;

        for (auto &[user, update] : updates) {
                nhlog::db()->debug("Updated user keys: {}", user);

                lmdb::val oldKeys;
                auto res = lmdb::dbi_get(txn, db, lmdb::val(user), oldKeys);

                if (res) {
                        auto last_changed =
                          json::parse(std::string_view(oldKeys.data(), oldKeys.size()))
                            .get<UserKeyCache>()
                            .last_changed;
                        // skip if we are tracking this and expect it to be up to date with the last
                        // sync token
                        if (!last_changed.empty() && last_changed != sync_token)
                                continue;
                }
                lmdb::dbi_put(txn, db, lmdb::val(user), lmdb::val(json(update).dump()));
        }

        txn.commit();

        std::map<std::string, VerificationStatus> tmp;
        const auto local_user = utils::localUser().toStdString();

        {
                std::unique_lock<std::mutex> lock(verification_storage.verification_storage_mtx);
                for (auto &[user_id, update] : updates) {
                        (void)update;
                        if (user_id == local_user) {
                                std::swap(tmp, verification_storage.status);
                        } else {
                                verification_storage.status.erase(user_id);
                        }
                }
        }
        for (auto &[user_id, update] : updates) {
                (void)update;
                if (user_id == local_user) {
                        for (const auto &[user, status] : tmp) {
                                (void)status;
                                emit verificationStatusChanged(user);
                        }
                } else {
                        emit verificationStatusChanged(user_id);
                }
        }
}

void
Cache::deleteUserKeys(lmdb::txn &txn, lmdb::dbi &db, const std::vector<std::string> &user_ids)
{
        for (const auto &user_id : user_ids)
                lmdb::dbi_del(txn, db, lmdb::val(user_id), nullptr);
}

void
Cache::markUserKeysOutOfDate(lmdb::txn &txn,
                             lmdb::dbi &db,
                             const std::vector<std::string> &user_ids,
                             const std::string &sync_token)
{
        mtx::requests::QueryKeys query;
        query.token = sync_token;

        for (const auto &user : user_ids) {
                nhlog::db()->debug("Marking user keys out of date: {}", user);

                lmdb::val oldKeys;
                auto res = lmdb::dbi_get(txn, db, lmdb::val(user), oldKeys);

                if (!res)
                        continue;

                auto cacheEntry =
                  json::parse(std::string_view(oldKeys.data(), oldKeys.size())).get<UserKeyCache>();
                cacheEntry.last_changed = sync_token;
                lmdb::dbi_put(txn, db, lmdb::val(user), lmdb::val(json(cacheEntry).dump()));

                query.device_keys[user] = {};
        }

        if (!query.device_keys.empty())
                http::client()->query_keys(query,
                                           [this, sync_token](const mtx::responses::QueryKeys &keys,
                                                              mtx::http::RequestErr err) {
                                                   if (err) {
                                                           nhlog::net()->warn(
                                                             "failed to query device keys: {} {}",
                                                             err->matrix_error.error,
                                                             static_cast<int>(err->status_code));
                                                           return;
                                                   }

                                                   emit userKeysUpdate(sync_token, keys);
                                           });
}

void
Cache::query_keys(const std::string &user_id,
                  std::function<void(const UserKeyCache &, mtx::http::RequestErr)> cb)
{
        auto cache_ = cache::userKeys(user_id);

        if (cache_.has_value()) {
                if (!cache_->updated_at.empty() && cache_->updated_at == cache_->last_changed) {
                        cb(cache_.value(), {});
                        return;
                }
        }

        mtx::requests::QueryKeys req;
        req.device_keys[user_id] = {};

        std::string last_changed;
        if (cache_)
                last_changed = cache_->last_changed;
        req.token = last_changed;

        http::client()->query_keys(req,
                                   [cb, user_id, last_changed](const mtx::responses::QueryKeys &res,
                                                               mtx::http::RequestErr err) {
                                           if (err) {
                                                   nhlog::net()->warn(
                                                     "failed to query device keys: {},{}",
                                                     err->matrix_error.errcode,
                                                     static_cast<int>(err->status_code));
                                                   cb({}, err);
                                                   return;
                                           }

                                           cache::updateUserKeys(last_changed, res);

                                           auto keys = cache::userKeys(user_id);
                                           cb(keys.value_or(UserKeyCache{}), err);
                                   });
}

void
to_json(json &j, const VerificationCache &info)
{
        j["device_verified"] = info.device_verified;
        j["device_blocked"]  = info.device_blocked;
}

void
from_json(const json &j, VerificationCache &info)
{
        info.device_verified = j.at("device_verified").get<std::vector<std::string>>();
        info.device_blocked  = j.at("device_blocked").get<std::vector<std::string>>();
}

std::optional<VerificationCache>
Cache::verificationCache(const std::string &user_id)
{
        lmdb::val verifiedVal;

        auto txn = lmdb::txn::begin(env_);
        auto db  = getVerificationDb(txn);

        try {
                VerificationCache verified_state;
                auto res = lmdb::dbi_get(txn, db, lmdb::val(user_id), verifiedVal);
                if (res) {
                        verified_state =
                          json::parse(std::string_view(verifiedVal.data(), verifiedVal.size()));
                        return verified_state;
                } else {
                        return {};
                }
        } catch (std::exception &) {
                return {};
        }
}

void
Cache::markDeviceVerified(const std::string &user_id, const std::string &key)
{
        lmdb::val val;

        auto txn = lmdb::txn::begin(env_);
        auto db  = getVerificationDb(txn);

        try {
                VerificationCache verified_state;
                auto res = lmdb::dbi_get(txn, db, lmdb::val(user_id), val);
                if (res) {
                        verified_state = json::parse(std::string_view(val.data(), val.size()));
                }

                for (const auto &device : verified_state.device_verified)
                        if (device == key)
                                return;

                verified_state.device_verified.push_back(key);
                lmdb::dbi_put(txn, db, lmdb::val(user_id), lmdb::val(json(verified_state).dump()));
                txn.commit();
        } catch (std::exception &) {
        }

        const auto local_user = utils::localUser().toStdString();
        std::map<std::string, VerificationStatus> tmp;
        {
                std::unique_lock<std::mutex> lock(verification_storage.verification_storage_mtx);
                if (user_id == local_user) {
                        std::swap(tmp, verification_storage.status);
                } else {
                        verification_storage.status.erase(user_id);
                }
        }
        if (user_id == local_user) {
                for (const auto &[user, status] : tmp) {
                        (void)status;
                        emit verificationStatusChanged(user);
                }
        } else {
                emit verificationStatusChanged(user_id);
        }
}

void
Cache::markDeviceUnverified(const std::string &user_id, const std::string &key)
{
        lmdb::val val;

        auto txn = lmdb::txn::begin(env_);
        auto db  = getVerificationDb(txn);

        try {
                VerificationCache verified_state;
                auto res = lmdb::dbi_get(txn, db, lmdb::val(user_id), val);
                if (res) {
                        verified_state = json::parse(std::string_view(val.data(), val.size()));
                }

                verified_state.device_verified.erase(
                  std::remove(verified_state.device_verified.begin(),
                              verified_state.device_verified.end(),
                              key),
                  verified_state.device_verified.end());

                lmdb::dbi_put(txn, db, lmdb::val(user_id), lmdb::val(json(verified_state).dump()));
                txn.commit();
        } catch (std::exception &) {
        }

        const auto local_user = utils::localUser().toStdString();
        std::map<std::string, VerificationStatus> tmp;
        {
                std::unique_lock<std::mutex> lock(verification_storage.verification_storage_mtx);
                if (user_id == local_user) {
                        std::swap(tmp, verification_storage.status);
                } else {
                        verification_storage.status.erase(user_id);
                }
        }
        if (user_id == local_user) {
                for (const auto &[user, status] : tmp) {
                        (void)status;
                        emit verificationStatusChanged(user);
                }
        } else {
                emit verificationStatusChanged(user_id);
        }
}

VerificationStatus
Cache::verificationStatus(const std::string &user_id)
{
        std::unique_lock<std::mutex> lock(verification_storage.verification_storage_mtx);
        if (verification_storage.status.count(user_id))
                return verification_storage.status.at(user_id);

        VerificationStatus status;

        if (auto verifCache = verificationCache(user_id)) {
                status.verified_devices = verifCache->device_verified;
        }

        const auto local_user = utils::localUser().toStdString();

        if (user_id == local_user)
                status.verified_devices.push_back(http::client()->device_id());

        verification_storage.status[user_id] = status;

        auto verifyAtLeastOneSig = [](const auto &toVerif,
                                      const std::map<std::string, std::string> &keys,
                                      const std::string &keyOwner) {
                if (!toVerif.signatures.count(keyOwner))
                        return false;

                for (const auto &[key_id, signature] : toVerif.signatures.at(keyOwner)) {
                        if (!keys.count(key_id))
                                continue;

                        if (mtx::crypto::ed25519_verify_signature(
                              keys.at(key_id), json(toVerif), signature))
                                return true;
                }
                return false;
        };

        try {
                // for local user verify this device_key -> our master_key -> our self_signing_key
                // -> our device_keys
                //
                // for other user verify this device_key -> our master_key -> our user_signing_key
                // -> their master_key -> their self_signing_key -> their device_keys
                //
                // This means verifying the other user adds 2 extra steps,verifying our user_signing
                // key and their master key
                auto ourKeys   = userKeys(local_user);
                auto theirKeys = userKeys(user_id);
                if (!ourKeys || !theirKeys)
                        return status;

                if (!mtx::crypto::ed25519_verify_signature(
                      olm::client()->identity_keys().ed25519,
                      json(ourKeys->master_keys),
                      ourKeys->master_keys.signatures.at(local_user)
                        .at("ed25519:" + http::client()->device_id())))
                        return status;

                auto master_keys = ourKeys->master_keys.keys;

                if (user_id != local_user) {
                        if (!verifyAtLeastOneSig(
                              ourKeys->user_signing_keys, master_keys, local_user))
                                return status;

                        if (!verifyAtLeastOneSig(
                              theirKeys->master_keys, ourKeys->user_signing_keys.keys, local_user))
                                return status;

                        master_keys = theirKeys->master_keys.keys;
                }

                status.user_verified = true;

                if (!verifyAtLeastOneSig(theirKeys->self_signing_keys, master_keys, user_id))
                        return status;

                for (const auto &[device, device_key] : theirKeys->device_keys) {
                        (void)device;
                        if (verifyAtLeastOneSig(
                              device_key, theirKeys->self_signing_keys.keys, user_id))
                                status.verified_devices.push_back(device_key.device_id);
                }

                verification_storage.status[user_id] = status;
                return status;
        } catch (std::exception &) {
                return status;
        }
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
to_json(nlohmann::json &obj, const DeviceAndMasterKeys &msg)
{
        obj["devices"]     = msg.devices;
        obj["master_keys"] = msg.master_keys;
}

void
from_json(const nlohmann::json &obj, DeviceAndMasterKeys &msg)
{
        msg.devices     = obj.at("devices").get<decltype(msg.devices)>();
        msg.master_keys = obj.at("master_keys").get<decltype(msg.master_keys)>();
}

void
to_json(nlohmann::json &obj, const SharedWithUsers &msg)
{
        obj["keys"] = msg.keys;
}

void
from_json(const nlohmann::json &obj, SharedWithUsers &msg)
{
        msg.keys = obj.at("keys").get<std::map<std::string, DeviceAndMasterKeys>>();
}

void
to_json(nlohmann::json &obj, const OutboundGroupSessionData &msg)
{
        obj["session_id"]    = msg.session_id;
        obj["session_key"]   = msg.session_key;
        obj["message_index"] = msg.message_index;

        obj["initially"] = msg.initially;
        obj["currently"] = msg.currently;
}

void
from_json(const nlohmann::json &obj, OutboundGroupSessionData &msg)
{
        msg.session_id    = obj.at("session_id");
        msg.session_key   = obj.at("session_key");
        msg.message_index = obj.at("message_index");

        msg.initially = obj.value("initially", SharedWithUsers{});
        msg.currently = obj.value("currently", SharedWithUsers{});
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

void
to_json(nlohmann::json &obj, const StoredOlmSession &msg)
{
        obj["ts"] = msg.last_message_ts;
        obj["s"]  = msg.pickled_session;
}
void
from_json(const nlohmann::json &obj, StoredOlmSession &msg)
{
        msg.last_message_ts = obj.at("ts").get<uint64_t>();
        msg.pickled_session = obj.at("s").get<std::string>();
}

namespace cache {
void
init(const QString &user_id)
{
        qRegisterMetaType<RoomMember>();
        qRegisterMetaType<RoomSearchResult>();
        qRegisterMetaType<RoomInfo>();
        qRegisterMetaType<QMap<QString, RoomInfo>>();
        qRegisterMetaType<std::map<QString, RoomInfo>>();
        qRegisterMetaType<std::map<QString, mtx::responses::Timeline>>();
        qRegisterMetaType<mtx::responses::QueryKeys>();

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

// user cache stores user keys
std::optional<UserKeyCache>
userKeys(const std::string &user_id)
{
        return instance_->userKeys(user_id);
}
void
updateUserKeys(const std::string &sync_token, const mtx::responses::QueryKeys &keyQuery)
{
        instance_->updateUserKeys(sync_token, keyQuery);
}

// device & user verification cache
std::optional<VerificationStatus>
verificationStatus(const std::string &user_id)
{
        return instance_->verificationStatus(user_id);
}

void
markDeviceVerified(const std::string &user_id, const std::string &device)
{
        instance_->markDeviceVerified(user_id, device);
}

void
markDeviceUnverified(const std::string &user_id, const std::string &device)
{
        instance_->markDeviceUnverified(user_id, device);
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
getRoomAvatarUrl(lmdb::txn &txn, lmdb::dbi &statesdb, lmdb::dbi &membersdb)
{
        return instance_->getRoomAvatarUrl(txn, statesdb, membersdb);
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

std::vector<QString>
roomIds()
{
        return instance_->roomIds();
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
                          mtx::crypto::OutboundGroupSessionPtr &session)
{
        instance_->saveOutboundMegolmSession(room_id, data, session);
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
updateOutboundMegolmSession(const std::string &room_id,
                            const OutboundGroupSessionData &data,
                            mtx::crypto::OutboundGroupSessionPtr &session)
{
        instance_->updateOutboundMegolmSession(room_id, data, session);
}
void
dropOutboundMegolmSession(const std::string &room_id)
{
        instance_->dropOutboundMegolmSession(room_id);
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
mtx::crypto::InboundGroupSessionPtr
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
saveOlmSession(const std::string &curve25519,
               mtx::crypto::OlmSessionPtr session,
               uint64_t timestamp)
{
        instance_->saveOlmSession(curve25519, std::move(session), timestamp);
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
std::optional<mtx::crypto::OlmSessionPtr>
getLatestOlmSession(const std::string &curve25519)
{
        return instance_->getLatestOlmSession(curve25519);
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
storeSecret(const std::string &name, const std::string &secret)
{
        instance_->storeSecret(name, secret);
}
std::optional<std::string>
secret(const std::string &name)
{
        return instance_->secret(name);
}
} // namespace cache
