// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>
#include <mtx/events.hpp>
#include <mtx/events/encrypted.hpp>
#include <mtxclient/crypto/client.hpp>

#include <CacheCryptoStructs.h>

constexpr auto OLM_ALGO = "m.olm.v1.curve25519-aes-sha2";

namespace olm {
Q_NAMESPACE

enum DecryptionErrorCode
{
    NoError,
    MissingSession, // Session was not found, retrieve from backup or request from other devices
                    // and try again
    MissingSessionIndex, // Session was found, but it does not reach back enough to this index,
                         // retrieve from backup or request from other devices and try again
    DbError,             // DB read failed
    DecryptionFailed,    // libolm error
    ParsingFailed,       // Failed to parse the actual event
    ReplayAttack,        // Megolm index reused
};
Q_ENUM_NS(DecryptionErrorCode)

struct DecryptionResult
{
    DecryptionErrorCode error;
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

void
from_json(const nlohmann::json &obj, OlmMessage &msg);

mtx::crypto::OlmClient *
client();

void
handle_to_device_messages(const std::vector<mtx::events::collections::DeviceEvents> &msgs);

nlohmann::json
try_olm_decryption(const std::string &sender_key,
                   const mtx::events::msg::OlmCipherContent &content);

void
handle_olm_message(const OlmMessage &msg, const UserKeyCache &otherUserDeviceKeys);

//! Establish a new inbound megolm session with the decrypted payload from olm.
void
create_inbound_megolm_session(const mtx::events::DeviceEvent<mtx::events::msg::RoomKey> &roomKey,
                              const std::string &sender_key,
                              const std::string &sender_ed25519);
void
import_inbound_megolm_session(
  const mtx::events::DeviceEvent<mtx::events::msg::ForwardedRoomKey> &roomKey);
void
lookup_keybackup(const std::string room, const std::string session_id);
void
download_full_keybackup();

nlohmann::json
handle_pre_key_olm_message(const std::string &sender,
                           const std::string &sender_key,
                           const mtx::events::msg::OlmCipherContent &content);

mtx::events::msg::Encrypted
encrypt_group_message_with_session(mtx::crypto::OutboundGroupSessionPtr &session,
                                   const std::string &device_id,
                                   nlohmann::json body);

mtx::events::msg::Encrypted
encrypt_group_message(const std::string &room_id,
                      const std::string &device_id,
                      nlohmann::json body);

//! Decrypt an event. Use dont_write_db to prevent db writes when already in a write transaction.
DecryptionResult
decryptEvent(const MegolmSessionIndex &index,
             const mtx::events::EncryptedEvent<mtx::events::msg::Encrypted> &event,
             bool dont_write_db = false);
crypto::Trust
calculate_trust(const std::string &user_id, const MegolmSessionIndex &index);

void
mark_keys_as_published();

//! Request the encryption keys from sender's device for the given event.
void
send_key_request_for(mtx::events::EncryptedEvent<mtx::events::msg::Encrypted> e,
                     const std::string &request_id,
                     bool cancel = false);

void
handle_key_request_message(const mtx::events::DeviceEvent<mtx::events::msg::KeyRequest> &);

void
send_megolm_key_to_device(const std::string &user_id,
                          const std::string &device_id,
                          const mtx::events::msg::ForwardedRoomKey &payload);

//! Send encrypted to device messages, targets is a map from userid to device ids or {} for all
//! devices
void
send_encrypted_to_device_messages(const std::map<std::string, std::vector<std::string>> targets,
                                  const mtx::events::collections::DeviceEvents &event,
                                  bool force_new_session = false);

//! Request backup and signing keys and cache them locally
void
request_cross_signing_keys();
//! Download backup and signing keys and cache them locally
void
download_cross_signing_keys();

} // namespace olm
