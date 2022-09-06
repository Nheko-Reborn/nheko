// SPDX-FileCopyrightText: 2017 Konstantinos Sideris <siderisk@auth.gr>
// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Cache.h"
#include "Cache_p.h"

#include <stdexcept>
#include <unordered_set>
#include <variant>

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QHash>
#include <QMap>
#include <QMessageBox>
#include <QStandardPaths>

#if __has_include(<keychain.h>)
#include <keychain.h>
#else
#include <qt5keychain/keychain.h>
#endif

#include <mtx/responses/common.hpp>
#include <mtx/responses/messages.hpp>

#include "ChatPage.h"
#include "EventAccessors.h"
#include "Logging.h"
#include "MatrixClient.h"
#include "UserSettingsPage.h"
#include "Utils.h"
#include "encryption/Olm.h"

//! Should be changed when a breaking change occurs in the cache format.
//! This will reset client's data.
static const std::string CURRENT_CACHE_FORMAT_VERSION{"2022.04.08"};

//! Keys used for the DB
static const std::string_view NEXT_BATCH_KEY("next_batch");
static const std::string_view OLM_ACCOUNT_KEY("olm_account");
static const std::string_view CACHE_FORMAT_VERSION_KEY("cache_format_version");
static const std::string_view CURRENT_ONLINE_BACKUP_VERSION("current_online_backup_version");

constexpr auto MAX_DBS    = 32384UL;
constexpr auto BATCH_SIZE = 100;

#if Q_PROCESSOR_WORDSIZE >= 5 // 40-bit or more, up to 2^(8*WORDSIZE) words addressable.
constexpr auto DB_SIZE                 = 32ULL * 1024ULL * 1024ULL * 1024ULL; // 32 GB
constexpr size_t MAX_RESTORED_MESSAGES = 30'000;
#elif Q_PROCESSOR_WORDSIZE == 4 // 32-bit address space limits mmaps
constexpr auto DB_SIZE                 = 1ULL * 1024ULL * 1024ULL * 1024ULL; // 1 GB
constexpr size_t MAX_RESTORED_MESSAGES = 5'000;
#else
#error Not enough virtual address space for the database on target CPU
#endif

//! Cache databases and their format.
//!
//! Contains UI information for the joined rooms. (i.e name, topic, avatar url etc).
//! Format: room_id -> RoomInfo
constexpr auto ROOMS_DB("rooms");
constexpr auto INVITES_DB("invites");
//! maps each room to its parent space (id->id)
constexpr auto SPACES_PARENTS_DB("space_parents");
//! maps each space to its current children (id->id)
constexpr auto SPACES_CHILDREN_DB("space_children");
//! Information that  must be kept between sync requests.
constexpr auto SYNC_STATE_DB("sync_state");
//! Read receipts per room/event.
constexpr auto READ_RECEIPTS_DB("read_receipts");
constexpr auto NOTIFICATIONS_DB("sent_notifications");
constexpr auto PRESENCE_DB("presence");

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
//! MegolmSessionIndex -> session data about which devices have access to this
constexpr auto MEGOLM_SESSIONS_DATA_DB("megolm_sessions_data_db");

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

struct RO_txn
{
    ~RO_txn() { txn.reset(); }
    operator MDB_txn *() const noexcept { return txn.handle(); }
    operator lmdb::txn &() noexcept { return txn; }

    lmdb::txn &txn;
};

RO_txn
ro_txn(lmdb::env &env)
{
    thread_local lmdb::txn txn     = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
    thread_local int reuse_counter = 0;

    if (reuse_counter >= 100 || txn.env() != env.handle()) {
        txn.abort();
        txn           = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
        reuse_counter = 0;
    } else if (reuse_counter > 0) {
        try {
            txn.renew();
        } catch (...) {
            txn.abort();
            txn           = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
            reuse_counter = 0;
        }
    }
    reuse_counter++;

    return RO_txn{txn};
}

template<class T>
bool
containsStateUpdates(const T &e)
{
    return std::visit([](const auto &ev) { return Cache::isStateEvent_<decltype(ev)>; }, e);
}

bool
containsStateUpdates(const mtx::events::collections::StrippedEvents &e)
{
    using namespace mtx::events;
    using namespace mtx::events::state;

    return std::holds_alternative<StrippedEvent<state::Avatar>>(e) ||
           std::holds_alternative<StrippedEvent<CanonicalAlias>>(e) ||
           std::holds_alternative<StrippedEvent<Name>>(e) ||
           std::holds_alternative<StrippedEvent<Member>>(e) ||
           std::holds_alternative<StrippedEvent<Topic>>(e);
}

bool
Cache::isHiddenEvent(lmdb::txn &txn,
                     mtx::events::collections::TimelineEvents e,
                     const std::string &room_id)
{
    using namespace mtx::events;

    // Always hide edits
    if (mtx::accessors::relations(e).replaces())
        return true;

    if (auto encryptedEvent = std::get_if<EncryptedEvent<msg::Encrypted>>(&e)) {
        MegolmSessionIndex index;
        index.room_id    = room_id;
        index.session_id = encryptedEvent->content.session_id;

        auto result = olm::decryptEvent(index, *encryptedEvent, true);
        if (!result.error)
            e = result.event.value();
    }

    mtx::events::account_data::nheko_extensions::HiddenEvents hiddenEvents;
    hiddenEvents.hidden_event_types = std::vector{
      EventType::Reaction,
      EventType::CallCandidates,
      EventType::Unsupported,
    };

    if (auto temp = getAccountData(txn, mtx::events::EventType::NhekoHiddenEvents, "")) {
        auto h = std::get<
          mtx::events::AccountDataEvent<mtx::events::account_data::nheko_extensions::HiddenEvents>>(
          *temp);
        if (h.content.hidden_event_types)
            hiddenEvents = std::move(h.content);
    }
    if (auto temp = getAccountData(txn, mtx::events::EventType::NhekoHiddenEvents, room_id)) {
        auto h = std::get<
          mtx::events::AccountDataEvent<mtx::events::account_data::nheko_extensions::HiddenEvents>>(
          *temp);
        if (h.content.hidden_event_types)
            hiddenEvents = std::move(h.content);
    }

    return std::visit(
      [hiddenEvents](const auto &ev) {
          return std::any_of(hiddenEvents.hidden_event_types->begin(),
                             hiddenEvents.hidden_event_types->end(),
                             [ev](EventType type) { return type == ev.type; });
      },
      e);
}

Cache::Cache(const QString &userId, QObject *parent)
  : QObject{parent}
  , env_{nullptr}
  , localUserId_{userId}
{
    connect(this, &Cache::userKeysUpdate, this, &Cache::updateUserKeys, Qt::QueuedConnection);
    connect(
      this,
      &Cache::verificationStatusChanged,
      this,
      [this](const std::string &u) {
          if (u == localUserId_.toStdString()) {
              auto status = verificationStatus(u);
              emit selfVerificationStatusChanged();
          }
      },
      Qt::QueuedConnection);
    setup();
}

void
Cache::setup()
{
    auto settings = UserSettings::instance();

    nhlog::db()->debug("setting up cache");

    // Previous location of the cache directory
    auto oldCache =
      QStringLiteral("%1/%2%3").arg(QStandardPaths::writableLocation(QStandardPaths::CacheLocation),
                                    QString::fromUtf8(localUserId_.toUtf8().toHex()),
                                    QString::fromUtf8(settings->profile().toUtf8().toHex()));

    cacheDirectory_ = QStringLiteral("%1/%2%3").arg(
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation),
      QString::fromUtf8(localUserId_.toUtf8().toHex()),
      QString::fromUtf8(settings->profile().toUtf8().toHex()));

    bool isInitial = !QFile::exists(cacheDirectory_);

    // NOTE: If both cache directories exist it's better to do nothing: it
    // could mean a previous migration failed or was interrupted.
    bool needsMigration = isInitial && QFile::exists(oldCache);

    if (needsMigration) {
        nhlog::db()->info("found old state directory, migrating");
        if (!QDir().rename(oldCache, cacheDirectory_)) {
            throw std::runtime_error(("Unable to migrate the old state directory (" + oldCache +
                                      ") to the new location (" + cacheDirectory_ + ")")
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
              ("Unable to create state directory:" + cacheDirectory_).toStdString().c_str());
        }
    }

    try {
        // NOTE(Nico): We may want to use (MDB_MAPASYNC | MDB_WRITEMAP) in the future, but
        // it can really mess up our database, so we shouldn't. For now, hopefully
        // NOMETASYNC is fast enough.
        env_.open(cacheDirectory_.toStdString().c_str(), MDB_NOMETASYNC | MDB_NOSYNC);
    } catch (const lmdb::error &e) {
        if (e.code() != MDB_VERSION_MISMATCH && e.code() != MDB_INVALID) {
            throw std::runtime_error("LMDB initialization failed" + std::string(e.what()));
        }

        nhlog::db()->warn("resetting cache due to LMDB version mismatch: {}", e.what());

        QDir stateDir(cacheDirectory_);

        auto eList = stateDir.entryList(QDir::NoDotAndDotDot);
        for (const auto &file : qAsConst(eList)) {
            if (!stateDir.remove(file))
                throw std::runtime_error(("Unable to delete file " + file).toStdString().c_str());
        }
        env_.open(cacheDirectory_.toStdString().c_str());
    }

    auto txn          = lmdb::txn::begin(env_);
    syncStateDb_      = lmdb::dbi::open(txn, SYNC_STATE_DB, MDB_CREATE);
    roomsDb_          = lmdb::dbi::open(txn, ROOMS_DB, MDB_CREATE);
    spacesChildrenDb_ = lmdb::dbi::open(txn, SPACES_CHILDREN_DB, MDB_CREATE | MDB_DUPSORT);
    spacesParentsDb_  = lmdb::dbi::open(txn, SPACES_PARENTS_DB, MDB_CREATE | MDB_DUPSORT);
    invitesDb_        = lmdb::dbi::open(txn, INVITES_DB, MDB_CREATE);
    readReceiptsDb_   = lmdb::dbi::open(txn, READ_RECEIPTS_DB, MDB_CREATE);
    notificationsDb_  = lmdb::dbi::open(txn, NOTIFICATIONS_DB, MDB_CREATE);
    presenceDb_       = lmdb::dbi::open(txn, PRESENCE_DB, MDB_CREATE);

    // Device management
    devicesDb_    = lmdb::dbi::open(txn, DEVICES_DB, MDB_CREATE);
    deviceKeysDb_ = lmdb::dbi::open(txn, DEVICE_KEYS_DB, MDB_CREATE);

    // Session management
    inboundMegolmSessionDb_  = lmdb::dbi::open(txn, INBOUND_MEGOLM_SESSIONS_DB, MDB_CREATE);
    outboundMegolmSessionDb_ = lmdb::dbi::open(txn, OUTBOUND_MEGOLM_SESSIONS_DB, MDB_CREATE);
    megolmSessionDataDb_     = lmdb::dbi::open(txn, MEGOLM_SESSIONS_DATA_DB, MDB_CREATE);

    // What rooms are encrypted
    encryptedRooms_                      = lmdb::dbi::open(txn, ENCRYPTED_ROOMS_DB, MDB_CREATE);
    [[maybe_unused]] auto verificationDb = getVerificationDb(txn);
    [[maybe_unused]] auto userKeysDb     = getUserKeysDb(txn);

    txn.commit();

    loadSecrets({
      {mtx::secret_storage::secrets::cross_signing_master, false},
      {mtx::secret_storage::secrets::cross_signing_self_signing, false},
      {mtx::secret_storage::secrets::cross_signing_user_signing, false},
      {mtx::secret_storage::secrets::megolm_backup_v1, false},
      {"pickle_secret", true},
    });
}

static void
fatalSecretError()
{
    QMessageBox::critical(
      nullptr,
      QCoreApplication::translate("SecretStorage", "Failed to connect to secret storage"),
      QCoreApplication::translate(
        "SecretStorage",
        "Nheko could not connect to the secure storage to save encryption secrets to. This can "
        "have multiple reasons. Check if your D-Bus service is running and you have configured a "
        "service like KWallet, Gnome Keyring, KeePassXC or the equivalent for your platform. If "
        "you are having trouble, feel free to open an issue here: "
        "https://github.com/Nheko-Reborn/nheko/issues"));

    QCoreApplication::exit(1);
    exit(1);
}

static QString
secretName(std::string name, bool internal)
{
    auto settings = UserSettings::instance();
    return (internal ? "nheko." : "matrix.") +
           QString(
             QCryptographicHash::hash(settings->profile().toUtf8(), QCryptographicHash::Sha256)
               .toBase64()) +
           "." + QString::fromStdString(name);
}

void
Cache::loadSecrets(std::vector<std::pair<std::string, bool>> toLoad)
{
    auto settings = UserSettings::instance()->qsettings();

    if (toLoad.empty()) {
        this->databaseReady_ = true;
        emit databaseReady();
        nhlog::db()->debug("Database ready");
        return;
    }

    if (settings->value(QStringLiteral("run_without_secure_secrets_service"), false).toBool()) {
        for (auto &[name_, internal] : toLoad) {
            auto name  = secretName(name_, internal);
            auto value = settings->value("secrets/" + name).toString();
            if (value.isEmpty()) {
                nhlog::db()->info("Restored empty secret '{}'.", name.toStdString());
            } else {
                std::unique_lock lock(secret_storage.mtx);
                secret_storage.secrets[name.toStdString()] = value.toStdString();
            }
        }
        // if we emit the databaseReady signal directly it won't be received
        QTimer::singleShot(0, this, [this] { loadSecrets({}); });
        return;
    }

    auto [name_, internal] = toLoad.front();

    auto job = new QKeychain::ReadPasswordJob(QCoreApplication::applicationName());
    job->setAutoDelete(true);
    job->setInsecureFallback(true);
    job->setSettings(settings);
    auto name = secretName(name_, internal);
    job->setKey(name);

    connect(job,
            &QKeychain::ReadPasswordJob::finished,
            this,
            [this, name, toLoad, job](QKeychain::Job *) mutable {
                nhlog::db()->debug("Finished reading '{}'", toLoad.begin()->first);
                const QString secret = job->textData();
                if (job->error() && job->error() != QKeychain::Error::EntryNotFound) {
                    nhlog::db()->error("Restoring secret '{}' failed ({}): {}",
                                       name.toStdString(),
                                       job->error(),
                                       job->errorString().toStdString());

                    fatalSecretError();
                }
                if (secret.isEmpty()) {
                    nhlog::db()->debug("Restored empty secret '{}'.", name.toStdString());
                } else {
                    std::unique_lock lock(secret_storage.mtx);
                    secret_storage.secrets[name.toStdString()] = secret.toStdString();
                }

                // load next secret
                toLoad.erase(toLoad.begin());

                // You can't start a job from the finish signal of a job.
                QTimer::singleShot(0, this, [this, toLoad] { loadSecrets(toLoad); });
            });
    nhlog::db()->debug("Reading '{}'", name_);
    job->start();
}

std::optional<std::string>
Cache::secret(const std::string name_, bool internal)
{
    auto name = secretName(name_, internal);
    std::unique_lock lock(secret_storage.mtx);
    if (auto secret = secret_storage.secrets.find(name.toStdString());
        secret != secret_storage.secrets.end())
        return secret->second;
    else
        return std::nullopt;
}

void
Cache::storeSecret(const std::string name_, const std::string secret, bool internal)
{
    auto name = secretName(name_, internal);
    {
        std::unique_lock lock(secret_storage.mtx);
        secret_storage.secrets[name.toStdString()] = secret;
    }

    auto settings = UserSettings::instance()->qsettings();
    if (settings->value(QStringLiteral("run_without_secure_secrets_service"), false).toBool()) {
        settings->setValue("secrets/" + name, QString::fromStdString(secret));
        // if we emit the signal directly it won't be received
        QTimer::singleShot(0, this, [this, name_] { emit secretChanged(name_); });
        nhlog::db()->info("Storing secret '{}' successful", name_);
        return;
    }

    auto job = new QKeychain::WritePasswordJob(QCoreApplication::applicationName());
    job->setAutoDelete(true);
    job->setInsecureFallback(true);
    job->setSettings(settings);

    job->setKey(name);

    job->setTextData(QString::fromStdString(secret));

    QObject::connect(
      job,
      &QKeychain::WritePasswordJob::finished,
      this,
      [name_, this](QKeychain::Job *job) {
          if (job->error()) {
              nhlog::db()->warn(
                "Storing secret '{}' failed: {}", name_, job->errorString().toStdString());
              fatalSecretError();
          } else {
              // if we emit the signal directly, qtkeychain breaks and won't execute new
              // jobs. You can't start a job from the finish signal of a job.
              QTimer::singleShot(0, this, [this, name_] { emit secretChanged(name_); });
              nhlog::db()->info("Storing secret '{}' successful", name_);
          }
      },
      Qt::ConnectionType::DirectConnection);
    job->start();
}

