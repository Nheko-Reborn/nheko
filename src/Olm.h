#pragma once

#include <boost/optional.hpp>

#include <memory>
#include <mtx/events.hpp>
#include <mtx/events/encrypted.hpp>
#include <mtxclient/crypto/client.hpp>

#include <CacheCryptoStructs.h>

constexpr auto OLM_ALGO = "m.olm.v1.curve25519-aes-sha2";

namespace olm {

enum class DecryptionErrorCode
{
        MissingSession, // Session was not found, retrieve from backup or request from other devices
                        // and try again
        DbError,        // DB read failed
        DecryptionFailed,   // libolm error
        ParsingFailed,      // Failed to parse the actual event
        ReplayAttack,       // Megolm index reused
        UnknownFingerprint, // Unknown device Fingerprint
};

struct DecryptionResult
{
        std::optional<DecryptionErrorCode> error;
        std::optional<std::string> error_message;
        std::optional<mtx::events::collections::TimelineEvents> event;
};

struct OlmMessage
{
        std::string sender_key;
        std::string sender;

        using RecipientKey = std::string;
        std::map<RecipientKey, mtx::events::msg::OlmCipherContent> ciphertext;
};

inline void
from_json(const nlohmann::json &obj, OlmMessage &msg)
{
        if (obj.at("type") != "m.room.encrypted")
                throw std::invalid_argument("invalid type for olm message");

        if (obj.at("content").at("algorithm") != OLM_ALGO)
                throw std::invalid_argument("invalid algorithm for olm message");

        msg.sender     = obj.at("sender");
        msg.sender_key = obj.at("content").at("sender_key");
        msg.ciphertext = obj.at("content")
                           .at("ciphertext")
                           .get<std::map<std::string, mtx::events::msg::OlmCipherContent>>();
}

mtx::crypto::OlmClient *
client();

void
handle_to_device_messages(const std::vector<mtx::events::collections::DeviceEvents> &msgs);

nlohmann::json
try_olm_decryption(const std::string &sender_key,
                   const mtx::events::msg::OlmCipherContent &content);

void
handle_olm_message(const OlmMessage &msg);

//! Establish a new inbound megolm session with the decrypted payload from olm.
void
create_inbound_megolm_session(const std::string &sender,
                              const std::string &sender_key,
                              const nlohmann::json &payload);

void
handle_pre_key_olm_message(const std::string &sender,
                           const std::string &sender_key,
                           const mtx::events::msg::OlmCipherContent &content);

mtx::events::msg::Encrypted
encrypt_group_message(const std::string &room_id,
                      const std::string &device_id,
                      nlohmann::json body);

DecryptionResult
decryptEvent(const MegolmSessionIndex &index,
             const mtx::events::EncryptedEvent<mtx::events::msg::Encrypted> &event);

void
mark_keys_as_published();

//! Request the encryption keys from sender's device for the given event.
void
request_keys(const std::string &room_id, const std::string &event_id);

void
send_key_request_for(const std::string &room_id,
                     const mtx::events::EncryptedEvent<mtx::events::msg::Encrypted> &);

void
handle_key_request_message(const mtx::events::DeviceEvent<mtx::events::msg::KeyRequest> &);

void
send_megolm_key_to_device(const std::string &user_id,
                          const std::string &device_id,
                          const json &payload);

} // namespace olm
