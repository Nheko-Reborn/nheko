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

#include <QByteArray>
#include <QDebug>
#include <QFile>
#include <QHash>
#include <QStandardPaths>

#include <variant.hpp>

#include "Cache.h"
#include "Utils.h"

//! Should be changed when a breaking change occurs in the cache format.
//! This will reset client's data.
static const std::string CURRENT_CACHE_FORMAT_VERSION("2018.05.11");

static const lmdb::val NEXT_BATCH_KEY("next_batch");
static const lmdb::val CACHE_FORMAT_VERSION_KEY("cache_format_version");

//! Cache databases and their format.
//!
//! Contains UI information for the joined rooms. (i.e name, topic, avatar url etc).
//! Format: room_id -> RoomInfo
static constexpr const char *ROOMS_DB   = "rooms";
static constexpr const char *INVITES_DB = "invites";
//! Keeps already downloaded media for reuse.
//! Format: matrix_url -> binary data.
static constexpr const char *MEDIA_DB = "media";
//! Information that  must be kept between sync requests.
static constexpr const char *SYNC_STATE_DB = "sync_state";
//! Read receipts per room/event.
static constexpr const char *READ_RECEIPTS_DB = "read_receipts";
static constexpr const char *NOTIFICATIONS_DB = "sent_notifications";

using CachedReceipts = std::multimap<uint64_t, std::string, std::greater<uint64_t>>;
using Receipts       = std::map<std::string, std::map<std::string, uint64_t>>;

namespace {
std::unique_ptr<Cache> instance_ = nullptr;
}

namespace cache {
void
init(const QString &user_id)
{
        if (!instance_)
                instance_ = std::make_unique<Cache>(user_id);
}

Cache *
client()
{
        return instance_.get();
}
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
  , localUserId_{userId}
{}

void
Cache::setup()
{
        qDebug() << "Setting up cache";

        auto statePath = QString("%1/%2/state")
                           .arg(QStandardPaths::writableLocation(QStandardPaths::CacheLocation))
                           .arg(QString::fromUtf8(localUserId_.toUtf8().toHex()));

        cacheDirectory_ = QString("%1/%2")
                            .arg(QStandardPaths::writableLocation(QStandardPaths::CacheLocation))
                            .arg(QString::fromUtf8(localUserId_.toUtf8().toHex()));

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

        auto txn         = lmdb::txn::begin(env_);
        syncStateDb_     = lmdb::dbi::open(txn, SYNC_STATE_DB, MDB_CREATE);
        roomsDb_         = lmdb::dbi::open(txn, ROOMS_DB, MDB_CREATE);
        invitesDb_       = lmdb::dbi::open(txn, INVITES_DB, MDB_CREATE);
        mediaDb_         = lmdb::dbi::open(txn, MEDIA_DB, MDB_CREATE);
        readReceiptsDb_  = lmdb::dbi::open(txn, READ_RECEIPTS_DB, MDB_CREATE);
        notificationsDb_ = lmdb::dbi::open(txn, NOTIFICATIONS_DB, MDB_CREATE);
        txn.commit();

        qRegisterMetaType<RoomInfo>();
}

void
Cache::saveImage(const QString &url, const QByteArray &image)
{
        auto key = url.toUtf8();

        try {
                auto txn = lmdb::txn::begin(env_);

                lmdb::dbi_put(txn,
                              mediaDb_,
                              lmdb::val(key.data(), key.size()),
                              lmdb::val(image.data(), image.size()));

                txn.commit();
        } catch (const lmdb::error &e) {
                qCritical() << "saveImage:" << e.what();
        }
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
                qCritical() << "image:" << e.what() << QString::fromStdString(url);
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
                qCritical() << "image:" << e.what() << url;
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

QString
Cache::nextBatchToken() const
{
        auto txn = lmdb::txn::begin(env_, nullptr, MDB_RDONLY);
        lmdb::val token;

        lmdb::dbi_get(txn, syncStateDb_, NEXT_BATCH_KEY, token);

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
        bool res = lmdb::dbi_get(txn, syncStateDb_, CACHE_FORMAT_VERSION_KEY, current_version);

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
                qCritical() << "readReceipts:" << e.what();
        }

        return receipts;
}

void
Cache::updateReadReceipt(lmdb::txn &txn, const std::string &room_id, const Receipts &receipts)
{
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
                        for (const auto &event_receipt : event_receipts)
                                saved_receipts.emplace(event_receipt.first, event_receipt.second);

                        // Save back the merged (or only the new) receipts.
                        nlohmann::json json_updated_value = saved_receipts;
                        std::string merged_receipts       = json_updated_value.dump();

                        lmdb::dbi_put(txn,
                                      readReceiptsDb_,
                                      lmdb::val(key.data(), key.size()),
                                      lmdb::val(merged_receipts.data(), merged_receipts.size()));

                } catch (const lmdb::error &e) {
                        qCritical() << "updateReadReceipts:" << e.what();
                }
        }
}