void
Cache::deleteSecret(const std::string name, bool internal)
{
    auto name_ = secretName(name, internal);
    {
        std::unique_lock lock(secret_storage.mtx);
        secret_storage.secrets.erase(name_.toStdString());
    }

    auto settings = UserSettings::instance()->qsettings();
    if (settings->value(QStringLiteral("run_without_secure_secrets_service"), false).toBool()) {
        settings->remove("secrets/" + name_);
        // if we emit the signal directly it won't be received
        QTimer::singleShot(0, this, [this, name] { emit secretChanged(name); });
        return;
    }

    auto job = new QKeychain::DeletePasswordJob(QCoreApplication::applicationName());
    job->setAutoDelete(true);
    job->setInsecureFallback(true);
    job->setSettings(settings);

    job->setKey(name_);

    job->connect(
      job, &QKeychain::Job::finished, this, [this, name]() { emit secretChanged(name); });
    job->start();
}

std::string
Cache::pickleSecret()
{
    if (pickle_secret_.empty()) {
        auto s = secret("pickle_secret", true);
        if (!s) {
            this->pickle_secret_ = mtx::client::utils::random_token(64, true);
            storeSecret("pickle_secret", pickle_secret_, true);
        } else {
            this->pickle_secret_ = *s;
        }
    }

    return pickle_secret_;
}

void
Cache::setEncryptedRoom(lmdb::txn &txn, const std::string &room_id)
{
    nhlog::db()->info("mark room {} as encrypted", room_id);

    encryptedRooms_.put(txn, room_id, "0");
}

bool
Cache::isRoomEncrypted(const std::string &room_id)
{
    std::string_view unused;

    auto txn = ro_txn(env_);
    auto res = encryptedRooms_.get(txn, room_id, unused);

    return res;
}

std::optional<mtx::events::state::Encryption>
Cache::roomEncryptionSettings(const std::string &room_id)
{
    using namespace mtx::events;
    using namespace mtx::events::state;

    try {
        auto txn      = ro_txn(env_);
        auto statesdb = getStatesDb(txn, room_id);
        std::string_view event;
        bool res = statesdb.get(txn, to_string(mtx::events::EventType::RoomEncryption), event);

        if (res) {
            try {
                StateEvent<Encryption> msg =
                  nlohmann::json::parse(event).get<StateEvent<Encryption>>();

                return msg.content;
            } catch (const nlohmann::json::exception &e) {
                nhlog::db()->warn("failed to parse m.room.encryption event: {}", e.what());
                return Encryption{};
            }
        }
    } catch (lmdb::error &) {
    }

    return std::nullopt;
}

mtx::crypto::ExportedSessionKeys
Cache::exportSessionKeys()
{
    using namespace mtx::crypto;

    ExportedSessionKeys keys;

    auto txn    = ro_txn(env_);
    auto cursor = lmdb::cursor::open(txn, inboundMegolmSessionDb_);

    std::string_view key, value;
    while (cursor.get(key, value, MDB_NEXT)) {
        ExportedSession exported;
        MegolmSessionIndex index;

        auto saved_session = unpickle<InboundSessionObject>(std::string(value), pickle_secret_);

        try {
            index = nlohmann::json::parse(key).get<MegolmSessionIndex>();
        } catch (const nlohmann::json::exception &e) {
            nhlog::db()->critical("failed to export megolm session: {}", e.what());
            continue;
        }

        try {
            using namespace mtx::crypto;

            std::string_view v;
            if (megolmSessionDataDb_.get(txn, nlohmann::json(index).dump(), v)) {
                auto data           = nlohmann::json::parse(v).get<GroupSessionData>();
                exported.sender_key = data.sender_key;
                if (!data.sender_claimed_ed25519_key.empty())
                    exported.sender_claimed_keys["ed25519"] = data.sender_claimed_ed25519_key;
                exported.forwarding_curve25519_key_chain = data.forwarding_curve25519_key_chain;
            } else {
                continue;
            }

        } catch (std::exception &e) {
            nhlog::db()->error("Failed to retrieve Megolm Session Data: {}", e.what());
            continue;
        }

        exported.room_id     = index.room_id;
        exported.session_id  = index.session_id;
        exported.session_key = export_session(saved_session.get(), -1);

        keys.sessions.push_back(exported);
    }

    cursor.close();

    return keys;
}

void
Cache::importSessionKeys(const mtx::crypto::ExportedSessionKeys &keys)
{
    std::size_t importCount = 0;

    auto txn = lmdb::txn::begin(env_);
    for (const auto &s : keys.sessions) {
        MegolmSessionIndex index;
        index.room_id    = s.room_id;
        index.session_id = s.session_id;

        GroupSessionData data{};
        data.sender_key                      = s.sender_key;
        data.forwarding_curve25519_key_chain = s.forwarding_curve25519_key_chain;
        data.trusted                         = false;

        if (s.sender_claimed_keys.count("ed25519"))
            data.sender_claimed_ed25519_key = s.sender_claimed_keys.at("ed25519");

        try {
            auto exported_session = mtx::crypto::import_session(s.session_key);

            using namespace mtx::crypto;
            const auto key = nlohmann::json(index).dump();
            const auto pickled =
              pickle<InboundSessionObject>(exported_session.get(), pickle_secret_);

            std::string_view value;
            if (inboundMegolmSessionDb_.get(txn, key, value)) {
                auto oldSession =
                  unpickle<InboundSessionObject>(std::string(value), pickle_secret_);
                if (olm_inbound_group_session_first_known_index(exported_session.get()) >=
                    olm_inbound_group_session_first_known_index(oldSession.get())) {
                    nhlog::crypto()->warn(
                      "Not storing inbound session with newer or equal first known index");
                    continue;
                }
            }

            inboundMegolmSessionDb_.put(txn, key, pickled);
            megolmSessionDataDb_.put(txn, key, nlohmann::json(data).dump());

            ChatPage::instance()->receivedSessionKey(index.room_id, index.session_id);
            importCount++;
        } catch (const mtx::crypto::olm_exception &e) {
            nhlog::crypto()->critical(
              "failed to import inbound megolm session {}: {}", index.session_id, e.what());
            continue;
        } catch (const lmdb::error &e) {
            nhlog::crypto()->critical(
              "failed to save inbound megolm session {}: {}", index.session_id, e.what());
            continue;
        }
    }
    txn.commit();

    nhlog::crypto()->info("Imported {} out of {} keys", importCount, keys.sessions.size());
}

//
// Session Management
//

void
Cache::saveInboundMegolmSession(const MegolmSessionIndex &index,
                                mtx::crypto::InboundGroupSessionPtr session,
                                const GroupSessionData &data)
{
    using namespace mtx::crypto;
    const auto key     = nlohmann::json(index).dump();
    const auto pickled = pickle<InboundSessionObject>(session.get(), pickle_secret_);

    auto txn = lmdb::txn::begin(env_);

    std::string_view value;
    if (inboundMegolmSessionDb_.get(txn, key, value)) {
        auto oldSession = unpickle<InboundSessionObject>(std::string(value), pickle_secret_);
        if (olm_inbound_group_session_first_known_index(session.get()) >
            olm_inbound_group_session_first_known_index(oldSession.get())) {
            nhlog::crypto()->warn("Not storing inbound session with newer first known index");
            return;
        }
    }

    inboundMegolmSessionDb_.put(txn, key, pickled);
    megolmSessionDataDb_.put(txn, key, nlohmann::json(data).dump());
    txn.commit();
}

mtx::crypto::InboundGroupSessionPtr
Cache::getInboundMegolmSession(const MegolmSessionIndex &index)
{
    using namespace mtx::crypto;

    try {
        auto txn        = ro_txn(env_);
        std::string key = nlohmann::json(index).dump();
        std::string_view value;

        if (inboundMegolmSessionDb_.get(txn, key, value)) {
            auto session = unpickle<InboundSessionObject>(std::string(value), pickle_secret_);
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
        auto txn        = ro_txn(env_);
        std::string key = nlohmann::json(index).dump();
        std::string_view value;

        return inboundMegolmSessionDb_.get(txn, key, value);
    } catch (std::exception &e) {
        nhlog::db()->error("Failed to get inbound megolm session {}", e.what());
    }

    return false;
}

void
Cache::updateOutboundMegolmSession(const std::string &room_id,
                                   const GroupSessionData &data_,
                                   mtx::crypto::OutboundGroupSessionPtr &ptr)
{
    using namespace mtx::crypto;

    if (!outboundMegolmSessionExists(room_id))
        return;

    GroupSessionData data = data_;
    data.message_index    = olm_outbound_group_session_message_index(ptr.get());
    MegolmSessionIndex index;
    index.room_id    = room_id;
    index.session_id = mtx::crypto::session_id(ptr.get());

    // Save the updated pickled data for the session.
    nlohmann::json j;
    j["session"] = pickle<OutboundSessionObject>(ptr.get(), pickle_secret_);

    auto txn = lmdb::txn::begin(env_);
    outboundMegolmSessionDb_.put(txn, room_id, j.dump());
    megolmSessionDataDb_.put(txn, nlohmann::json(index).dump(), nlohmann::json(data).dump());
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
        outboundMegolmSessionDb_.del(txn, room_id);
        // don't delete session data, so that we can still share the session.
        txn.commit();
    }
}

void
Cache::saveOutboundMegolmSession(const std::string &room_id,
                                 const GroupSessionData &data_,
                                 mtx::crypto::OutboundGroupSessionPtr &session)
{
    using namespace mtx::crypto;
    const auto pickled = pickle<OutboundSessionObject>(session.get(), pickle_secret_);

    GroupSessionData data = data_;
    data.message_index    = olm_outbound_group_session_message_index(session.get());
    MegolmSessionIndex index;
    index.room_id    = room_id;
    index.session_id = mtx::crypto::session_id(session.get());

    nlohmann::json j;
    j["session"] = pickled;

    auto txn = lmdb::txn::begin(env_);
    outboundMegolmSessionDb_.put(txn, room_id, j.dump());
    megolmSessionDataDb_.put(txn, nlohmann::json(index).dump(), nlohmann::json(data).dump());
    txn.commit();
}

bool
Cache::outboundMegolmSessionExists(const std::string &room_id) noexcept
{
    try {
        auto txn = ro_txn(env_);
        std::string_view value;
        return outboundMegolmSessionDb_.get(txn, room_id, value);
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

        auto txn = ro_txn(env_);
        std::string_view value;
        outboundMegolmSessionDb_.get(txn, room_id, value);
        auto obj = nlohmann::json::parse(value);

        OutboundGroupSessionDataRef ref{};
        ref.session =
          unpickle<OutboundSessionObject>(obj.at("session").get<std::string>(), pickle_secret_);

        MegolmSessionIndex index;
        index.room_id    = room_id;
        index.session_id = mtx::crypto::session_id(ref.session.get());

        if (megolmSessionDataDb_.get(txn, nlohmann::json(index).dump(), value)) {
            ref.data = nlohmann::json::parse(value).get<GroupSessionData>();
        }

        return ref;
    } catch (std::exception &e) {
        nhlog::db()->error("Failed to retrieve outbound Megolm Session: {}", e.what());
        return {};
    }
}

