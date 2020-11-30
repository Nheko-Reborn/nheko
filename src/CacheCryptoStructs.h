#pragma once

#include <map>
#include <mutex>

#include <mtx/responses/crypto.hpp>
#include <mtxclient/crypto/objects.hpp>

struct DeviceAndMasterKeys
{
        // map from device id or master key id to message_index
        std::map<std::string, uint64_t> devices, master_keys;
};

struct SharedWithUsers
{
        // userid to keys
        std::map<std::string, DeviceAndMasterKeys> keys;
};

// Extra information associated with an outbound megolm session.
struct OutboundGroupSessionData
{
        std::string session_id;
        std::string session_key;
        uint64_t message_index = 0;

        // who has access to this session.
        // Rotate, when a user leaves the room and share, when a user gets added.
        SharedWithUsers initially, currently;
};

void
to_json(nlohmann::json &obj, const OutboundGroupSessionData &msg);
void
from_json(const nlohmann::json &obj, OutboundGroupSessionData &msg);

struct OutboundGroupSessionDataRef
{
        mtx::crypto::OutboundGroupSessionPtr session;
        OutboundGroupSessionData data;
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
        //! The room in which this session exists.
        std::string room_id;
        //! The session_id of the megolm session.
        std::string session_id;
        //! The curve25519 public key of the sender.
        std::string sender_key;
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
        bool user_verified = false;
        //! List of all devices marked as verified
        std::vector<std::string> verified_devices;
};

//! In memory cache of verification status
struct VerificationStorage
{
        //! mapping of user to verification status
        std::map<std::string, VerificationStatus> status;
        std::mutex verification_storage_mtx;
};

// this will store the keys of the user with whom a encrypted room is shared with
struct UserKeyCache
{
        //! Device id to device keys
        std::map<std::string, mtx::crypto::DeviceKeys> device_keys;
        //! corss signing keys
        mtx::crypto::CrossSigningKeys master_keys, user_signing_keys, self_signing_keys;
        //! Sync token when nheko last fetched the keys
        std::string updated_at;
        //! Sync token when the keys last changed. updated != last_changed means they are outdated.
        std::string last_changed;
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
        std::vector<std::string> device_verified;
        //! list of devices the user blocks
        std::vector<std::string> device_blocked;
};

void
to_json(nlohmann::json &j, const VerificationCache &info);
void
from_json(const nlohmann::json &j, VerificationCache &info);
