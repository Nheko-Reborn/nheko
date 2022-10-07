// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

#include <map>
#include <mutex>
#include <set>

#include <mtx/events/encrypted.hpp>
#include <mtx/responses/crypto.hpp>
#include <mtxclient/crypto/objects.hpp>

namespace crypto {
Q_NAMESPACE
//! How much a participant is trusted.
enum Trust
{
    Unverified, //! Device unverified or master key changed.
    TOFU,       //! Device is signed by the sender, but the user is not verified, but they never
                //! changed the master key.
    Verified,   //! User was verified and has crosssigned this device or device is verified.
};
Q_ENUM_NS(Trust)
}

struct DeviceKeysToMsgIndex
{
    // map from device key to message_index
    // Using the device id is safe because we check for reuse on device list updates
    // Using the device id makes our logic much easier to read.
    std::map<std::string, uint64_t> deviceids;
};

struct SharedWithUsers
{
    // userid to keys
    std::map<std::string, DeviceKeysToMsgIndex> keys;
};

// Extra information associated with an outbound megolm session.
struct GroupSessionData
{
    uint64_t message_index = 0;
    uint64_t timestamp     = 0;

    // If we got the session via key sharing or forwarding, we can usually trust it.
    // If it came from asymmetric key backup, it is not trusted.
    // TODO(Nico): What about forwards? They might come from key backup?
    bool trusted = true;

    // the original 25519 key
    std::string sender_key;

    std::string sender_claimed_ed25519_key;
    std::vector<std::string> forwarding_curve25519_key_chain;

    //! map from index to event_id to check for replay attacks
    std::map<uint32_t, std::string> indices;

    // who has access to this session.
    // Rotate, when a user leaves the room and share, when a user gets added.
    SharedWithUsers currently;
};

void
to_json(nlohmann::json &obj, const GroupSessionData &msg);
void
from_json(const nlohmann::json &obj, GroupSessionData &msg);

struct OutboundGroupSessionDataRef
{
    mtx::crypto::OutboundGroupSessionPtr session;
    GroupSessionData data;
};

struct DevicePublicKeys
{
    std::string ed25519;
    std::string curve25519;
};

void
to_json(nlohmann::json &obj, const DevicePublicKeys &msg);
void
from_json(const nlohmann::json &obj, DevicePublicKeys &msg);

//! Represents a unique megolm session identifier.
struct MegolmSessionIndex
{
    MegolmSessionIndex() = default;
    MegolmSessionIndex(std::string room_id_, const mtx::events::msg::Encrypted &e)
      : room_id(std::move(room_id_))
      , session_id(e.session_id)
    {}

    //! The room in which this session exists.
    std::string room_id;
    //! The session_id of the megolm session.
    std::string session_id;
};

void
to_json(nlohmann::json &obj, const MegolmSessionIndex &msg);
void
from_json(const nlohmann::json &obj, MegolmSessionIndex &msg);

struct StoredOlmSession
{
    std::uint64_t last_message_ts = 0;
    std::string pickled_session;
};
void
to_json(nlohmann::json &obj, const StoredOlmSession &msg);
void
from_json(const nlohmann::json &obj, StoredOlmSession &msg);

//! Verification status of a single user
struct VerificationStatus
{
    //! True, if the users master key is verified
    crypto::Trust user_verified = crypto::Trust::Unverified;
    //! List of all devices marked as verified
    std::set<std::string> verified_devices;
    //! Map from sender key/curve25519 to trust status
    std::map<std::string, crypto::Trust> verified_device_keys;
    //! Count of unverified devices
    int unverified_device_count = 0;
    // if the keys are not in cache
    bool no_keys = false;
};

//! In memory cache of verification status
struct VerificationStorage
{
    //! mapping of user to verification status
    std::map<std::string, VerificationStatus> status;
    std::mutex verification_storage_mtx;
};

//! In memory cache of verification status
struct SecretsStorage
{
    //! secret name -> secret
    std::map<std::string, std::string> secrets;
    std::mutex mtx;
};

// this will store the keys of the user with whom a encrypted room is shared with
struct UserKeyCache
{
    //! Device id to device keys
    std::map<std::string, mtx::crypto::DeviceKeys> device_keys;
    //! cross signing keys
    mtx::crypto::CrossSigningKeys master_keys, user_signing_keys, self_signing_keys;
    //! Sync token when nheko last fetched the keys
    std::string updated_at;
    //! Sync token when the keys last changed. updated != last_changed means they are outdated.
    std::string last_changed;
    //! if the master key has ever changed
    bool master_key_changed = false;
    //! Device keys that were already used at least once
    std::set<std::string> seen_device_keys;
    //! Device ids that were already used at least once
    std::set<std::string> seen_device_ids;
};

void
to_json(nlohmann::json &j, const UserKeyCache &info);
void
from_json(const nlohmann::json &j, UserKeyCache &info);

// the reason these are stored in a seperate cache rather than storing it in the user cache is
// UserKeyCache stores only keys of users with which encrypted room is shared
struct VerificationCache
{
    //! list of verified device_ids with device-verification
    std::set<std::string> device_verified;
    //! list of devices the user blocks
    std::set<std::string> device_blocked;
};

void
to_json(nlohmann::json &j, const VerificationCache &info);
void
from_json(const nlohmann::json &j, VerificationCache &info);

struct OnlineBackupVersion
{
    //! the version of the online backup currently enabled
    std::string version;
    //! the algorithm used by the backup
    std::string algorithm;
};

void
to_json(nlohmann::json &j, const OnlineBackupVersion &info);
void
from_json(const nlohmann::json &j, OnlineBackupVersion &info);