void
Cache::saveState(const mtx::responses::Sync &res)
{
        auto txn = lmdb::txn::begin(env_);

        setNextBatchToken(txn, res.next_batch);

        // Save joined rooms
        for (const auto &room : res.rooms.join) {
                auto statesdb  = getStatesDb(txn, room.first);
                auto membersdb = getMembersDb(txn, room.first);

                saveStateEvents(txn, statesdb, membersdb, room.first, room.second.state.events);
                saveStateEvents(txn, statesdb, membersdb, room.first, room.second.timeline.events);

                RoomInfo updatedInfo;
                updatedInfo.name  = getRoomName(txn, statesdb, membersdb).toStdString();
                updatedInfo.topic = getRoomTopic(txn, statesdb).toStdString();
                updatedInfo.avatar_url =
                  getRoomAvatarUrl(txn, statesdb, membersdb, QString::fromStdString(room.first))
                    .toStdString();

                lmdb::dbi_put(
                  txn, roomsDb_, lmdb::val(room.first), lmdb::val(json(updatedInfo).dump()));

                updateReadReceipt(txn, room.first, room.second.ephemeral.receipts);

                // Clean up non-valid invites.
                removeInvite(txn, room.first);
        }

        saveInvites(txn, res.rooms.invite);

        removeLeftRooms(txn, res.rooms.leave);

        txn.commit();
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
                if (mpark::holds_alternative<StrippedEvent<Member>>(e)) {
                        auto msg = mpark::get<StrippedEvent<Member>>(e);

                        auto display_name = msg.content.display_name.empty()
                                              ? msg.state_key
                                              : msg.content.display_name;

                        MemberInfo tmp{display_name, msg.content.avatar_url};

                        lmdb::dbi_put(
                          txn, membersdb, lmdb::val(msg.state_key), lmdb::val(json(tmp).dump()));
                } else {
                        mpark::visit(
                          [&txn, &statesdb](auto msg) {
                                  bool res = lmdb::dbi_put(txn,
                                                           statesdb,
                                                           lmdb::val(to_string(msg.type)),
                                                           lmdb::val(json(msg).dump()));

                                  if (!res)
                                          std::cout << "couldn't save data" << json(msg).dump()
                                                    << '\n';
                          },
                          e);
                }
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
                        qWarning()
                          << "failed to parse room info:" << QString::fromStdString(room_id)
                          << QString::fromStdString(std::string(data.data(), data.size()));
                }
        }

        txn.commit();

        return RoomInfo();
}

std::map<QString, RoomInfo>
Cache::getRoomInfo(const std::vector<std::string> &rooms)
{
        std::map<QString, RoomInfo> room_info;

        auto txn = lmdb::txn::begin(env_, nullptr, MDB_RDONLY);

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
                                qWarning()
                                  << "failed to parse room info:" << QString::fromStdString(room)
                                  << QString::fromStdString(std::string(data.data(), data.size()));
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
                                        qWarning() << "failed to parse room info for invite:"
                                                   << QString::fromStdString(room)
                                                   << QString::fromStdString(
                                                        std::string(data.data(), data.size()));
                                }
                        }
                }
        }

        txn.commit();

        return room_info;
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

                        return QString::fromStdString(msg.content.url);
                } catch (const json::exception &e) {
                        qWarning() << QString::fromStdString(e.what());
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
                        qWarning() << QString::fromStdString(e.what());
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
                        qWarning() << QString::fromStdString(e.what());
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
                        qWarning() << QString::fromStdString(e.what());
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
                        qWarning() << QString::fromStdString(e.what());
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
                return QString("%1 and %2 others").arg(first_member).arg(total);

        return "Empty Room";
}

JoinRule
Cache::getRoomJoinRule(lmdb::txn &txn, lmdb::dbi &statesdb)
{
        using namespace mtx::events;
        using namespace mtx::events::state;

        lmdb::val event;
        bool res = lmdb::dbi_get(
          txn, statesdb, lmdb::val(to_string(mtx::events::EventType::RoomJoinRules)), event);

        if (res) {
                try {
                        StateEvent<JoinRules> msg =
                          json::parse(std::string(event.data(), event.size()));
                        return msg.content.join_rule;
                } catch (const json::exception &e) {
                        qWarning() << e.what();
                }
        }
        return JoinRule::Knock;
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
                        qWarning() << e.what();
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
                        qWarning() << QString::fromStdString(e.what());
                }
        }

        return QString();
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
                        qWarning() << QString::fromStdString(e.what());
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
                        qWarning() << QString::fromStdString(e.what());
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
                        qWarning() << QString::fromStdString(e.what());
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
                        qWarning() << QString::fromStdString(e.what());
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
                        qWarning() << QString::fromStdString(e.what());
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
                qWarning() << "failed to parse room info" << e.what()
                           << QString::fromStdString(std::string(response.data(), response.size()));
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
        qDebug() << "loading" << rooms.size() << "rooms";

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
                results.push_back(
                  RoomSearchResult{it->second.first,
                                   it->second.second,
                                   QImage::fromData(image(txn, it->second.second.avatar_url))});
        }

        txn.commit();

        return results;
}

QVector<SearchResult>
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

        QVector<SearchResult> results;
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
                        qWarning() << e.what();
                }

                currentIndex += 1;
        }

        cursor.close();
        txn.commit();

        return members;
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
                        qWarning() << "hasEnoughPowerLevel: " << e.what();
                }
        }

        txn.commit();

        return user_level >= min_event_level;
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
