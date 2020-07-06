#pragma once

#include <map>
#include <mutex>

//#include <nlohmann/json.hpp>

#include <mtx/responses.hpp>
#include <mtxclient/crypto/client.hpp>

// Extra information associated with an outbound megolm session.
struct OutboundGroupSessionData
{
        std::string session_id;
        std::string session_key;
        uint64_t message_index = 0;
};

void
to_json(nlohmann::json &obj, const OutboundGroupSessionData &msg);
void
from_json(const nlohmann::json &obj, OutboundGroupSessionData &msg);

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

struct OlmSessionStorage
{
        // Megolm sessions
        std::map<std::string, mtx::crypto::InboundGroupSessionPtr> group_inbound_sessions;
        std::map<std::string, mtx::crypto::OutboundGroupSessionPtr> group_outbound_sessions;
        std::map<std::string, OutboundGroupSessionData> group_outbound_session_data;

        // Guards for accessing megolm sessions.
        std::mutex group_outbound_mtx;
        std::mutex group_inbound_mtx;
};

// this will store the keys of the user with whom a encrypted room is shared with
struct UserCache
{
        //! map of public key key_ids and their public_key
        mtx::responses::QueryKeys keys;

        UserCache(mtx::responses::QueryKeys res)
          : keys(res)
        {}
        UserCache() {}
};

void
to_json(nlohmann::json &j, const UserCache &info);
void
from_json(const nlohmann::json &j, UserCache &info);

// the reason these are stored in a seperate cache rather than storing it in the user cache is
// UserCache stores only keys of users with which encrypted room is shared
struct DeviceVerifiedCache
{
        //! list of verified device_ids with device-verification
        std::vector<std::string> device_verified;
        //! list of verified device_ids with cross-signing
        std::vector<std::string> cross_verified;
        //! list of devices the user blocks
        std::vector<std::string> device_blocked;
        //! this stores if the user is verified (with cross-signing)
        bool is_user_verified = false;

        DeviceVerifiedCache(std::vector<std::string> device_verified_,
                            std::vector<std::string> cross_verified_,
                            std::vector<std::string> device_blocked_,
                            bool is_user_verified_ = false)
          : device_verified(device_verified_)
          , cross_verified(cross_verified_)
          , device_blocked(device_blocked_)
          , is_user_verified(is_user_verified_)
        {}

        DeviceVerifiedCache() {}
};

void
to_json(nlohmann::json &j, const DeviceVerifiedCache &info);
void
from_json(const nlohmann::json &j, DeviceVerifiedCache &info);