std::optional<GroupSessionData>
Cache::getMegolmSessionData(const MegolmSessionIndex &index)
{
    try {
        using namespace mtx::crypto;

        auto txn = ro_txn(env_);

        std::string_view value;
        if (megolmSessionDataDb_.get(txn, nlohmann::json(index).dump(), value)) {
            return nlohmann::json::parse(value).get<GroupSessionData>();
        }

        return std::nullopt;
    } catch (std::exception &e) {
        nhlog::db()->error("Failed to retrieve Megolm Session Data: {}", e.what());
        return std::nullopt;
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

    const auto pickled    = pickle<SessionObject>(session.get(), pickle_secret_);
    const auto session_id = mtx::crypto::session_id(session.get());

    StoredOlmSession stored_session;
    stored_session.pickled_session = pickled;
    stored_session.last_message_ts = timestamp;

    db.put(txn, session_id, nlohmann::json(stored_session).dump());

    txn.commit();
}

std::optional<mtx::crypto::OlmSessionPtr>
Cache::getOlmSession(const std::string &curve25519, const std::string &session_id)
{
    using namespace mtx::crypto;

    auto txn = lmdb::txn::begin(env_);
    auto db  = getOlmSessionsDb(txn, curve25519);

    std::string_view pickled;
    bool found = db.get(txn, session_id, pickled);

    txn.commit();

    if (found) {
        auto data = nlohmann::json::parse(pickled).get<StoredOlmSession>();
        return unpickle<SessionObject>(data.pickled_session, pickle_secret_);
    }

    return std::nullopt;
}

std::optional<mtx::crypto::OlmSessionPtr>
Cache::getLatestOlmSession(const std::string &curve25519)
{
    using namespace mtx::crypto;

    auto txn = lmdb::txn::begin(env_);
    auto db  = getOlmSessionsDb(txn, curve25519);

    std::string_view session_id, pickled_session;

    std::optional<StoredOlmSession> currentNewest;

    auto cursor = lmdb::cursor::open(txn, db);
    while (cursor.get(session_id, pickled_session, MDB_NEXT)) {
        auto data = nlohmann::json::parse(pickled_session).get<StoredOlmSession>();
        if (!currentNewest || currentNewest->last_message_ts < data.last_message_ts)
            currentNewest = data;
    }
    cursor.close();

    txn.commit();

    return currentNewest ? std::optional(unpickle<SessionObject>(currentNewest->pickled_session,
                                                                 pickle_secret_))
                         : std::nullopt;
}

std::vector<std::string>
Cache::getOlmSessions(const std::string &curve25519)
{
    using namespace mtx::crypto;

    auto txn = lmdb::txn::begin(env_);
    auto db  = getOlmSessionsDb(txn, curve25519);

    std::string_view session_id, unused;
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
    syncStateDb_.put(txn, OLM_ACCOUNT_KEY, data);
    txn.commit();
}

std::string
Cache::restoreOlmAccount()
{
    auto txn = ro_txn(env_);

    std::string_view pickled;
    syncStateDb_.get(txn, OLM_ACCOUNT_KEY, pickled);

    return std::string(pickled.data(), pickled.size());
}

void
Cache::saveBackupVersion(const OnlineBackupVersion &data)
{
    auto txn = lmdb::txn::begin(env_);
    syncStateDb_.put(txn, CURRENT_ONLINE_BACKUP_VERSION, nlohmann::json(data).dump());
    txn.commit();
}

void
Cache::deleteBackupVersion()
{
    auto txn = lmdb::txn::begin(env_);
    syncStateDb_.del(txn, CURRENT_ONLINE_BACKUP_VERSION);
    txn.commit();
}

std::optional<OnlineBackupVersion>
Cache::backupVersion()
{
    try {
        auto txn = ro_txn(env_);
        std::string_view v;
        syncStateDb_.get(txn, CURRENT_ONLINE_BACKUP_VERSION, v);

        return nlohmann::json::parse(v).get<OnlineBackupVersion>();
    } catch (...) {
        return std::nullopt;
    }
}

void
Cache::removeInvite(lmdb::txn &txn, const std::string &room_id)
{
    invitesDb_.del(txn, room_id);
    getInviteStatesDb(txn, room_id).drop(txn, true);
    getInviteMembersDb(txn, room_id).drop(txn, true);
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
    roomsDb_.del(txn, roomid);
    getStatesDb(txn, roomid).drop(txn, true);
    getAccountDataDb(txn, roomid).drop(txn, true);
    getMembersDb(txn, roomid).drop(txn, true);
}

void
Cache::removeRoom(const std::string &roomid)
{
    auto txn = lmdb::txn::begin(env_, nullptr, 0);
    roomsDb_.del(txn, roomid);
    txn.commit();
}

void
Cache::setNextBatchToken(lmdb::txn &txn, const std::string &token)
{
    syncStateDb_.put(txn, NEXT_BATCH_KEY, token);
}

bool
Cache::isInitialized()
{
    if (!env_.handle())
        return false;

    auto txn = ro_txn(env_);
    std::string_view token;

    bool res = syncStateDb_.get(txn, NEXT_BATCH_KEY, token);

    return res;
}

std::string
Cache::nextBatchToken()
{
    if (!env_.handle())
        throw lmdb::error("Env already closed", MDB_INVALID);

    auto txn = ro_txn(env_);
    std::string_view token;

    bool result = syncStateDb_.get(txn, NEXT_BATCH_KEY, token);

    if (result)
        return std::string(token.data(), token.size());
    else
        return "";
}

void
Cache::deleteData()
{
    if (this->databaseReady_) {
        this->databaseReady_ = false;
        // TODO: We need to remove the env_ while not accepting new requests.
        lmdb::dbi_close(env_, syncStateDb_);
        lmdb::dbi_close(env_, roomsDb_);
        lmdb::dbi_close(env_, invitesDb_);
        lmdb::dbi_close(env_, readReceiptsDb_);
        lmdb::dbi_close(env_, notificationsDb_);

        lmdb::dbi_close(env_, devicesDb_);
        lmdb::dbi_close(env_, deviceKeysDb_);

        lmdb::dbi_close(env_, inboundMegolmSessionDb_);
        lmdb::dbi_close(env_, outboundMegolmSessionDb_);
        lmdb::dbi_close(env_, megolmSessionDataDb_);

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
        deleteSecret("pickle_secret", true);
    }
}

//! migrates db to the current format
bool
Cache::runMigrations()
{
    std::string stored_version;
    {
        auto txn = ro_txn(env_);

        std::string_view current_version;
        bool res = syncStateDb_.get(txn, CACHE_FORMAT_VERSION_KEY, current_version);

        if (!res)
            return false;

        stored_version = std::string(current_version);
    }

    std::vector<std::pair<std::string, std::function<bool()>>> migrations{
      {"2020.05.01",
       [this]() {
           try {
               auto txn              = lmdb::txn::begin(env_, nullptr);
               auto pending_receipts = lmdb::dbi::open(txn, "pending_receipts", MDB_CREATE);
               lmdb::dbi_drop(txn, pending_receipts, true);
               txn.commit();
           } catch (const lmdb::error &) {
               nhlog::db()->critical("Failed to delete pending_receipts database in migration!");
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
                       auto messagesDb =
                         lmdb::dbi::open(txn, std::string(room_id + "/messages").c_str());

                       // keep some old messages and batch token
                       {
                           auto roomsCursor = lmdb::cursor::open(txn, messagesDb);
                           std::string_view ts, stored_message;
                           bool start = true;
                           mtx::responses::Timeline oldMessages;
                           while (
                             roomsCursor.get(ts, stored_message, start ? MDB_FIRST : MDB_NEXT)) {
                               start = false;

                               auto j = nlohmann::json::parse(
                                 std::string_view(stored_message.data(), stored_message.size()));

                               if (oldMessages.prev_batch.empty())
                                   oldMessages.prev_batch = j["token"].get<std::string>();
                               else if (j["token"].get<std::string>() != oldMessages.prev_batch)
                                   break;

                               mtx::events::collections::TimelineEvent te;
                               from_json(j["event"], te);
                               oldMessages.events.push_back(te.data);
                           }
                           // messages were stored in reverse order, so we
                           // need to reverse them
                           std::reverse(oldMessages.events.begin(), oldMessages.events.end());
                           // save messages using the new method
                           auto eventsDb = getEventsDb(txn, room_id);
                           saveTimelineMessages(txn, eventsDb, room_id, oldMessages);
                       }

                       // delete old messages db
                       lmdb::dbi_drop(txn, messagesDb, true);
                   } catch (std::exception &e) {
                       nhlog::db()->error(
                         "While migrating messages from {}, ignoring error {}", room_id, e.what());
                   }
               }
               txn.commit();
           } catch (const lmdb::error &) {
               nhlog::db()->critical("Failed to delete messages database in migration!");
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

               std::string_view dbName, ignored;
               auto olmDbCursor = lmdb::cursor::open(txn, mainDb);
               while (olmDbCursor.get(dbName, ignored, MDB_NEXT)) {
                   // skip every db but olm session dbs
                   nhlog::db()->debug("Db {}", dbName);
                   if (dbName.find("olm_sessions/") != 0)
                       continue;

                   nhlog::db()->debug("Migrating {}", dbName);

                   auto olmDb = lmdb::dbi::open(txn, std::string(dbName).c_str());

                   std::string_view session_id, session_value;

                   std::vector<std::pair<std::string, StoredOlmSession>> sessions;

                   auto cursor = lmdb::cursor::open(txn, olmDb);
                   while (cursor.get(session_id, session_value, MDB_NEXT)) {
                       nhlog::db()->debug(
                         "session_id {}, session_value {}", session_id, session_value);
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

                   auto newDbName = std::string(dbName);
                   newDbName.erase(0, sizeof("olm_sessions") - 1);
                   newDbName = "olm_sessions.v2" + newDbName;

                   auto newDb = lmdb::dbi::open(txn, newDbName.c_str(), MDB_CREATE);

                   for (const auto &[key, value] : sessions) {
                       // nhlog::db()->debug("{}\n{}", key, nlohmann::json(value).dump());
                       newDb.put(txn, key, nlohmann::json(value).dump());
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
      {"2021.08.22",
       [this]() {
           try {
               auto txn      = lmdb::txn::begin(env_, nullptr);
               auto try_drop = [&txn](const std::string &dbName) {
                   try {
                       auto db = lmdb::dbi::open(txn, dbName.c_str());
                       db.drop(txn, true);
                   } catch (std::exception &e) {
                       nhlog::db()->warn("Failed to drop '{}': {}", dbName, e.what());
                   }
               };

               auto room_ids = getRoomIds(txn);

               for (const auto &room : room_ids) {
                   try_drop(room + "/state");
                   try_drop(room + "/state_by_key");
                   try_drop(room + "/account_data");
                   try_drop(room + "/members");
                   try_drop(room + "/mentions");
                   try_drop(room + "/events");
                   try_drop(room + "/event_order");
                   try_drop(room + "/event2order");
                   try_drop(room + "/msg2order");
                   try_drop(room + "/order2msg");
                   try_drop(room + "/pending");
                   try_drop(room + "/related");
               }

               // clear db, don't delete
               roomsDb_.drop(txn, false);
               setNextBatchToken(txn, "");

               txn.commit();
           } catch (const lmdb::error &) {
               nhlog::db()->critical("Failed to clear cache!");
               return false;
           }

           nhlog::db()->info("Successfully cleared the cache. Will do a clean sync after startup.");
           return true;
       }},
      {"2021.08.31",
       [this]() {
           storeSecret("pickle_secret", "secret", true);
           return true;
       }},
      {"2022.04.08",
       [this]() {
           try {
               auto txn = lmdb::txn::begin(env_, nullptr);
               auto inboundMegolmSessionDb =
                 lmdb::dbi::open(txn, INBOUND_MEGOLM_SESSIONS_DB, MDB_CREATE);
               auto outboundMegolmSessionDb =
                 lmdb::dbi::open(txn, OUTBOUND_MEGOLM_SESSIONS_DB, MDB_CREATE);
               auto megolmSessionDataDb = lmdb::dbi::open(txn, MEGOLM_SESSIONS_DATA_DB, MDB_CREATE);
               try {
                   outboundMegolmSessionDb.drop(txn, false);
               } catch (std::exception &e) {
                   nhlog::db()->warn("Failed to drop outbound sessions: {}", e.what());
               }

               std::string_view key, value;
               auto cursor = lmdb::cursor::open(txn, inboundMegolmSessionDb);
               std::map<std::string, std::string> inboundSessions;
               std::map<std::string, std::string> megolmSessionData;
               while (cursor.get(key, value, MDB_NEXT)) {
                   auto indexVal = nlohmann::json::parse(key);
                   if (!indexVal.contains("sender_key") || !indexVal.at("sender_key").is_string())
                       continue;
                   auto sender_key = indexVal["sender_key"].get<std::string>();
                   indexVal.erase("sender_key");

                   std::string_view dataVal;
                   bool res = megolmSessionDataDb.get(txn, key, dataVal);
                   if (res) {
                       auto data                          = nlohmann::json::parse(dataVal);
                       data["sender_key"]                 = sender_key;
                       inboundSessions[indexVal.dump()]   = std::string(value);
                       megolmSessionData[indexVal.dump()] = data.dump();
                   }
               }
               cursor.close();
               inboundMegolmSessionDb.drop(txn, false);
               megolmSessionDataDb.drop(txn, false);

               for (const auto &[k, v] : inboundSessions) {
                   inboundMegolmSessionDb.put(txn, k, v);
               }
               for (const auto &[k, v] : megolmSessionData) {
                   megolmSessionDataDb.put(txn, k, v);
               }
               txn.commit();
               return true;
           } catch (std::exception &e) {
               nhlog::db()->warn(
                 "Failed to migrate stored megolm session to have no sender key: {}", e.what());
               return false;
           }
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
    auto txn = ro_txn(env_);

    std::string_view current_version;
    bool res = syncStateDb_.get(txn, CACHE_FORMAT_VERSION_KEY, current_version);

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

    syncStateDb_.put(txn, CACHE_FORMAT_VERSION_KEY, CURRENT_CACHE_FORMAT_VERSION);

    txn.commit();
}

CachedReceipts
Cache::readReceipts(const QString &event_id, const QString &room_id)
{
    CachedReceipts receipts;

    ReadReceiptKey receipt_key{event_id.toStdString(), room_id.toStdString()};
    nlohmann::json json_key = receipt_key;

    try {
        auto txn = ro_txn(env_);
        auto key = json_key.dump();

        std::string_view value;

        bool res = readReceiptsDb_.get(txn, key, value);

        if (res) {
            auto json_response =
              nlohmann::json::parse(std::string_view(value.data(), value.size()));
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

            std::string_view prev_value;

            bool exists = readReceiptsDb_.get(txn, key, prev_value);

            std::map<std::string, uint64_t> saved_receipts;

            // If an entry for the event id already exists, we would
            // merge the existing receipts with the new ones.
            if (exists) {
                auto json_value =
                  nlohmann::json::parse(std::string_view(prev_value.data(), prev_value.size()));

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

            readReceiptsDb_.put(txn, key, merged_receipts);

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
    std::string last_event_id_, fullyReadEventId_;
    {
        auto txn = ro_txn(env_);

        // Get last event id on the room.
        const auto last_event_id = getLastEventId(txn, room_id);
        const auto localUser     = utils::localUser().toStdString();

        std::string fullyReadEventId;
        if (auto ev = getAccountData(txn, mtx::events::EventType::FullyRead, room_id)) {
            if (auto fr =
                  std::get_if<mtx::events::AccountDataEvent<mtx::events::account_data::FullyRead>>(
                    &ev.value())) {
                fullyReadEventId = fr->content.event_id;
            }
        }

        if (last_event_id.empty() || fullyReadEventId.empty())
            return true;

        if (last_event_id == fullyReadEventId)
            return false;

        last_event_id_    = std::string(last_event_id);
        fullyReadEventId_ = std::string(fullyReadEventId);
    }

    // Retrieve all read receipts for that event.
    return getEventIndex(room_id, last_event_id_) > getEventIndex(room_id, fullyReadEventId_);
}

void
Cache::updateState(const std::string &room, const mtx::responses::StateEvents &state)
{
    auto txn         = lmdb::txn::begin(env_);
    auto statesdb    = getStatesDb(txn, room);
    auto stateskeydb = getStatesKeyDb(txn, room);
    auto membersdb   = getMembersDb(txn, room);
    auto eventsDb    = getEventsDb(txn, room);

    saveStateEvents(txn, statesdb, stateskeydb, membersdb, eventsDb, room, state.events);

    RoomInfo updatedInfo;

    {
        std::string_view data;
        if (roomsDb_.get(txn, room, data)) {
            try {
                updatedInfo =
                  nlohmann::json::parse(std::string_view(data.data(), data.size())).get<RoomInfo>();
            } catch (const nlohmann::json::exception &e) {
                nhlog::db()->warn("failed to parse room info: room_id ({}), {}: {}",
                                  room,
                                  std::string(data.data(), data.size()),
                                  e.what());
            }
        }
    }

    updatedInfo.name       = getRoomName(txn, statesdb, membersdb).toStdString();
    updatedInfo.topic      = getRoomTopic(txn, statesdb).toStdString();
    updatedInfo.avatar_url = getRoomAvatarUrl(txn, statesdb, membersdb).toStdString();
    updatedInfo.version    = getRoomVersion(txn, statesdb).toStdString();
    updatedInfo.is_space   = getRoomIsSpace(txn, statesdb);

    roomsDb_.put(txn, room, nlohmann::json(updatedInfo).dump());
    updateSpaces(txn, {room}, {room});
    txn.commit();
}

namespace {
template<typename T>
auto
isMessage(const mtx::events::RoomEvent<T> &e)
  -> std::enable_if_t<std::is_same<decltype(e.content.msgtype), std::string>::value, bool>
{
    return true;
}

template<typename T>
auto
isMessage(const mtx::events::Event<T> &)
{
    return false;
}

template<typename T>
auto
isMessage(const mtx::events::EncryptedEvent<T> &)
{
    return true;
}

auto
isMessage(const mtx::events::RoomEvent<mtx::events::voip::CallInvite> &)
{
    return true;
}

auto
isMessage(const mtx::events::RoomEvent<mtx::events::voip::CallAnswer> &)
{
    return true;
}
auto
isMessage(const mtx::events::RoomEvent<mtx::events::voip::CallHangUp> &)
{
    return true;
}
}

void
Cache::saveState(const mtx::responses::Sync &res)
{
    using namespace mtx::events;
    auto local_user_id = this->localUserId_.toStdString();

    auto currentBatchToken = res.next_batch;

    auto txn = lmdb::txn::begin(env_);

    setNextBatchToken(txn, res.next_batch);

    if (!res.account_data.events.empty()) {
        auto accountDataDb = getAccountDataDb(txn, "");
        for (const auto &ev : res.account_data.events)
            std::visit(
              [&txn, &accountDataDb](const auto &event) {
                  if constexpr (std::is_same_v<
                                  std::remove_cv_t<std::remove_reference_t<decltype(event)>>,
                                  AccountDataEvent<
                                    mtx::events::account_data::nheko_extensions::HiddenEvents>>) {
                      if (!event.content.hidden_event_types) {
                          accountDataDb.del(txn, "im.nheko.hidden_events");
                          return;
                      }
                  }

                  auto j = nlohmann::json(event);
                  accountDataDb.put(txn, j["type"].get<std::string>(), j.dump());
              },
              ev);
    }

    auto userKeyCacheDb = getUserKeysDb(txn);

    std::set<std::string> spaces_with_updates;
    std::set<std::string> rooms_with_space_updates;

    // Save joined rooms
    for (const auto &room : res.rooms.join) {
        auto statesdb    = getStatesDb(txn, room.first);
        auto stateskeydb = getStatesKeyDb(txn, room.first);
        auto membersdb   = getMembersDb(txn, room.first);
        auto eventsDb    = getEventsDb(txn, room.first);

        saveStateEvents(
          txn, statesdb, stateskeydb, membersdb, eventsDb, room.first, room.second.state.events);
        saveStateEvents(
          txn, statesdb, stateskeydb, membersdb, eventsDb, room.first, room.second.timeline.events);

        saveTimelineMessages(txn, eventsDb, room.first, room.second.timeline);

        RoomInfo updatedInfo;
        {
            // retrieve the old tags and modification ts
            std::string_view data;
            if (roomsDb_.get(txn, room.first, data)) {
                try {
                    RoomInfo tmp = nlohmann::json::parse(std::string_view(data.data(), data.size()))
                                     .get<RoomInfo>();
                    updatedInfo.tags = std::move(tmp.tags);

                    updatedInfo.approximate_last_modification_ts =
                      tmp.approximate_last_modification_ts;
                } catch (const nlohmann::json::exception &e) {
                    nhlog::db()->warn("failed to parse room info: room_id ({}), {}: {}",
                                      room.first,
                                      std::string(data.data(), data.size()),
                                      e.what());
                }
            }
        }

        updatedInfo.name       = getRoomName(txn, statesdb, membersdb).toStdString();
        updatedInfo.topic      = getRoomTopic(txn, statesdb).toStdString();
        updatedInfo.avatar_url = getRoomAvatarUrl(txn, statesdb, membersdb).toStdString();
        updatedInfo.version    = getRoomVersion(txn, statesdb).toStdString();
        updatedInfo.is_space   = getRoomIsSpace(txn, statesdb);

        updatedInfo.notification_count = room.second.unread_notifications.notification_count;
        updatedInfo.highlight_count    = room.second.unread_notifications.highlight_count;

        if (updatedInfo.is_space) {
            bool space_updates = false;
            for (const auto &e : room.second.state.events)
                if (std::holds_alternative<StateEvent<state::space::Child>>(e) ||
                    std::holds_alternative<StateEvent<state::PowerLevels>>(e))
                    space_updates = true;
            for (const auto &e : room.second.timeline.events)
                if (std::holds_alternative<StateEvent<state::space::Child>>(e) ||
                    std::holds_alternative<StateEvent<state::PowerLevels>>(e))
                    space_updates = true;

            if (space_updates)
                spaces_with_updates.insert(room.first);
        }

        {
            bool room_has_space_update = false;
            for (const auto &e : room.second.state.events) {
                if (auto se = std::get_if<StateEvent<state::space::Parent>>(&e)) {
                    spaces_with_updates.insert(se->state_key);
                    room_has_space_update = true;
                }
            }
            for (const auto &e : room.second.timeline.events) {
                if (auto se = std::get_if<StateEvent<state::space::Parent>>(&e)) {
                    spaces_with_updates.insert(se->state_key);
                    room_has_space_update = true;
                }
            }

            if (room_has_space_update)
                rooms_with_space_updates.insert(room.first);
        }

        // Process the account_data associated with this room
        if (!room.second.account_data.events.empty()) {
            auto accountDataDb = getAccountDataDb(txn, room.first);

            for (const auto &evt : room.second.account_data.events) {
                std::visit(
                  [&txn, &accountDataDb](const auto &event) {
                      if constexpr (std::is_same_v<
                                      std::remove_cv_t<std::remove_reference_t<decltype(event)>>,
                                      AccountDataEvent<mtx::events::account_data::nheko_extensions::
                                                         HiddenEvents>>) {
                          if (!event.content.hidden_event_types) {
                              accountDataDb.del(txn, "im.nheko.hidden_events");
                              return;
                          }
                      }
                      auto j = nlohmann::json(event);
                      accountDataDb.put(txn, j["type"].get<std::string>(), j.dump());
                  },
                  evt);

                // for tag events
                if (std::holds_alternative<AccountDataEvent<account_data::Tags>>(evt)) {
                    auto tags_evt = std::get<AccountDataEvent<account_data::Tags>>(evt);

                    updatedInfo.tags.clear();
                    for (const auto &tag : tags_evt.content.tags) {
                        updatedInfo.tags.push_back(tag.first);
                    }
                }
                if (auto fr = std::get_if<
                      mtx::events::AccountDataEvent<mtx::events::account_data::FullyRead>>(&evt)) {
                    nhlog::db()->debug("Fully read: {}", fr->content.event_id);
                    emit removeNotification(QString::fromStdString(room.first),
                                            QString::fromStdString(fr->content.event_id));
                }
            }
        }

        for (const auto &e : room.second.timeline.events) {
            if (!std::visit([](const auto &e) -> bool { return isMessage(e); }, e))
                continue;
            updatedInfo.approximate_last_modification_ts =
              std::visit([](const auto &e) -> uint64_t { return e.origin_server_ts; }, e);
        }

        roomsDb_.put(txn, room.first, nlohmann::json(updatedInfo).dump());

        for (const auto &e : room.second.ephemeral.events) {
            if (auto receiptsEv =
                  std::get_if<mtx::events::EphemeralEvent<mtx::events::ephemeral::Receipt>>(&e)) {
                Receipts receipts;

                for (const auto &[event_id, userReceipts] : receiptsEv->content.receipts) {
                    if (auto r = userReceipts.find(mtx::events::ephemeral::Receipt::Read);
                        r != userReceipts.end()) {
                        for (const auto &[user_id, receipt] : r->second.users) {
                            receipts[event_id][user_id] = receipt.ts;
                        }
                    }
                    if (userReceipts.count(mtx::events::ephemeral::Receipt::ReadPrivate)) {
                        const auto &users =
                          userReceipts.at(mtx::events::ephemeral::Receipt::ReadPrivate).users;
                        if (auto ts = users.find(local_user_id);
                            ts != users.end() && ts->second.ts != 0)
                            receipts[event_id][local_user_id] = ts->second.ts;
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

    removeLeftRooms(txn, res.rooms.leave);

    updateSpaces(txn, spaces_with_updates, std::move(rooms_with_space_updates));

    txn.commit();

    std::map<QString, bool> readStatus;

    for (const auto &room : res.rooms.join) {
        for (const auto &e : room.second.ephemeral.events) {
            if (auto receiptsEv =
                  std::get_if<mtx::events::EphemeralEvent<mtx::events::ephemeral::Receipt>>(&e)) {
                std::vector<QString> receipts;

                for (const auto &[event_id, userReceipts] : receiptsEv->content.receipts) {
                    if (auto r = userReceipts.find(mtx::events::ephemeral::Receipt::Read);
                        r != userReceipts.end()) {
                        for (const auto &[user_id, receipt] : r->second.users) {
                            (void)receipt;

                            if (user_id != local_user_id) {
                                receipts.push_back(QString::fromStdString(event_id));
                                break;
                            }
                        }
                    }
                }
                if (!receipts.empty())
                    emit newReadReceipts(QString::fromStdString(room.first), receipts);
            }
        }
        readStatus.emplace(QString::fromStdString(room.first), calculateRoomReadStatus(room.first));
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
        updatedInfo.name       = getInviteRoomName(txn, statesdb, membersdb).toStdString();
        updatedInfo.topic      = getInviteRoomTopic(txn, statesdb).toStdString();
        updatedInfo.avatar_url = getInviteRoomAvatarUrl(txn, statesdb, membersdb).toStdString();
        updatedInfo.is_space   = getInviteRoomIsSpace(txn, statesdb);
        updatedInfo.is_invite  = true;

        invitesDb_.put(txn, room.first, nlohmann::json(updatedInfo).dump());
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
            auto display_name =
              msg->content.display_name.empty() ? msg->state_key : msg->content.display_name;

            MemberInfo tmp{display_name, msg->content.avatar_url, msg->content.is_direct};

            membersdb.put(txn, msg->state_key, nlohmann::json(tmp).dump());
        } else {
            std::visit(
              [&txn, &statesdb](auto msg) {
                  auto j   = nlohmann::json(msg);
                  bool res = statesdb.put(txn, j["type"].get<std::string>(), j.dump());

                  if (!res)
                      nhlog::db()->warn("couldn't save data: {}", nlohmann::json(msg).dump());
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
        presenceDb_.put(txn, update.sender, nlohmann::json(update.content).dump());
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

RoomInfo
Cache::singleRoomInfo(const std::string &room_id)
{
    auto txn = ro_txn(env_);

    try {
        auto statesdb = getStatesDb(txn, room_id);

        std::string_view data;

        // Check if the room is joined.
        if (roomsDb_.get(txn, room_id, data)) {
            try {
                RoomInfo tmp     = nlohmann::json::parse(data).get<RoomInfo>();
                tmp.member_count = getMembersDb(txn, room_id).size(txn);
                tmp.join_rule    = getRoomJoinRule(txn, statesdb);
                tmp.guest_access = getRoomGuestAccess(txn, statesdb);

                return tmp;
            } catch (const nlohmann::json::exception &e) {
                nhlog::db()->warn("failed to parse room info: room_id ({}), {}: {}",
                                  room_id,
                                  std::string(data.data(), data.size()),
                                  e.what());
            }
        }
    } catch (const lmdb::error &e) {
        nhlog::db()->warn("failed to read room info from db: room_id ({}), {}", room_id, e.what());
    }

    return RoomInfo();
}
void
Cache::updateLastMessageTimestamp(const std::string &room_id, uint64_t ts)
{
    auto txn = lmdb::txn::begin(env_);

    try {
        auto statesdb = getStatesDb(txn, room_id);

        std::string_view data;

        // Check if the room is joined.
        if (roomsDb_.get(txn, room_id, data)) {
            try {
                RoomInfo tmp                         = nlohmann::json::parse(data).get<RoomInfo>();
                tmp.approximate_last_modification_ts = ts;
                roomsDb_.put(txn, room_id, nlohmann::json(tmp).dump());
                txn.commit();
                return;
            } catch (const nlohmann::json::exception &e) {
                nhlog::db()->warn("failed to parse room info: room_id ({}), {}: {}",
                                  room_id,
                                  std::string(data.data(), data.size()),
                                  e.what());
            }
        }
    } catch (const lmdb::error &e) {
        nhlog::db()->warn("failed to read room info from db: room_id ({}), {}", room_id, e.what());
    }
}

std::map<QString, RoomInfo>
Cache::getRoomInfo(const std::vector<std::string> &rooms)
{
    std::map<QString, RoomInfo> room_info;

    // TODO This should be read only.
    auto txn = lmdb::txn::begin(env_);

    for (const auto &room : rooms) {
        std::string_view data;
        auto statesdb = getStatesDb(txn, room);

        // Check if the room is joined.
        if (roomsDb_.get(txn, room, data)) {
            try {
                RoomInfo tmp     = nlohmann::json::parse(data).get<RoomInfo>();
                tmp.member_count = getMembersDb(txn, room).size(txn);
                tmp.join_rule    = getRoomJoinRule(txn, statesdb);
                tmp.guest_access = getRoomGuestAccess(txn, statesdb);

                room_info.emplace(QString::fromStdString(room), std::move(tmp));
            } catch (const nlohmann::json::exception &e) {
                nhlog::db()->warn("failed to parse room info: room_id ({}), {}: {}",
                                  room,
                                  std::string(data.data(), data.size()),
                                  e.what());
            }
        } else {
            // Check if the room is an invite.
            if (invitesDb_.get(txn, room, data)) {
                try {
                    RoomInfo tmp = nlohmann::json::parse(std::string_view(data)).get<RoomInfo>();
                    tmp.member_count = getInviteMembersDb(txn, room).size(txn);

                    room_info.emplace(QString::fromStdString(room), std::move(tmp));
                } catch (const nlohmann::json::exception &e) {
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
    auto txn = ro_txn(env_);

    std::vector<QString> rooms;
    std::string_view room_id, unused;

    auto roomsCursor = lmdb::cursor::open(txn, roomsDb_);
    while (roomsCursor.get(room_id, unused, MDB_NEXT))
        rooms.push_back(QString::fromStdString(std::string(room_id)));

    roomsCursor.close();

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
    std::string_view indexVal, val;
    if (!cursor.get(indexVal, val, MDB_FIRST)) {
        return "";
    }

    auto j = nlohmann::json::parse(val);

    return j.value("prev_batch", "");
}

Cache::Messages
Cache::getTimelineMessages(lmdb::txn &txn, const std::string &room_id, uint64_t index, bool forward)
{
    // TODO(nico): Limit the messages returned by this maybe?
    auto orderDb  = getOrderToMessageDb(txn, room_id);
    auto eventsDb = getEventsDb(txn, room_id);

    Messages messages{};

    std::string_view indexVal, event_id;

    auto cursor = lmdb::cursor::open(txn, orderDb);
    if (index == std::numeric_limits<uint64_t>::max()) {
        if (!cursor.get(indexVal, event_id, forward ? MDB_FIRST : MDB_LAST)) {
            messages.end_of_cache = true;
            return messages;
        }
    } else {
        if (!cursor.get(indexVal, event_id, MDB_SET)) {
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
        std::string_view event;
        bool success = eventsDb.get(txn, event_id, event);
        if (!success)
            continue;

        mtx::events::collections::TimelineEvent te;
        try {
            from_json(nlohmann::json::parse(event), te);
        } catch (std::exception &e) {
            nhlog::db()->error("Failed to parse message from cache {}", e.what());
            continue;
        }

        messages.timeline.events.push_back(std::move(te.data));
    }
    cursor.close();

    // std::reverse(timeline.events.begin(), timeline.events.end());
    messages.next_index   = lmdb::from_sv<uint64_t>(indexVal);
    messages.end_of_cache = !ret;

    return messages;
}

std::optional<mtx::events::collections::TimelineEvent>
Cache::getEvent(const std::string &room_id, const std::string &event_id)
{
    auto txn      = ro_txn(env_);
    auto eventsDb = getEventsDb(txn, room_id);

    std::string_view event{};
    bool success = eventsDb.get(txn, event_id, event);
    if (!success)
        return {};

    mtx::events::collections::TimelineEvent te;
    try {
        from_json(nlohmann::json::parse(event), te);
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
    eventsDb.put(txn, event_id, event_json.dump());
    txn.commit();
}

void
Cache::replaceEvent(const std::string &room_id,
                    const std::string &event_id,
                    const mtx::events::collections::TimelineEvent &event)
{
    auto txn         = lmdb::txn::begin(env_);
    auto eventsDb    = getEventsDb(txn, room_id);
    auto relationsDb = getRelationsDb(txn, room_id);
    auto event_json  = mtx::accessors::serialize_event(event.data).dump();

    {
        eventsDb.del(txn, event_id);
        eventsDb.put(txn, event_id, event_json);
        for (const auto &relation : mtx::accessors::relations(event.data).relations) {
            relationsDb.put(txn, relation.event_id, event_id);
        }
    }

    txn.commit();
}

std::vector<std::string>
Cache::relatedEvents(const std::string &room_id, const std::string &event_id)
{
    auto txn         = ro_txn(env_);
    auto relationsDb = getRelationsDb(txn, room_id);

    std::vector<std::string> related_ids;

    auto related_cursor         = lmdb::cursor::open(txn, relationsDb);
    std::string_view related_to = event_id, related_event;
    bool first                  = true;

    try {
        if (!related_cursor.get(related_to, related_event, MDB_SET))
            return {};

        while (
          related_cursor.get(related_to, related_event, first ? MDB_FIRST_DUP : MDB_NEXT_DUP)) {
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

size_t
Cache::memberCount(const std::string &room_id)
{
    auto txn = ro_txn(env_);
    return getMembersDb(txn, room_id).size(txn);
}

QMap<QString, RoomInfo>
Cache::roomInfo(bool withInvites)
{
    QMap<QString, RoomInfo> result;

    auto txn = ro_txn(env_);

    std::string_view room_id;
    std::string_view room_data;

    // Gather info about the joined rooms.
    auto roomsCursor = lmdb::cursor::open(txn, roomsDb_);
    while (roomsCursor.get(room_id, room_data, MDB_NEXT)) {
        RoomInfo tmp     = nlohmann::json::parse(std::move(room_data)).get<RoomInfo>();
        tmp.member_count = getMembersDb(txn, std::string(room_id)).size(txn);
        result.insert(QString::fromStdString(std::string(room_id)), std::move(tmp));
    }
    roomsCursor.close();

    if (withInvites) {
        // Gather info about the invites.
        auto invitesCursor = lmdb::cursor::open(txn, invitesDb_);
        while (invitesCursor.get(room_id, room_data, MDB_NEXT)) {
            RoomInfo tmp     = nlohmann::json::parse(room_data).get<RoomInfo>();
            tmp.member_count = getInviteMembersDb(txn, std::string(room_id)).size(txn);
            result.insert(QString::fromStdString(std::string(room_id)), std::move(tmp));
        }
        invitesCursor.close();
    }

    return result;
}

std::string
Cache::getLastEventId(lmdb::txn &txn, const std::string &room_id)
{
    lmdb::dbi orderDb;
    try {
        orderDb = getOrderToMessageDb(txn, room_id);
    } catch (lmdb::runtime_error &e) {
        nhlog::db()->error(
          "Can't open db for room '{}', probably doesn't exist yet. ({})", room_id, e.what());
        return {};
    }

    std::string_view indexVal, val;

    auto cursor = lmdb::cursor::open(txn, orderDb);
    if (!cursor.get(indexVal, val, MDB_LAST)) {
        return {};
    }

    return std::string(val.data(), val.size());
}

std::optional<Cache::TimelineRange>
Cache::getTimelineRange(const std::string &room_id)
{
    auto txn = ro_txn(env_);
    lmdb::dbi orderDb;
    try {
        orderDb = getOrderToMessageDb(txn, room_id);
    } catch (lmdb::runtime_error &e) {
        nhlog::db()->error(
          "Can't open db for room '{}', probably doesn't exist yet. ({})", room_id, e.what());
        return {};
    }

    std::string_view indexVal, val;

    auto cursor = lmdb::cursor::open(txn, orderDb);
    if (!cursor.get(indexVal, val, MDB_LAST)) {
        return {};
    }

    TimelineRange range{};
    range.last = lmdb::from_sv<uint64_t>(indexVal);

    if (!cursor.get(indexVal, val, MDB_FIRST)) {
        return {};
    }
    range.first = lmdb::from_sv<uint64_t>(indexVal);

    return range;
}
std::optional<uint64_t>
Cache::getTimelineIndex(const std::string &room_id, std::string_view event_id)
{
    if (event_id.empty() || room_id.empty())
        return {};

    auto txn = ro_txn(env_);

    lmdb::dbi orderDb;
    try {
        orderDb = getMessageToOrderDb(txn, room_id);
    } catch (lmdb::runtime_error &e) {
        nhlog::db()->error(
          "Can't open db for room '{}', probably doesn't exist yet. ({})", room_id, e.what());
        return {};
    }

    std::string_view indexVal{event_id.data(), event_id.size()}, val;

    bool success = orderDb.get(txn, indexVal, val);
    if (!success) {
        return {};
    }

    return lmdb::from_sv<uint64_t>(val);
}

std::optional<uint64_t>
Cache::getEventIndex(const std::string &room_id, std::string_view event_id)
{
    if (room_id.empty() || event_id.empty())
        return {};

    auto txn = ro_txn(env_);

    lmdb::dbi orderDb;
    try {
        orderDb = getEventToOrderDb(txn, room_id);
    } catch (lmdb::runtime_error &e) {
        nhlog::db()->error(
          "Can't open db for room '{}', probably doesn't exist yet. ({})", room_id, e.what());
        return {};
    }

    std::string_view val;

    bool success = orderDb.get(txn, event_id, val);
    if (!success) {
        return {};
    }

    return lmdb::from_sv<uint64_t>(val);
}

std::optional<std::pair<uint64_t, std::string>>
Cache::lastInvisibleEventAfter(const std::string &room_id, std::string_view event_id)
{
    if (room_id.empty() || event_id.empty())
        return {};

    auto txn = ro_txn(env_);

    lmdb::dbi orderDb;
    lmdb::dbi eventOrderDb;
    lmdb::dbi timelineDb;
    try {
        orderDb      = getEventToOrderDb(txn, room_id);
        eventOrderDb = getEventOrderDb(txn, room_id);
        timelineDb   = getMessageToOrderDb(txn, room_id);
    } catch (lmdb::runtime_error &e) {
        nhlog::db()->error(
          "Can't open db for room '{}', probably doesn't exist yet. ({})", room_id, e.what());
        return {};
    }

    std::string_view indexVal;

    bool success = orderDb.get(txn, event_id, indexVal);
    if (!success) {
        return {};
    }

    try {
        uint64_t prevIdx = lmdb::from_sv<uint64_t>(indexVal);
        std::string prevId{event_id};

        auto cursor = lmdb::cursor::open(txn, eventOrderDb);
        cursor.get(indexVal, MDB_SET);
        while (cursor.get(indexVal, event_id, MDB_NEXT)) {
            std::string evId = nlohmann::json::parse(event_id)["event_id"].get<std::string>();
            std::string_view temp;
            if (timelineDb.get(txn, evId, temp)) {
                return std::pair{prevIdx, std::string(prevId)};
            } else {
                prevIdx = lmdb::from_sv<uint64_t>(indexVal);
                prevId  = std::move(evId);
            }
        }

        return std::pair{prevIdx, std::string(prevId)};
    } catch (lmdb::runtime_error &e) {
        nhlog::db()->error("Failed to get last invisible event after {}", event_id, e.what());
        return {};
    }
}

std::optional<uint64_t>
Cache::getArrivalIndex(const std::string &room_id, std::string_view event_id)
{
    auto txn = ro_txn(env_);

    lmdb::dbi orderDb;
    try {
        orderDb = getEventToOrderDb(txn, room_id);
    } catch (lmdb::runtime_error &e) {
        nhlog::db()->error(
          "Can't open db for room '{}', probably doesn't exist yet. ({})", room_id, e.what());
        return {};
    }

    std::string_view val;

    bool success = orderDb.get(txn, event_id, val);
    if (!success) {
        return {};
    }

    return lmdb::from_sv<uint64_t>(val);
}

std::optional<std::string>
Cache::getTimelineEventId(const std::string &room_id, uint64_t index)
{
    auto txn = ro_txn(env_);
    lmdb::dbi orderDb;
    try {
        orderDb = getOrderToMessageDb(txn, room_id);
    } catch (lmdb::runtime_error &e) {
        nhlog::db()->error(
          "Can't open db for room '{}', probably doesn't exist yet. ({})", room_id, e.what());
        return {};
    }

    std::string_view val;

    bool success = orderDb.get(txn, lmdb::to_sv(index), val);
    if (!success) {
        return {};
    }

    return std::string(val);
}

QHash<QString, RoomInfo>
Cache::invites()
{
    QHash<QString, RoomInfo> result;

    auto txn    = ro_txn(env_);
    auto cursor = lmdb::cursor::open(txn, invitesDb_);

    std::string_view room_id, room_data;

    while (cursor.get(room_id, room_data, MDB_NEXT)) {
        try {
            RoomInfo tmp     = nlohmann::json::parse(room_data).get<RoomInfo>();
            tmp.member_count = getInviteMembersDb(txn, std::string(room_id)).size(txn);
            result.insert(QString::fromStdString(std::string(room_id)), std::move(tmp));
        } catch (const nlohmann::json::exception &e) {
            nhlog::db()->warn("failed to parse room info for invite: "
                              "room_id ({}), {}: {}",
                              room_id,
                              std::string(room_data),
                              e.what());
        }
    }

    cursor.close();

    return result;
}

std::optional<RoomInfo>
Cache::invite(std::string_view roomid)
{
    std::optional<RoomInfo> result;

    auto txn = ro_txn(env_);

    std::string_view room_data;

    if (invitesDb_.get(txn, roomid, room_data)) {
        try {
            RoomInfo tmp     = nlohmann::json::parse(room_data).get<RoomInfo>();
            tmp.member_count = getInviteMembersDb(txn, std::string(roomid)).size(txn);
            result           = std::move(tmp);
        } catch (const nlohmann::json::exception &e) {
            nhlog::db()->warn("failed to parse room info for invite: "
                              "room_id ({}), {}: {}",
                              roomid,
                              std::string(room_data),
                              e.what());
        }
    }

    return result;
}

QString
Cache::getRoomAvatarUrl(lmdb::txn &txn, lmdb::dbi &statesdb, lmdb::dbi &membersdb)
{
    using namespace mtx::events;
    using namespace mtx::events::state;

    std::string_view event;
    bool res = statesdb.get(txn, to_string(mtx::events::EventType::RoomAvatar), event);

    if (res) {
        try {
            StateEvent<Avatar> msg =
              nlohmann::json::parse(std::string_view(event.data(), event.size()))
                .get<StateEvent<Avatar>>();

            if (!msg.content.url.empty())
                return QString::fromStdString(msg.content.url);
        } catch (const nlohmann::json::exception &e) {
            nhlog::db()->warn("failed to parse m.room.avatar event: {}", e.what());
        }
    }

    // We don't use an avatar for group chats.
    if (membersdb.size(txn) > 2)
        return QString();

    auto cursor = lmdb::cursor::open(txn, membersdb);
    std::string_view user_id;
    std::string_view member_data;
    std::string fallback_url;

    // Resolve avatar for 1-1 chats.
    while (cursor.get(user_id, member_data, MDB_NEXT)) {
        try {
            MemberInfo m = nlohmann::json::parse(member_data).get<MemberInfo>();
            if (user_id == localUserId_.toStdString()) {
                fallback_url = m.avatar_url;
                continue;
            }

            cursor.close();
            return QString::fromStdString(m.avatar_url);
        } catch (const nlohmann::json::exception &e) {
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

    std::string_view event;
    bool res = statesdb.get(txn, to_string(mtx::events::EventType::RoomName), event);

    if (res) {
        try {
            StateEvent<Name> msg =
              nlohmann::json::parse(std::string_view(event.data(), event.size()))
                .get<StateEvent<Name>>();

            if (!msg.content.name.empty())
                return QString::fromStdString(msg.content.name);
        } catch (const nlohmann::json::exception &e) {
            nhlog::db()->warn("failed to parse m.room.name event: {}", e.what());
        }
    }

    res = statesdb.get(txn, to_string(mtx::events::EventType::RoomCanonicalAlias), event);

    if (res) {
        try {
            StateEvent<CanonicalAlias> msg =
              nlohmann::json::parse(std::string_view(event.data(), event.size()))
                .get<StateEvent<CanonicalAlias>>();

            if (!msg.content.alias.empty())
                return QString::fromStdString(msg.content.alias);
        } catch (const nlohmann::json::exception &e) {
            nhlog::db()->warn("failed to parse m.room.canonical_alias event: {}", e.what());
        }
    }

    auto cursor      = lmdb::cursor::open(txn, membersdb);
    const auto total = membersdb.size(txn);

    std::size_t ii = 0;
    std::string_view user_id;
    std::string_view member_data;
    std::map<std::string, MemberInfo> members;

    while (cursor.get(user_id, member_data, MDB_NEXT) && ii < 3) {
        try {
            members.emplace(user_id, nlohmann::json::parse(member_data).get<MemberInfo>());
        } catch (const nlohmann::json::exception &e) {
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
        return tr("%1 and %n other(s)", "", (int)total - 1).arg(first_member);

    return tr("Empty Room");
}

mtx::events::state::JoinRule
Cache::getRoomJoinRule(lmdb::txn &txn, lmdb::dbi &statesdb)
{
    using namespace mtx::events;
    using namespace mtx::events::state;

    std::string_view event;
    bool res = statesdb.get(txn, to_string(mtx::events::EventType::RoomJoinRules), event);

    if (res) {
        try {
            StateEvent<state::JoinRules> msg =
              nlohmann::json::parse(event).get<StateEvent<state::JoinRules>>();
            return msg.content.join_rule;
        } catch (const nlohmann::json::exception &e) {
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

    std::string_view event;
    bool res = statesdb.get(txn, to_string(mtx::events::EventType::RoomGuestAccess), event);

    if (res) {
        try {
            StateEvent<GuestAccess> msg =
              nlohmann::json::parse(event).get<StateEvent<GuestAccess>>();
            return msg.content.guest_access == AccessState::CanJoin;
        } catch (const nlohmann::json::exception &e) {
            nhlog::db()->warn("failed to parse m.room.guest_access event: {}", e.what());
        }
    }
    return false;
}

QString
Cache::getRoomTopic(lmdb::txn &txn, lmdb::dbi &statesdb)
{
    using namespace mtx::events;
    using namespace mtx::events::state;

    std::string_view event;
    bool res = statesdb.get(txn, to_string(mtx::events::EventType::RoomTopic), event);

    if (res) {
        try {
            StateEvent<Topic> msg = nlohmann::json::parse(event).get<StateEvent<Topic>>();

            if (!msg.content.topic.empty())
                return QString::fromStdString(msg.content.topic);
        } catch (const nlohmann::json::exception &e) {
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

    std::string_view event;
    bool res = statesdb.get(txn, to_string(mtx::events::EventType::RoomCreate), event);

    if (res) {
        try {
            StateEvent<Create> msg = nlohmann::json::parse(event).get<StateEvent<Create>>();

            if (!msg.content.room_version.empty())
                return QString::fromStdString(msg.content.room_version);
        } catch (const nlohmann::json::exception &e) {
            nhlog::db()->warn("failed to parse m.room.create event: {}", e.what());
        }
    }

    nhlog::db()->warn("m.room.create event is missing room version, assuming version \"1\"");
    return QStringLiteral("1");
}

bool
Cache::getRoomIsSpace(lmdb::txn &txn, lmdb::dbi &statesdb)
{
    using namespace mtx::events;
    using namespace mtx::events::state;

    std::string_view event;
    bool res = statesdb.get(txn, to_string(mtx::events::EventType::RoomCreate), event);

    if (res) {
        try {
            StateEvent<Create> msg = nlohmann::json::parse(event).get<StateEvent<Create>>();

            return msg.content.type == mtx::events::state::room_type::space;
        } catch (const nlohmann::json::exception &e) {
            nhlog::db()->warn("failed to parse m.room.create event: {}", e.what());
        }
    }

    nhlog::db()->warn("m.room.create event is missing room version, assuming version \"1\"");
    return false;
}

QString
Cache::getInviteRoomName(lmdb::txn &txn, lmdb::dbi &statesdb, lmdb::dbi &membersdb)
{
    using namespace mtx::events;
    using namespace mtx::events::state;

    std::string_view event;
    bool res = statesdb.get(txn, to_string(mtx::events::EventType::RoomName), event);

    if (res) {
        try {
            StrippedEvent<state::Name> msg =
              nlohmann::json::parse(event).get<StrippedEvent<state::Name>>();
            return QString::fromStdString(msg.content.name);
        } catch (const nlohmann::json::exception &e) {
            nhlog::db()->warn("failed to parse m.room.name event: {}", e.what());
        }
    }

    auto cursor = lmdb::cursor::open(txn, membersdb);
    std::string_view user_id, member_data;

    while (cursor.get(user_id, member_data, MDB_NEXT)) {
        if (user_id == localUserId_.toStdString())
            continue;

        try {
            MemberInfo tmp = nlohmann::json::parse(member_data).get<MemberInfo>();
            cursor.close();

            return QString::fromStdString(tmp.name);
        } catch (const nlohmann::json::exception &e) {
            nhlog::db()->warn("failed to parse member info: {}", e.what());
        }
    }

    cursor.close();

    return tr("Empty Room");
}

QString
Cache::getInviteRoomAvatarUrl(lmdb::txn &txn, lmdb::dbi &statesdb, lmdb::dbi &membersdb)
{
    using namespace mtx::events;
    using namespace mtx::events::state;

    std::string_view event;
    bool res = statesdb.get(txn, to_string(mtx::events::EventType::RoomAvatar), event);

    if (res) {
        try {
            StrippedEvent<state::Avatar> msg =
              nlohmann::json::parse(event).get<StrippedEvent<state::Avatar>>();
            return QString::fromStdString(msg.content.url);
        } catch (const nlohmann::json::exception &e) {
            nhlog::db()->warn("failed to parse m.room.avatar event: {}", e.what());
        }
    }

    auto cursor = lmdb::cursor::open(txn, membersdb);
    std::string_view user_id, member_data;

    while (cursor.get(user_id, member_data, MDB_NEXT)) {
        if (user_id == localUserId_.toStdString())
            continue;

        try {
            MemberInfo tmp = nlohmann::json::parse(member_data).get<MemberInfo>();
            cursor.close();

            return QString::fromStdString(tmp.avatar_url);
        } catch (const nlohmann::json::exception &e) {
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

    std::string_view event;
    bool res = db.get(txn, to_string(mtx::events::EventType::RoomTopic), event);

    if (res) {
        try {
            StrippedEvent<Topic> msg = nlohmann::json::parse(event).get<StrippedEvent<Topic>>();
            return QString::fromStdString(msg.content.topic);
        } catch (const nlohmann::json::exception &e) {
            nhlog::db()->warn("failed to parse m.room.topic event: {}", e.what());
        }
    }

    return QString();
}

bool
Cache::getInviteRoomIsSpace(lmdb::txn &txn, lmdb::dbi &db)
{
    using namespace mtx::events;
    using namespace mtx::events::state;

    std::string_view event;
    bool res = db.get(txn, to_string(mtx::events::EventType::RoomCreate), event);

    if (res) {
        try {
            StrippedEvent<Create> msg = nlohmann::json::parse(event).get<StrippedEvent<Create>>();
            return msg.content.type == mtx::events::state::room_type::space;
        } catch (const nlohmann::json::exception &e) {
            nhlog::db()->warn("failed to parse m.room.topic event: {}", e.what());
        }
    }

    return false;
}

std::vector<std::string>
Cache::joinedRooms()
{
    auto txn         = ro_txn(env_);
    auto roomsCursor = lmdb::cursor::open(txn, roomsDb_);

    std::string_view id, data;
    std::vector<std::string> room_ids;

    // Gather the room ids for the joined rooms.
    while (roomsCursor.get(id, data, MDB_NEXT))
        room_ids.emplace_back(id);

    roomsCursor.close();

    return room_ids;
}

std::optional<MemberInfo>
Cache::getMember(const std::string &room_id, const std::string &user_id)
{
    if (user_id.empty() || !env_.handle())
        return std::nullopt;

    try {
        auto txn = ro_txn(env_);

        auto membersdb = getMembersDb(txn, room_id);

        std::string_view info;
        if (membersdb.get(txn, user_id, info)) {
            MemberInfo m = nlohmann::json::parse(info).get<MemberInfo>();
            return m;
        }
    } catch (std::exception &e) {
        nhlog::db()->warn(
          "Failed to read member ({}) in room ({}): {}", user_id, room_id, e.what());
    }
    return std::nullopt;
}

std::vector<RoomMember>
Cache::getMembers(const std::string &room_id, std::size_t startIndex, std::size_t len)
{
    try {
        auto txn    = ro_txn(env_);
        auto db     = getMembersDb(txn, room_id);
        auto cursor = lmdb::cursor::open(txn, db);

        std::size_t currentIndex = 0;

        const auto endIndex = std::min(startIndex + len, db.size(txn));

        std::vector<RoomMember> members;

        std::string_view user_id, user_data;
        while (cursor.get(user_id, user_data, MDB_NEXT)) {
            if (currentIndex < startIndex) {
                currentIndex += 1;
                continue;
            }

            if (currentIndex >= endIndex)
                break;

            try {
                MemberInfo tmp = nlohmann::json::parse(user_data).get<MemberInfo>();
                members.emplace_back(RoomMember{QString::fromStdString(std::string(user_id)),
                                                QString::fromStdString(tmp.name)});
            } catch (const nlohmann::json::exception &e) {
                nhlog::db()->warn("{}", e.what());
            }

            currentIndex += 1;
        }

        cursor.close();

        return members;
    } catch (const lmdb::error &e) {
        nhlog::db()->error("Failed to retrieve members from db in room {}: {}", room_id, e.what());
        return {};
    }
}

std::vector<RoomMember>
Cache::getMembersFromInvite(const std::string &room_id, std::size_t startIndex, std::size_t len)
{
    try {
        auto txn = ro_txn(env_);
        std::vector<RoomMember> members;

        auto db     = getInviteMembersDb(txn, room_id);
        auto cursor = lmdb::cursor::open(txn, db);

        std::size_t currentIndex = 0;

        const auto endIndex = std::min(startIndex + len, db.size(txn));

        std::string_view user_id, user_data;
        while (cursor.get(user_id, user_data, MDB_NEXT)) {
            if (currentIndex < startIndex) {
                currentIndex += 1;
                continue;
            }

            if (currentIndex >= endIndex)
                break;

            try {
                MemberInfo tmp = nlohmann::json::parse(user_data).get<MemberInfo>();
                members.emplace_back(RoomMember{QString::fromStdString(std::string(user_id)),
                                                QString::fromStdString(tmp.name),
                                                tmp.is_direct});
            } catch (const nlohmann::json::exception &e) {
                nhlog::db()->warn("{}", e.what());
            }

            currentIndex += 1;
        }

        cursor.close();

        return members;
    } catch (const lmdb::error &e) {
        nhlog::db()->error("Failed to retrieve members from db in room {}: {}", room_id, e.what());
        return {};
    }
}

bool
Cache::isRoomMember(const std::string &user_id, const std::string &room_id)
{
    try {
        auto txn = ro_txn(env_);
        auto db  = getMembersDb(txn, room_id);

        std::string_view value;
        bool res = db.get(txn, user_id, value);

        return res;
    } catch (std::exception &e) {
        nhlog::db()->warn(
          "Failed to read member membership ({}) in room ({}): {}", user_id, room_id, e.what());
    }
    return false;
}

void
Cache::savePendingMessage(const std::string &room_id,
                          const mtx::events::collections::TimelineEvent &message)
{
    auto txn      = lmdb::txn::begin(env_);
    auto eventsDb = getEventsDb(txn, room_id);

    mtx::responses::Timeline timeline;
    timeline.events.push_back(message.data);
    saveTimelineMessages(txn, eventsDb, room_id, timeline);

    auto pending = getPendingMessagesDb(txn, room_id);

    int64_t now = QDateTime::currentMSecsSinceEpoch();
    pending.put(txn, lmdb::to_sv(now), mtx::accessors::event_id(message.data));

    txn.commit();
}
std::vector<std::string>
Cache::pendingEvents(const std::string &room_id)
{
    auto txn     = ro_txn(env_);
    auto pending = getPendingMessagesDb(txn, room_id);

    std::vector<std::string> related_ids;

    try {
        {
            auto pendingCursor = lmdb::cursor::open(txn, pending);
            std::string_view tsIgnored, pendingTxn;
            while (pendingCursor.get(tsIgnored, pendingTxn, MDB_NEXT)) {
                related_ids.emplace_back(pendingTxn.data(), pendingTxn.size());
            }
        }
    } catch (const lmdb::error &e) {
        nhlog::db()->error("pending events error: {}", e.what());
    }

    return related_ids;
}

std::optional<mtx::events::collections::TimelineEvent>
Cache::firstPendingMessage(const std::string &room_id)
{
    auto txn     = lmdb::txn::begin(env_);
    auto pending = getPendingMessagesDb(txn, room_id);

    {
        auto pendingCursor = lmdb::cursor::open(txn, pending);
        std::string_view tsIgnored, pendingTxn;
        while (pendingCursor.get(tsIgnored, pendingTxn, MDB_NEXT)) {
            auto eventsDb = getEventsDb(txn, room_id);
            std::string_view event;
            if (!eventsDb.get(txn, pendingTxn, event)) {
                pending.del(txn, tsIgnored, pendingTxn);
                continue;
            }

            try {
                mtx::events::collections::TimelineEvent te;
                from_json(nlohmann::json::parse(event), te);

                pendingCursor.close();
                txn.commit();
                return te;
            } catch (std::exception &e) {
                nhlog::db()->error("Failed to parse message from cache {}", e.what());
                pending.del(txn, tsIgnored, pendingTxn);
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
        std::string_view tsIgnored, pendingTxn;
        while (pendingCursor.get(tsIgnored, pendingTxn, MDB_NEXT)) {
            if (std::string_view(pendingTxn.data(), pendingTxn.size()) == txn_id)
                lmdb::cursor_del(pendingCursor);
        }
    }

    txn.commit();
}

void
Cache::saveTimelineMessages(lmdb::txn &txn,
                            lmdb::dbi &eventsDb,
                            const std::string &room_id,
                            const mtx::responses::Timeline &res)
{
    if (res.events.empty())
        return;

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

    std::string_view indexVal, val;
    uint64_t index = std::numeric_limits<uint64_t>::max() / 2;
    auto cursor    = lmdb::cursor::open(txn, orderDb);
    if (cursor.get(indexVal, val, MDB_LAST)) {
        index = lmdb::from_sv<uint64_t>(indexVal);
    }

    uint64_t msgIndex = std::numeric_limits<uint64_t>::max() / 2;
    auto msgCursor    = lmdb::cursor::open(txn, order2msgDb);
    if (msgCursor.get(indexVal, val, MDB_LAST)) {
        msgIndex = lmdb::from_sv<uint64_t>(indexVal);
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

        std::string_view event_id = event_id_val;

        nlohmann::json orderEntry = nlohmann::json::object();
        orderEntry["event_id"]    = event_id_val;
        if (first && !res.prev_batch.empty())
            orderEntry["prev_batch"] = res.prev_batch;

        std::string_view txn_order;
        if (!txn_id.empty() && evToOrderDb.get(txn, txn_id, txn_order)) {
            eventsDb.put(txn, event_id, event.dump());
            eventsDb.del(txn, txn_id);

            std::string_view msg_txn_order;
            if (msg2orderDb.get(txn, txn_id, msg_txn_order)) {
                order2msgDb.put(txn, msg_txn_order, event_id);
                msg2orderDb.put(txn, event_id, msg_txn_order);
                msg2orderDb.del(txn, txn_id);
            }

            orderDb.put(txn, txn_order, orderEntry.dump());
            evToOrderDb.put(txn, event_id, txn_order);
            evToOrderDb.del(txn, txn_id);

            auto relations = mtx::accessors::relations(e);
            if (!relations.relations.empty()) {
                for (const auto &r : relations.relations) {
                    if (!r.event_id.empty()) {
                        relationsDb.del(txn, r.event_id, txn_id);
                        relationsDb.put(txn, r.event_id, event_id);
                    }
                }
            }

            auto pendingCursor = lmdb::cursor::open(txn, pending);
            std::string_view tsIgnored, pendingTxn;
            while (pendingCursor.get(tsIgnored, pendingTxn, MDB_NEXT)) {
                if (std::string_view(pendingTxn.data(), pendingTxn.size()) == txn_id)
                    lmdb::cursor_del(pendingCursor);
            }
        } else if (auto redaction =
                     std::get_if<mtx::events::RedactionEvent<mtx::events::msg::Redaction>>(&e)) {
            if (redaction->redacts.empty())
                continue;

            std::string_view oldEvent;
            bool success = eventsDb.get(txn, redaction->redacts, oldEvent);
            if (!success)
                continue;

            mtx::events::collections::TimelineEvent te;
            try {
                from_json(nlohmann::json::parse(std::string_view(oldEvent.data(), oldEvent.size())),
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
                nhlog::db()->error("Failed to parse message from cache {}", e.what());
                continue;
            }

            eventsDb.put(txn, redaction->redacts, event.dump());
            eventsDb.put(txn, redaction->event_id, nlohmann::json(*redaction).dump());
        } else {
            first = false;

            // This check protects against duplicates in the timeline. If the event_id
            // is already in the DB, we skip putting it (again) in ordered DBs, and only
            // update the event itself and its relations.
            std::string_view unused_read;
            if (!evToOrderDb.get(txn, event_id, unused_read)) {
                ++index;

                nhlog::db()->debug("saving '{}'", orderEntry.dump());

                cursor.put(lmdb::to_sv(index), orderEntry.dump(), MDB_APPEND);
                evToOrderDb.put(txn, event_id, lmdb::to_sv(index));

                // TODO(Nico): Allow blacklisting more event types in UI
                if (!isHiddenEvent(txn, e, room_id)) {
                    ++msgIndex;
                    msgCursor.put(lmdb::to_sv(msgIndex), event_id, MDB_APPEND);

                    msg2orderDb.put(txn, event_id, lmdb::to_sv(msgIndex));
                }
            } else {
                nhlog::db()->warn("duplicate event '{}'", orderEntry.dump());
            }
            eventsDb.put(txn, event_id, event.dump());

            auto relations = mtx::accessors::relations(e);
            if (!relations.relations.empty()) {
                for (const auto &r : relations.relations) {
                    if (!r.event_id.empty()) {
                        relationsDb.put(txn, r.event_id, event_id);
                    }
                }
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

    std::string_view indexVal, val;
    uint64_t index = std::numeric_limits<uint64_t>::max() / 2;
    {
        auto cursor = lmdb::cursor::open(txn, orderDb);
        if (cursor.get(indexVal, val, MDB_FIRST)) {
            index = lmdb::from_sv<uint64_t>(indexVal);
        }
    }

    uint64_t msgIndex = std::numeric_limits<uint64_t>::max() / 2;
    {
        auto msgCursor = lmdb::cursor::open(txn, order2msgDb);
        if (msgCursor.get(indexVal, val, MDB_FIRST)) {
            msgIndex = lmdb::from_sv<uint64_t>(indexVal);
        }
    }

    if (res.chunk.empty()) {
        if (orderDb.get(txn, lmdb::to_sv(index), val)) {
            auto orderEntry          = nlohmann::json::parse(val);
            orderEntry["prev_batch"] = res.end;
            orderDb.put(txn, lmdb::to_sv(index), orderEntry.dump());
            txn.commit();
        }
        return index;
    }

    std::string event_id_val;
    for (const auto &e : res.chunk) {
        if (std::holds_alternative<mtx::events::RedactionEvent<mtx::events::msg::Redaction>>(e))
            continue;

        auto event                = mtx::accessors::serialize_event(e);
        event_id_val              = event["event_id"].get<std::string>();
        std::string_view event_id = event_id_val;

        // This check protects against duplicates in the timeline. If the event_id is
        // already in the DB, we skip putting it (again) in ordered DBs, and only update the
        // event itself and its relations.
        std::string_view unused_read;
        if (!evToOrderDb.get(txn, event_id, unused_read)) {
            --index;

            nlohmann::json orderEntry = nlohmann::json::object();
            orderEntry["event_id"]    = event_id_val;

            orderDb.put(txn, lmdb::to_sv(index), orderEntry.dump());
            evToOrderDb.put(txn, event_id, lmdb::to_sv(index));

            // TODO(Nico): Allow blacklisting more event types in UI
            if (!isHiddenEvent(txn, e, room_id)) {
                --msgIndex;
                order2msgDb.put(txn, lmdb::to_sv(msgIndex), event_id);

                msg2orderDb.put(txn, event_id, lmdb::to_sv(msgIndex));
            }
        }
        eventsDb.put(txn, event_id, event.dump());

        auto relations = mtx::accessors::relations(e);
        if (!relations.relations.empty()) {
            for (const auto &r : relations.relations) {
                if (!r.event_id.empty()) {
                    relationsDb.put(txn, r.event_id, event_id);
                }
            }
        }
    }

    nlohmann::json orderEntry = nlohmann::json::object();
    orderEntry["event_id"]    = event_id_val;
    orderEntry["prev_batch"]  = res.end;
    orderDb.put(txn, lmdb::to_sv(index), orderEntry.dump());

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

    std::string_view indexVal, val;
    auto cursor = lmdb::cursor::open(txn, orderDb);

    bool start                   = true;
    bool passed_pagination_token = false;
    while (cursor.get(indexVal, val, start ? MDB_LAST : MDB_PREV)) {
        start = false;
        nlohmann::json obj;

        try {
            obj = nlohmann::json::parse(std::string_view(val.data(), val.size()));
        } catch (std::exception &) {
            // workaround bug in the initial db format, where we sometimes didn't store
            // json...
            obj = {{"event_id", std::string(val.data(), val.size())}};
        }

        if (passed_pagination_token) {
            if (obj.count("event_id") != 0) {
                std::string event_id = obj["event_id"].get<std::string>();

                if (!event_id.empty()) {
                    evToOrderDb.del(txn, event_id);
                    eventsDb.del(txn, event_id);
                    relationsDb.del(txn, event_id);

                    std::string_view order{};
                    bool exists = msg2orderDb.get(txn, event_id, order);
                    if (exists) {
                        order2msgDb.del(txn, order);
                        msg2orderDb.del(txn, event_id);
                    }
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

        std::string_view eventId;
        bool innerStart = true;
        bool found      = false;
        while (cursor.get(indexVal, eventId, innerStart ? MDB_LAST : MDB_PREV)) {
            innerStart = false;

            nlohmann::json obj;
            try {
                obj = nlohmann::json::parse(std::string_view(eventId.data(), eventId.size()));
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

    if (!start) {
        do {
            lmdb::cursor_del(msgCursor);
        } while (msgCursor.get(indexVal, val, MDB_PREV));
    }

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
    std::string_view event_id, msg;

    auto cursor = lmdb::cursor::open(txn, db);

    while (cursor.get(event_id, msg, MDB_NEXT)) {
        auto obj = nlohmann::json::parse(msg);

        if (obj.count("event") == 0)
            continue;

        mtx::responses::Notification notification;
        from_json(obj, notification);

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
        nlohmann::json val = notif;
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

        nlohmann::json obj = notif;

        db.put(txn, event_id, obj.dump());
    }
}

void
Cache::markSentNotification(const std::string &event_id)
{
    auto txn = lmdb::txn::begin(env_);
    notificationsDb_.put(txn, event_id, "");
    txn.commit();
}

void
Cache::removeReadNotification(const std::string &event_id)
{
    auto txn = lmdb::txn::begin(env_);

    notificationsDb_.del(txn, event_id);

    txn.commit();
}

bool
Cache::isNotificationSent(const std::string &event_id)
{
    auto txn = ro_txn(env_);

    std::string_view value;
    bool res = notificationsDb_.get(txn, event_id, value);

    return res;
}

std::vector<std::string>
Cache::getRoomIds(lmdb::txn &txn)
{
    auto cursor = lmdb::cursor::open(txn, roomsDb_);

    std::vector<std::string> rooms;

    std::string_view room_id, _unused;
    while (cursor.get(room_id, _unused, MDB_NEXT))
        rooms.emplace_back(room_id);

    cursor.close();

    return rooms;
}

void
Cache::deleteOldMessages()
{
    std::string_view indexVal, val;

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
            last = lmdb::from_sv<uint64_t>(indexVal);
        } else {
            continue;
        }
        if (cursor.get(indexVal, val, MDB_FIRST)) {
            first = lmdb::from_sv<uint64_t>(indexVal);
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
            auto obj = nlohmann::json::parse(std::string_view(val.data(), val.size()));

            if (obj.count("event_id") != 0) {
                std::string event_id = obj["event_id"].get<std::string>();
                evToOrderDb.del(txn, event_id);
                eventsDb.del(txn, event_id);

                relationsDb.del(txn, event_id);

                std::string_view order{};
                bool exists = m2o.get(txn, event_id, order);
                if (exists) {
                    o2m.del(txn, order);
                    m2o.del(txn, event_id);
                }
            }
            cursor.del();
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

void
Cache::updateSpaces(lmdb::txn &txn,
                    const std::set<std::string> &spaces_with_updates,
                    std::set<std::string> rooms_with_updates)
{
    if (spaces_with_updates.empty() && rooms_with_updates.empty())
        return;

    for (const auto &space : spaces_with_updates) {
        // delete old entries
        {
            auto cursor         = lmdb::cursor::open(txn, spacesChildrenDb_);
            bool first          = true;
            std::string_view sp = space, space_child = "";

            if (cursor.get(sp, space_child, MDB_SET)) {
                while (cursor.get(sp, space_child, first ? MDB_FIRST_DUP : MDB_NEXT_DUP)) {
                    first = false;
                    spacesParentsDb_.del(txn, space_child, space);
                }
            }
            cursor.close();
            spacesChildrenDb_.del(txn, space);
        }

        for (const auto &event :
             getStateEventsWithType<mtx::events::state::space::Child>(txn, space)) {
            if (event.content.via.has_value() && event.state_key.size() > 3 &&
                event.state_key.at(0) == '!') {
                spacesChildrenDb_.put(txn, space, event.state_key);
                spacesParentsDb_.put(txn, event.state_key, space);
            }
        }

        for (const auto &r : getRoomIds(txn)) {
            if (auto parent = getStateEvent<mtx::events::state::space::Parent>(txn, r, space)) {
                rooms_with_updates.insert(r);
            }
        }
    }

    const auto space_event_type = to_string(mtx::events::EventType::SpaceChild);

    for (const auto &room : rooms_with_updates) {
        for (const auto &event :
             getStateEventsWithType<mtx::events::state::space::Parent>(txn, room)) {
            if (event.content.via.has_value() && event.state_key.size() > 3 &&
                event.state_key.at(0) == '!') {
                const std::string &space = event.state_key;

                auto pls = getStateEvent<mtx::events::state::PowerLevels>(txn, space);

                if (!pls)
                    continue;

                if (pls->content.user_level(event.sender) >=
                    pls->content.state_level(space_event_type)) {
                    spacesChildrenDb_.put(txn, space, room);
                    spacesParentsDb_.put(txn, room, space);
                } else {
                    nhlog::db()->debug("Skipping {} in {} because of missing PL. {}: {} < {}",
                                       room,
                                       space,
                                       event.sender,
                                       pls->content.user_level(event.sender),
                                       pls->content.state_level(space_event_type));
                }
            }
        }
    }
}

QMap<QString, std::optional<RoomInfo>>
Cache::spaces()
{
    auto txn = ro_txn(env_);

    QMap<QString, std::optional<RoomInfo>> ret;
    {
        auto cursor = lmdb::cursor::open(txn, spacesChildrenDb_);
        bool first  = true;
        std::string_view space_id, space_child;
        while (cursor.get(space_id, space_child, first ? MDB_FIRST : MDB_NEXT)) {
            first = false;

            if (!space_child.empty()) {
                std::string_view room_data;
                if (roomsDb_.get(txn, space_id, room_data)) {
                    RoomInfo tmp = nlohmann::json::parse(std::move(room_data)).get<RoomInfo>();
                    ret.insert(QString::fromUtf8(space_id.data(), space_id.size()), tmp);
                } else {
                    ret.insert(QString::fromUtf8(space_id.data(), space_id.size()), std::nullopt);
                }
            }
        }
        cursor.close();
    }

    return ret;
}

std::vector<std::string>
Cache::getParentRoomIds(const std::string &room_id)
{
    auto txn = ro_txn(env_);

    std::vector<std::string> roomids;
    {
        auto cursor         = lmdb::cursor::open(txn, spacesParentsDb_);
        bool first          = true;
        std::string_view sp = room_id, space_parent;
        if (cursor.get(sp, space_parent, MDB_SET)) {
            while (cursor.get(sp, space_parent, first ? MDB_FIRST_DUP : MDB_NEXT_DUP)) {
                first = false;

                if (!space_parent.empty())
                    roomids.emplace_back(space_parent);
            }
        }
        cursor.close();
    }

    return roomids;
}

std::vector<std::string>
Cache::getChildRoomIds(const std::string &room_id)
{
    auto txn = ro_txn(env_);

    std::vector<std::string> roomids;
    {
        auto cursor         = lmdb::cursor::open(txn, spacesChildrenDb_);
        bool first          = true;
        std::string_view sp = room_id, space_child;
        if (cursor.get(sp, space_child, MDB_SET)) {
            while (cursor.get(sp, space_child, first ? MDB_FIRST_DUP : MDB_NEXT_DUP)) {
                first = false;

                if (!space_child.empty())
                    roomids.emplace_back(space_child);
            }
        }
        cursor.close();
    }

    return roomids;
}

std::vector<ImagePackInfo>
Cache::getImagePacks(const std::string &room_id, std::optional<bool> stickers)
{
    auto txn = ro_txn(env_);
    std::vector<ImagePackInfo> infos;

    auto addPack = [&infos, stickers](const mtx::events::msc2545::ImagePack &pack,
                                      const std::string &source_room,
                                      const std::string &state_key,
                                      bool from_space) {
        bool pack_is_sticker = pack.pack ? pack.pack->is_sticker() : true;
        bool pack_is_emoji   = pack.pack ? pack.pack->is_emoji() : true;
        bool pack_matches =
          !stickers.has_value() || (stickers.value() ? pack_is_sticker : pack_is_emoji);

        ImagePackInfo info;
        info.source_room = source_room;
        info.state_key   = state_key;
        info.pack.pack   = pack.pack;
        info.from_space  = from_space;

        for (const auto &img : pack.images) {
            if (stickers.has_value() &&
                (img.second.overrides_usage()
                   ? (stickers.value() ? !img.second.is_sticker() : !img.second.is_emoji())
                   : !pack_matches))
                continue;

            info.pack.images.insert(img);
        }

        if (!info.pack.images.empty())
            infos.push_back(std::move(info));
    };

    // packs from account data
    if (auto accountpack =
          getAccountData(txn, mtx::events::EventType::ImagePackInAccountData, "")) {
        auto tmp =
          std::get_if<mtx::events::EphemeralEvent<mtx::events::msc2545::ImagePack>>(&*accountpack);
        if (tmp)
            addPack(tmp->content, "", "", false);
    }

    // packs from rooms, that were enabled globally
    if (auto roomPacks = getAccountData(txn, mtx::events::EventType::ImagePackRooms, "")) {
        auto tmp = std::get_if<mtx::events::EphemeralEvent<mtx::events::msc2545::ImagePackRooms>>(
          &*roomPacks);
        if (tmp) {
            for (const auto &[room_id2, state_to_d] : tmp->content.rooms) {
                // don't add stickers from this room twice
                if (room_id2 == room_id)
                    continue;

                for (const auto &[state_id, d] : state_to_d) {
                    (void)d;
                    if (auto pack =
                          getStateEvent<mtx::events::msc2545::ImagePack>(txn, room_id2, state_id))
                        addPack(pack->content, room_id2, state_id, false);
                }
            }
        }
    }

    std::function<void(const std::string &room_id)> addRoomAndCanonicalParents;
    std::unordered_set<std::string> visitedRooms;
    addRoomAndCanonicalParents =
      [this, &addRoomAndCanonicalParents, &addPack, &visitedRooms, &txn, &room_id](
        const std::string &current_room) {
          if (visitedRooms.count(current_room))
              return;
          else
              visitedRooms.insert(current_room);

          if (auto pack = getStateEvent<mtx::events::msc2545::ImagePack>(txn, current_room)) {
              addPack(pack->content, current_room, "", current_room != room_id);
          }
          for (const auto &pack :
               getStateEventsWithType<mtx::events::msc2545::ImagePack>(txn, current_room)) {
              addPack(pack.content, current_room, pack.state_key, current_room != room_id);
          }

          for (const auto &parent :
               getStateEventsWithType<mtx::events::state::space::Parent>(txn, current_room)) {
              if (parent.content.canonical && parent.content.via && !parent.content.via->empty())
                  addRoomAndCanonicalParents(parent.state_key);
          }
      };

    // packs from current room and then iterate canonical space parents
    addRoomAndCanonicalParents(room_id);

    return infos;
}

std::optional<mtx::events::collections::RoomAccountDataEvents>
Cache::getAccountData(mtx::events::EventType type, const std::string &room_id)
{
    auto txn = ro_txn(env_);
    return getAccountData(txn, type, room_id);
}

std::optional<mtx::events::collections::RoomAccountDataEvents>
Cache::getAccountData(lmdb::txn &txn, mtx::events::EventType type, const std::string &room_id)
{
    try {
        auto db = getAccountDataDb(txn, room_id);

        std::string_view data;
        if (db.get(txn, to_string(type), data)) {
            mtx::responses::utils::RoomAccountDataEvents events;
            nlohmann::json j = nlohmann::json::array({
              nlohmann::json::parse(data),
            });
            mtx::responses::utils::parse_room_account_data_events(j, events);
            if (events.size() == 1)
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

    int64_t min_event_level = std::numeric_limits<int64_t>::max();
    int64_t user_level      = std::numeric_limits<int64_t>::min();

    std::string_view event;
    bool res = db.get(txn, to_string(EventType::RoomPowerLevels), event);

    if (res) {
        try {
            StateEvent<PowerLevels> msg =
              nlohmann::json::parse(std::string_view(event.data(), event.size()))
                .get<StateEvent<PowerLevels>>();

            user_level = msg.content.user_level(user_id);

            for (const auto &ty : eventTypes)
                min_event_level = std::min(min_event_level, msg.content.state_level(to_string(ty)));
        } catch (const nlohmann::json::exception &e) {
            nhlog::db()->warn("failed to parse m.room.power_levels event: {}", e.what());
        }
    }

    txn.commit();

    return user_level >= min_event_level;
}

std::vector<std::string>
Cache::roomMembers(const std::string &room_id)
{
    auto txn = ro_txn(env_);

    try {
        std::vector<std::string> members;
        std::string_view user_id, unused;

        auto db = getMembersDb(txn, room_id);

        auto cursor = lmdb::cursor::open(txn, db);
        while (cursor.get(user_id, unused, MDB_NEXT))
            members.emplace_back(user_id);
        cursor.close();

        return members;
    } catch (const lmdb::error &e) {
        nhlog::db()->error("Failed to retrieve members from db in room {}: {}", room_id, e.what());
        return {};
    }
}

crypto::Trust
Cache::roomVerificationStatus(const std::string &room_id)
{
    crypto::Trust trust = crypto::Verified;

    try {
        auto txn = lmdb::txn::begin(env_);

        auto db     = getMembersDb(txn, room_id);
        auto keysDb = getUserKeysDb(txn);
        std::vector<std::string> keysToRequest;

        std::string_view user_id, unused;
        auto cursor = lmdb::cursor::open(txn, db);
        while (cursor.get(user_id, unused, MDB_NEXT)) {
            auto verif = verificationStatus_(std::string(user_id), txn);
            if (verif.unverified_device_count) {
                trust = crypto::Unverified;
                if (verif.verified_devices.empty() && verif.no_keys) {
                    // we probably don't have the keys yet, so query them
                    keysToRequest.push_back(std::string(user_id));
                }
            } else if (verif.user_verified == crypto::TOFU && trust == crypto::Verified)
                trust = crypto::TOFU;
        }

        if (!keysToRequest.empty()) {
            std::string_view token;

            bool result = syncStateDb_.get(txn, NEXT_BATCH_KEY, token);

            if (!result)
                token = "";
            markUserKeysOutOfDate(txn, keysDb, keysToRequest, std::string(token));
        }

    } catch (std::exception &e) {
        nhlog::db()->error("Failed to calculate verification status for {}: {}", room_id, e.what());
        trust = crypto::Unverified;
    }

    return trust;
}

std::map<std::string, std::optional<UserKeyCache>>
Cache::getMembersWithKeys(const std::string &room_id, bool verified_only)
{
    std::string_view keys;

    try {
        auto txn = ro_txn(env_);
        std::map<std::string, std::optional<UserKeyCache>> members;

        auto db     = getMembersDb(txn, room_id);
        auto keysDb = getUserKeysDb(txn);

        std::string_view user_id, unused;
        auto cursor = lmdb::cursor::open(txn, db);
        while (cursor.get(user_id, unused, MDB_NEXT)) {
            auto res = keysDb.get(txn, user_id, keys);

            if (res) {
                auto k = nlohmann::json::parse(keys).get<UserKeyCache>();
                if (verified_only) {
                    auto verif = verificationStatus_(std::string(user_id), txn);

                    if (verif.user_verified == crypto::Trust::Verified ||
                        !verif.verified_devices.empty()) {
                        auto keyCopy = k;
                        keyCopy.device_keys.clear();

                        std::copy_if(
                          k.device_keys.begin(),
                          k.device_keys.end(),
                          std::inserter(keyCopy.device_keys, keyCopy.device_keys.end()),
                          [&verif](const auto &key) {
                              auto curve25519 = key.second.keys.find("curve25519:" + key.first);
                              if (curve25519 == key.second.keys.end())
                                  return false;
                              if (auto t = verif.verified_device_keys.find(curve25519->second);
                                  t == verif.verified_device_keys.end() ||
                                  t->second != crypto::Trust::Verified)
                                  return false;

                              return key.first == key.second.device_id &&
                                     std::find(verif.verified_devices.begin(),
                                               verif.verified_devices.end(),
                                               key.first) != verif.verified_devices.end();
                          });

                        if (!keyCopy.device_keys.empty())
                            members[std::string(user_id)] = std::move(keyCopy);
                    }
                } else {
                    members[std::string(user_id)] = std::move(k);
                }
            } else {
                if (!verified_only)
                    members[std::string(user_id)] = {};
            }
        }
        cursor.close();

        return members;
    } catch (std::exception &e) {
        nhlog::db()->debug("Error retrieving members: {}", e.what());
        return {};
    }
}

QString
Cache::displayName(const QString &room_id, const QString &user_id)
{
    return QString::fromStdString(displayName(room_id.toStdString(), user_id.toStdString()));
}

static bool
isDisplaynameSafe(const std::string &s)
{
    for (QChar c : QString::fromStdString(s).toStdU32String()) {
        if (c.isPrint() && !c.isSpace())
            return false;
    }

    return true;
}

std::string
Cache::displayName(const std::string &room_id, const std::string &user_id)
{
    if (auto info = getMember(room_id, user_id); info && !isDisplaynameSafe(info->name))
        return info->name;

    return user_id;
}

QString
Cache::avatarUrl(const QString &room_id, const QString &user_id)
{
    if (auto info = getMember(room_id.toStdString(), user_id.toStdString());
        info && !info->avatar_url.empty())
        return QString::fromStdString(info->avatar_url);

    return QString();
}

mtx::events::presence::Presence
Cache::presence(const std::string &user_id)
{
    mtx::events::presence::Presence presence_{};
    presence_.presence = mtx::presence::PresenceState::offline;

    if (user_id.empty())
        return presence_;

    std::string_view presenceVal;

    auto txn = ro_txn(env_);
    auto res = presenceDb_.get(txn, user_id, presenceVal);

    if (res) {
        presence_ = nlohmann::json::parse(std::string_view(presenceVal.data(), presenceVal.size()))
                      .get<mtx::events::presence::Presence>();
    }

    return presence_;
}

void
to_json(nlohmann::json &j, const UserKeyCache &info)
{
    j["device_keys"]        = info.device_keys;
    j["seen_device_keys"]   = info.seen_device_keys;
    j["seen_device_ids"]    = info.seen_device_ids;
    j["master_keys"]        = info.master_keys;
    j["master_key_changed"] = info.master_key_changed;
    j["user_signing_keys"]  = info.user_signing_keys;
    j["self_signing_keys"]  = info.self_signing_keys;
    j["updated_at"]         = info.updated_at;
    j["last_changed"]       = info.last_changed;
}

void
from_json(const nlohmann::json &j, UserKeyCache &info)
{
    info.device_keys = j.value("device_keys", std::map<std::string, mtx::crypto::DeviceKeys>{});
    info.seen_device_keys   = j.value("seen_device_keys", std::set<std::string>{});
    info.seen_device_ids    = j.value("seen_device_ids", std::set<std::string>{});
    info.master_keys        = j.value("master_keys", mtx::crypto::CrossSigningKeys{});
    info.master_key_changed = j.value("master_key_changed", false);
    info.user_signing_keys  = j.value("user_signing_keys", mtx::crypto::CrossSigningKeys{});
    info.self_signing_keys  = j.value("self_signing_keys", mtx::crypto::CrossSigningKeys{});
    info.updated_at         = j.value("updated_at", "");
    info.last_changed       = j.value("last_changed", "");
}

std::optional<UserKeyCache>
Cache::userKeys(const std::string &user_id)
{
    auto txn = ro_txn(env_);
    return userKeys_(user_id, txn);
}

std::optional<UserKeyCache>
Cache::userKeys_(const std::string &user_id, lmdb::txn &txn)
{
    std::string_view keys;

    try {
        auto db  = getUserKeysDb(txn);
        auto res = db.get(txn, user_id, keys);

        if (res) {
            return nlohmann::json::parse(keys).get<UserKeyCache>();
        } else {
            return std::nullopt;
        }
    } catch (std::exception &e) {
        nhlog::db()->error("Failed to retrieve user keys for {}: {}", user_id, e.what());
        return std::nullopt;
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

        auto updateToWrite = update;

        std::string_view oldKeys;
        auto res = db.get(txn, user, oldKeys);

        if (res) {
            updateToWrite     = nlohmann::json::parse(oldKeys).get<UserKeyCache>();
            auto last_changed = updateToWrite.last_changed;
            // skip if we are tracking this and expect it to be up to date with the last
            // sync token
            if (!last_changed.empty() && last_changed != sync_token) {
                nhlog::db()->debug("Not storing update for user {}, because "
                                   "last_changed {}, but we fetched update for {}",
                                   user,
                                   last_changed,
                                   sync_token);
                continue;
            }

            if (!updateToWrite.master_keys.keys.empty() &&
                update.master_keys.keys != updateToWrite.master_keys.keys) {
                nhlog::db()->debug("Master key of {} changed:\nold: {}\nnew: {}",
                                   user,
                                   updateToWrite.master_keys.keys.size(),
                                   update.master_keys.keys.size());
                updateToWrite.master_key_changed = true;
            }

            updateToWrite.master_keys       = update.master_keys;
            updateToWrite.self_signing_keys = update.self_signing_keys;
            updateToWrite.user_signing_keys = update.user_signing_keys;

            auto oldDeviceKeys = std::move(updateToWrite.device_keys);
            updateToWrite.device_keys.clear();

            // Don't insert keys, which we have seen once already
            for (const auto &[device_id, device_keys] : update.device_keys) {
                if (oldDeviceKeys.count(device_id) &&
                    oldDeviceKeys.at(device_id).keys == device_keys.keys) {
                    // this is safe, since the keys are the same
                    updateToWrite.device_keys[device_id] = device_keys;
                } else {
                    bool keyReused = false;
                    for (const auto &[key_id, key] : device_keys.keys) {
                        (void)key_id;
                        if (updateToWrite.seen_device_keys.count(key)) {
                            nhlog::crypto()->warn(
                              "Key '{}' reused by ({}: {})", key, user, device_id);
                            keyReused = true;
                            break;
                        }
                        if (updateToWrite.seen_device_ids.count(device_id)) {
                            nhlog::crypto()->warn("device_id '{}' reused by ({})", device_id, user);
                            keyReused = true;
                            break;
                        }
                    }

                    if (!keyReused && !oldDeviceKeys.count(device_id)) {
                        // ensure the key has a valid signature from itself
                        std::string device_signing_key = "ed25519:" + device_keys.device_id;
                        if (device_id != device_keys.device_id) {
                            nhlog::crypto()->warn("device {}:{} has a different device id "
                                                  "in the body: {}",
                                                  user,
                                                  device_id,
                                                  device_keys.device_id);
                            continue;
                        }
                        if (!device_keys.signatures.count(user) ||
                            !device_keys.signatures.at(user).count(device_signing_key)) {
                            nhlog::crypto()->warn("device {}:{} has no signature", user, device_id);
                            continue;
                        }
                        if (!device_keys.keys.count(device_signing_key) ||
                            !device_keys.keys.count("curve25519:" + device_id)) {
                            nhlog::crypto()->warn(
                              "Device key has no curve25519 or ed25519 key  {}:{}",
                              user,
                              device_id);
                            continue;
                        }

                        if (!mtx::crypto::ed25519_verify_signature(
                              device_keys.keys.at(device_signing_key),
                              nlohmann::json(device_keys),
                              device_keys.signatures.at(user).at(device_signing_key))) {
                            nhlog::crypto()->warn(
                              "device {}:{} has an invalid signature", user, device_id);
                            continue;
                        }

                        updateToWrite.device_keys[device_id] = device_keys;
                    }
                }

                for (const auto &[key_id, key] : device_keys.keys) {
                    (void)key_id;
                    updateToWrite.seen_device_keys.insert(key);
                }
                updateToWrite.seen_device_ids.insert(device_id);
            }
        }
        updateToWrite.updated_at = sync_token;
        db.put(txn, user, nlohmann::json(updateToWrite).dump());
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
Cache::markUserKeysOutOfDate(const std::vector<std::string> &user_ids)
{
    auto currentBatchToken = nextBatchToken();
    auto txn               = lmdb::txn::begin(env_);
    auto db                = getUserKeysDb(txn);
    markUserKeysOutOfDate(txn, db, user_ids, currentBatchToken);
    txn.commit();
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
        if (user.size() > 255) {
            nhlog::db()->debug("Skipping device key query for user with invalid mxid: {}", user);
            continue;
        }

        nhlog::db()->debug("Marking user keys out of date: {}", user);

        std::string_view oldKeys;

        UserKeyCache cacheEntry{};
        auto res = db.get(txn, user, oldKeys);
        if (res) {
            try {
                cacheEntry = nlohmann::json::parse(std::string_view(oldKeys.data(), oldKeys.size()))
                               .get<UserKeyCache>();
            } catch (std::exception &e) {
                nhlog::db()->error("Failed to parse {}: {}", oldKeys, e.what());
            }
        }
        cacheEntry.last_changed = sync_token;

        db.put(txn, user, nlohmann::json(cacheEntry).dump());

        query.device_keys[user] = {};
    }

    if (!query.device_keys.empty())
        http::client()->query_keys(
          query,
          [this, sync_token](const mtx::responses::QueryKeys &keys, mtx::http::RequestErr err) {
              if (err) {
                  nhlog::net()->warn("failed to query device keys: {} {}",
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
    if (user_id.size() > 255) {
        nhlog::db()->debug("Skipping device key query for user with invalid mxid: {}", user_id);

        mtx::http::ClientError err{};
        err.parse_error = "invalid mxid, more than 255 bytes";
        cb({}, err);
        return;
    }

    mtx::requests::QueryKeys req;
    std::string last_changed;
    {
        auto txn    = ro_txn(env_);
        auto cache_ = userKeys_(user_id, txn);

        if (cache_.has_value()) {
            if (cache_->updated_at == cache_->last_changed) {
                cb(cache_.value(), {});
                return;
            } else
                nhlog::db()->info("Keys outdated for {}: {} vs {}",
                                  user_id,
                                  cache_->updated_at,
                                  cache_->last_changed);
        } else
            nhlog::db()->info("No keys found for {}", user_id);

        req.device_keys[user_id] = {};

        if (cache_)
            last_changed = cache_->last_changed;
        req.token = last_changed;
    }

    // use context object so that we can disconnect again
    QObject *context{new QObject(this)};
    QObject::connect(
      this,
      &Cache::userKeysUpdateFinalize,
      context,
      [cb, user_id, context_ = context, this](std::string updated_user) mutable {
          if (user_id == updated_user) {
              context_->deleteLater();
              auto txn  = ro_txn(env_);
              auto keys = this->userKeys_(user_id, txn);
              cb(keys.value_or(UserKeyCache{}), {});
          }
      },
      Qt::QueuedConnection);

    http::client()->query_keys(
      req,
      [cb, user_id, last_changed, this](const mtx::responses::QueryKeys &res,
                                        mtx::http::RequestErr err) {
          if (err) {
              nhlog::net()->warn("failed to query device keys: {},{}",
                                 mtx::errors::to_string(err->matrix_error.errcode),
                                 static_cast<int>(err->status_code));
              cb({}, err);
              return;
          }

          emit userKeysUpdate(last_changed, res);
          emit userKeysUpdateFinalize(user_id);
      });
}

void
to_json(nlohmann::json &j, const VerificationCache &info)
{
    j["device_verified"] = info.device_verified;
    j["device_blocked"]  = info.device_blocked;
}

void
from_json(const nlohmann::json &j, VerificationCache &info)
{
    info.device_verified = j.at("device_verified").get<std::set<std::string>>();
    info.device_blocked  = j.at("device_blocked").get<std::set<std::string>>();
}

void
to_json(nlohmann::json &j, const OnlineBackupVersion &info)
{
    j["v"] = info.version;
    j["a"] = info.algorithm;
}

void
from_json(const nlohmann::json &j, OnlineBackupVersion &info)
{
    info.version   = j.at("v").get<std::string>();
    info.algorithm = j.at("a").get<std::string>();
}

std::optional<VerificationCache>
Cache::verificationCache(const std::string &user_id, lmdb::txn &txn)
{
    std::string_view verifiedVal;

    auto db = getVerificationDb(txn);

    try {
        VerificationCache verified_state;
        auto res = db.get(txn, user_id, verifiedVal);
        if (res) {
            verified_state = nlohmann::json::parse(verifiedVal).get<VerificationCache>();
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
    {
        std::string_view val;

        auto txn = lmdb::txn::begin(env_);
        auto db  = getVerificationDb(txn);

        try {
            VerificationCache verified_state;
            auto res = db.get(txn, user_id, val);
            if (res) {
                verified_state = nlohmann::json::parse(val).get<VerificationCache>();
            }

            for (const auto &device : verified_state.device_verified)
                if (device == key)
                    return;

            verified_state.device_verified.insert(key);
            db.put(txn, user_id, nlohmann::json(verified_state).dump());
            txn.commit();
        } catch (std::exception &) {
        }
    }

    const auto local_user = utils::localUser().toStdString();
    std::map<std::string, VerificationStatus> tmp;
    {
        std::unique_lock<std::mutex> lock(verification_storage.verification_storage_mtx);
        if (user_id == local_user) {
            std::swap(tmp, verification_storage.status);
            verification_storage.status.clear();
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
    std::string_view val;

    auto txn = lmdb::txn::begin(env_);
    auto db  = getVerificationDb(txn);

    try {
        VerificationCache verified_state;
        auto res = db.get(txn, user_id, val);
        if (res) {
            verified_state = nlohmann::json::parse(val).get<VerificationCache>();
        }

        verified_state.device_verified.erase(key);

        db.put(txn, user_id, nlohmann::json(verified_state).dump());
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
    }
    emit verificationStatusChanged(user_id);
}

VerificationStatus
Cache::verificationStatus(const std::string &user_id)
{
    auto txn = ro_txn(env_);
    return verificationStatus_(user_id, txn);
}

VerificationStatus
Cache::verificationStatus_(const std::string &user_id, lmdb::txn &txn)
{
    std::unique_lock<std::mutex> lock(verification_storage.verification_storage_mtx);
    if (verification_storage.status.count(user_id))
        return verification_storage.status.at(user_id);

    VerificationStatus status;

    // assume there is at least one unverified device until we have checked we have the device
    // list for that user.
    status.unverified_device_count = 1;
    status.no_keys                 = true;

    if (auto verifCache = verificationCache(user_id, txn)) {
        status.verified_devices = verifCache->device_verified;
    }

    const auto local_user = utils::localUser().toStdString();

    crypto::Trust trustlevel = crypto::Trust::Unverified;
    if (user_id == local_user) {
        status.verified_devices.insert(http::client()->device_id());
        trustlevel = crypto::Trust::Verified;
    }

    auto verifyAtLeastOneSig = [](const auto &toVerif,
                                  const std::map<std::string, std::string> &keys,
                                  const std::string &keyOwner) {
        if (!toVerif.signatures.count(keyOwner))
            return false;

        for (const auto &[key_id, signature] : toVerif.signatures.at(keyOwner)) {
            if (!keys.count(key_id))
                continue;

            if (mtx::crypto::ed25519_verify_signature(
                  keys.at(key_id), nlohmann::json(toVerif), signature))
                return true;
        }
        return false;
    };

    auto updateUnverifiedDevices = [&status](auto &theirDeviceKeys) {
        int currentVerifiedDevices = 0;
        for (const auto &device_id : status.verified_devices) {
            if (theirDeviceKeys.count(device_id))
                currentVerifiedDevices++;
        }
        status.unverified_device_count =
          static_cast<int>(theirDeviceKeys.size()) - currentVerifiedDevices;
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
        auto ourKeys   = userKeys_(local_user, txn);
        auto theirKeys = userKeys_(user_id, txn);
        if (theirKeys)
            status.no_keys = false;

        if (!ourKeys || !theirKeys) {
            verification_storage.status[user_id] = status;
            return status;
        }

        // Update verified devices count to count without cross-signing
        updateUnverifiedDevices(theirKeys->device_keys);

        {
            auto &mk           = ourKeys->master_keys;
            std::string dev_id = "ed25519:" + http::client()->device_id();
            if (!mk.signatures.count(local_user) || !mk.signatures.at(local_user).count(dev_id) ||
                !mtx::crypto::ed25519_verify_signature(olm::client()->identity_keys().ed25519,
                                                       nlohmann::json(mk),
                                                       mk.signatures.at(local_user).at(dev_id))) {
                nhlog::crypto()->debug("We have not verified our own master key");
                verification_storage.status[user_id] = status;
                return status;
            }
        }

        auto master_keys = ourKeys->master_keys.keys;

        if (user_id != local_user) {
            bool theirMasterKeyVerified =
              verifyAtLeastOneSig(ourKeys->user_signing_keys, master_keys, local_user) &&
              verifyAtLeastOneSig(
                theirKeys->master_keys, ourKeys->user_signing_keys.keys, local_user);

            if (theirMasterKeyVerified)
                trustlevel = crypto::Trust::Verified;
            else if (!theirKeys->master_key_changed)
                trustlevel = crypto::Trust::TOFU;
            else {
                verification_storage.status[user_id] = status;
                return status;
            }

            master_keys = theirKeys->master_keys.keys;
        }

        status.user_verified = trustlevel;

        verification_storage.status[user_id] = status;
        if (!verifyAtLeastOneSig(theirKeys->self_signing_keys, master_keys, user_id))
            return status;

        for (const auto &[device, device_key] : theirKeys->device_keys) {
            (void)device;
            try {
                auto identkey = device_key.keys.at("curve25519:" + device_key.device_id);
                if (verifyAtLeastOneSig(device_key, theirKeys->self_signing_keys.keys, user_id)) {
                    status.verified_devices.insert(device_key.device_id);
                    status.verified_device_keys[identkey] = trustlevel;
                }
            } catch (...) {
            }
        }

        updateUnverifiedDevices(theirKeys->device_keys);
        verification_storage.status[user_id] = status;
        return status;
    } catch (std::exception &e) {
        nhlog::db()->error("Failed to calculate verification status of {}: {}", user_id, e.what());
        return status;
    }
}

void
to_json(nlohmann::json &j, const RoomInfo &info)
{
    j["name"]         = info.name;
    j["topic"]        = info.topic;
    j["avatar_url"]   = info.avatar_url;
    j["version"]      = info.version;
    j["is_invite"]    = info.is_invite;
    j["is_space"]     = info.is_space;
    j["join_rule"]    = info.join_rule;
    j["guest_access"] = info.guest_access;

    j["app_l_ts"] = info.approximate_last_modification_ts;

    j["notification_count"] = info.notification_count;
    j["highlight_count"]    = info.highlight_count;

    if (info.member_count != 0)
        j["member_count"] = info.member_count;

    if (info.tags.size() != 0)
        j["tags"] = info.tags;
}

void
from_json(const nlohmann::json &j, RoomInfo &info)
{
    info.name       = j.at("name").get<std::string>();
    info.topic      = j.at("topic").get<std::string>();
    info.avatar_url = j.at("avatar_url").get<std::string>();
    info.version    = j.value(
      "version", QCoreApplication::translate("RoomInfo", "no version stored").toStdString());
    info.is_invite    = j.at("is_invite").get<bool>();
    info.is_space     = j.value("is_space", false);
    info.join_rule    = j.at("join_rule").get<mtx::events::state::JoinRule>();
    info.guest_access = j.at("guest_access").get<bool>();

    info.approximate_last_modification_ts = j.value<uint64_t>("app_l_ts", 0);
    // workaround for bad values being stored in the past
    if (info.approximate_last_modification_ts < 100000000000)
        info.approximate_last_modification_ts = 0;

    info.notification_count = j.value("notification_count", 0);
    info.highlight_count    = j.value("highlight_count", 0);

    if (j.count("member_count"))
        info.member_count = j.at("member_count").get<size_t>();

    if (j.count("tags"))
        info.tags = j.at("tags").get<std::vector<std::string>>();
}

void
to_json(nlohmann::json &j, const ReadReceiptKey &key)
{
    j = nlohmann::json{{"event_id", key.event_id}, {"room_id", key.room_id}};
}

void
from_json(const nlohmann::json &j, ReadReceiptKey &key)
{
    key.event_id = j.at("event_id").get<std::string>();
    key.room_id  = j.at("room_id").get<std::string>();
}

void
to_json(nlohmann::json &j, const MemberInfo &info)
{
    j["name"]       = info.name;
    j["avatar_url"] = info.avatar_url;
    if (info.is_direct)
        j["is_direct"] = info.is_direct;
}

void
from_json(const nlohmann::json &j, MemberInfo &info)
{
    info.name       = j.at("name").get<std::string>();
    info.avatar_url = j.at("avatar_url").get<std::string>();
    info.is_direct  = j.value("is_direct", false);
}

void
to_json(nlohmann::json &obj, const DeviceKeysToMsgIndex &msg)
{
    obj["deviceids"] = msg.deviceids;
}

void
from_json(const nlohmann::json &obj, DeviceKeysToMsgIndex &msg)
{
    msg.deviceids = obj.at("deviceids").get<decltype(msg.deviceids)>();
}

void
to_json(nlohmann::json &obj, const SharedWithUsers &msg)
{
    obj["keys"] = msg.keys;
}

void
from_json(const nlohmann::json &obj, SharedWithUsers &msg)
{
    msg.keys = obj.at("keys").get<std::map<std::string, DeviceKeysToMsgIndex>>();
}

void
to_json(nlohmann::json &obj, const GroupSessionData &msg)
{
    obj["message_index"] = msg.message_index;
    obj["ts"]            = msg.timestamp;
    obj["trust"]         = msg.trusted;

    obj["sender_key"]                      = msg.sender_key;
    obj["sender_claimed_ed25519_key"]      = msg.sender_claimed_ed25519_key;
    obj["forwarding_curve25519_key_chain"] = msg.forwarding_curve25519_key_chain;

    obj["currently"] = msg.currently;

    obj["indices"] = msg.indices;
}

void
from_json(const nlohmann::json &obj, GroupSessionData &msg)
{
    msg.message_index = obj.at("message_index").get<uint64_t>();
    msg.timestamp     = obj.value("ts", 0ULL);
    msg.trusted       = obj.value("trust", true);

    msg.sender_key                 = obj.value("sender_key", "");
    msg.sender_claimed_ed25519_key = obj.value("sender_claimed_ed25519_key", "");
    msg.forwarding_curve25519_key_chain =
      obj.value("forwarding_curve25519_key_chain", std::vector<std::string>{});

    msg.currently = obj.value("currently", SharedWithUsers{});

    msg.indices = obj.value("indices", std::map<uint32_t, std::string>());
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
    msg.ed25519    = obj.at("ed25519").get<std::string>();
    msg.curve25519 = obj.at("curve25519").get<std::string>();
}

void
to_json(nlohmann::json &obj, const MegolmSessionIndex &msg)
{
    obj["room_id"]    = msg.room_id;
    obj["session_id"] = msg.session_id;
}

void
from_json(const nlohmann::json &obj, MegolmSessionIndex &msg)
{
    msg.room_id    = obj.at("room_id").get<std::string>();
    msg.session_id = obj.at("session_id").get<std::string>();
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

mtx::events::presence::Presence
presence(const std::string &user_id)
{
    if (!instance_)
        return {};
    return instance_->presence(user_id);
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
QHash<QString, RoomInfo>
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

std::vector<RoomMember>
getMembers(const std::string &room_id, std::size_t startIndex, std::size_t len)
{
    return instance_->getMembers(room_id, startIndex, len);
}

std::vector<RoomMember>
getMembersFromInvite(const std::string &room_id, std::size_t startIndex, std::size_t len)
{
    return instance_->getMembersFromInvite(room_id, startIndex, len);
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

//! Check if the given user has power level greater than
//! lowest power level of the given events.
bool
hasEnoughPowerLevel(const std::vector<mtx::events::EventType> &eventTypes,
                    const std::string &room_id,
                    const std::string &user_id)
{
    return instance_->hasEnoughPowerLevel(eventTypes, room_id, user_id);
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

std::optional<uint64_t>
getEventIndex(const std::string &room_id, std::string_view event_id)
{
    return instance_->getEventIndex(room_id, event_id);
}

std::optional<std::pair<uint64_t, std::string>>
lastInvisibleEventAfter(const std::string &room_id, std::string_view event_id)
{
    return instance_->lastInvisibleEventAfter(room_id, event_id);
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
                          const GroupSessionData &data,
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
                            const GroupSessionData &data,
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
                         mtx::crypto::InboundGroupSessionPtr session,
                         const GroupSessionData &data)
{
    instance_->saveInboundMegolmSession(index, std::move(session), data);
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
std::optional<GroupSessionData>
getMegolmSessionData(const MegolmSessionIndex &index)
{
    return instance_->getMegolmSessionData(index);
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
