#pragma once

#include <boost/optional.hpp>

#include <memory>
#include <mtx.hpp>
#include <mtxclient/crypto/client.hpp>

constexpr auto OLM_ALGO = "m.olm.v1.curve25519-aes-sha2";

namespace olm {

struct OlmCipherContent
{
        std::string body;
        uint8_t type;
};

inline void
from_json(const nlohmann::json &obj, OlmCipherContent &msg)
{
        msg.body = obj.at("body");
        msg.type = obj.at("type");
}

struct OlmMessage
{
        std::string sender_key;
        std::string sender;

        using RecipientKey = std::string;
        std::map<RecipientKey, OlmCipherContent> ciphertext;
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
        msg.ciphertext =
          obj.at("content").at("ciphertext").get<std::map<std::string, OlmCipherContent>>();
}

mtx::crypto::OlmClient *
client();

void
handle_to_device_messages(const std::vector<nlohmann::json> &msgs);

boost::optional<json>
try_olm_decryption(const std::string &sender_key, const OlmCipherContent &content);

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
                           const OlmCipherContent &content);

mtx::events::msg::Encrypted
encrypt_group_message(const std::string &room_id,
                      const std::string &device_id,
                      const std::string &body);

} // namespace olm
