#include "Olm.hpp"

#include "Cache.h"
#include "Logging.hpp"

using namespace mtx::crypto;

namespace {
auto client_ = std::make_unique<mtx::crypto::OlmClient>();
}

namespace olm {

mtx::crypto::OlmClient *
client()
{
        return client_.get();
}

void
handle_to_device_messages(const std::vector<nlohmann::json> &msgs)
{
        if (msgs.empty())
                return;

        nhlog::crypto()->info("received {} to_device messages", msgs.size());

        for (const auto &msg : msgs) {
                try {
                        OlmMessage olm_msg = msg;
                        handle_olm_message(std::move(olm_msg));
                } catch (const nlohmann::json::exception &e) {
                        nhlog::crypto()->warn(
                          "parsing error for olm message: {} {}", e.what(), msg.dump(2));
                } catch (const std::invalid_argument &e) {
                        nhlog::crypto()->warn(
                          "validation error for olm message: {} {}", e.what(), msg.dump(2));
                }
        }
}

void
handle_olm_message(const OlmMessage &msg)
{
        nhlog::crypto()->info("sender    : {}", msg.sender);
        nhlog::crypto()->info("sender_key: {}", msg.sender_key);

        const auto my_key = olm::client()->identity_keys().curve25519;

        for (const auto &cipher : msg.ciphertext) {
                // We skip messages not meant for the current device.
                if (cipher.first != my_key)
                        continue;

                const auto type = cipher.second.type;
                nhlog::crypto()->info("type: {}", type == 0 ? "OLM_PRE_KEY" : "OLM_MESSAGE");

                if (type == OLM_MESSAGE_TYPE_PRE_KEY)
                        handle_pre_key_olm_message(msg.sender, msg.sender_key, cipher.second);
                else
                        handle_olm_normal_message(msg.sender, msg.sender_key, cipher.second);
        }
}

void
handle_pre_key_olm_message(const std::string &sender,
                           const std::string &sender_key,
                           const OlmCipherContent &content)
{
        nhlog::crypto()->info("opening olm session with {}", sender);

        OlmSessionPtr inbound_session = nullptr;
        try {
                inbound_session = olm::client()->create_inbound_session(content.body);
        } catch (const olm_exception &e) {
                nhlog::crypto()->critical(
                  "failed to create inbound session with {}: {}", sender, e.what());
                return;
        }

        if (!matches_inbound_session_from(inbound_session.get(), sender_key, content.body)) {
                nhlog::crypto()->warn("inbound olm session doesn't match sender's key ({})",
                                      sender);
                return;
        }

        mtx::crypto::BinaryBuf output;
        try {
                output = olm::client()->decrypt_message(
                  inbound_session.get(), OLM_MESSAGE_TYPE_PRE_KEY, content.body);
        } catch (const olm_exception &e) {
                nhlog::crypto()->critical(
                  "failed to decrypt olm message {}: {}", content.body, e.what());
                return;
        }

        auto plaintext = json::parse(std::string((char *)output.data(), output.size()));
        nhlog::crypto()->info("decrypted message: \n {}", plaintext.dump(2));

        std::string room_id, session_id, session_key;
        try {
                room_id     = plaintext.at("content").at("room_id");
                session_id  = plaintext.at("content").at("session_id");
                session_key = plaintext.at("content").at("session_key");
        } catch (const nlohmann::json::exception &e) {
                nhlog::crypto()->critical(
                  "failed to parse plaintext olm message: {} {}", e.what(), plaintext.dump(2));
                return;
        }

        MegolmSessionIndex index;
        index.room_id    = room_id;
        index.session_id = session_id;
        index.sender_key = sender_key;

        if (!cache::client()->inboundMegolmSessionExists(index)) {
                auto megolm_session = olm::client()->init_inbound_group_session(session_key);

                try {
                        cache::client()->saveInboundMegolmSession(index, std::move(megolm_session));
                } catch (const lmdb::error &e) {
                        nhlog::crypto()->critical("failed to save inbound megolm session: {}",
                                                  e.what());
                        return;
                }

                nhlog::crypto()->info(
                  "established inbound megolm session ({}, {})", room_id, sender);
        } else {
                nhlog::crypto()->warn(
                  "inbound megolm session already exists ({}, {})", room_id, sender);
        }
}

void
handle_olm_normal_message(const std::string &, const std::string &, const OlmCipherContent &)
{
        nhlog::crypto()->warn("olm(1) not implemeted yet");
}

mtx::events::msg::Encrypted
encrypt_group_message(const std::string &room_id,
                      const std::string &device_id,
                      const std::string &body)
{
        using namespace mtx::events;

        // Always chech before for existence.
        auto res     = cache::client()->getOutboundMegolmSession(room_id);
        auto payload = olm::client()->encrypt_group_message(res.session, body);

        // Prepare the m.room.encrypted event.
        msg::Encrypted data;
        data.ciphertext = std::string((char *)payload.data(), payload.size());
        data.sender_key = olm::client()->identity_keys().curve25519;
        data.session_id = res.data.session_id;
        data.device_id  = device_id;

        auto message_index = olm_outbound_group_session_message_index(res.session);
        nhlog::crypto()->info("next message_index {}", message_index);

        // We need to re-pickle the session after we send a message to save the new message_index.
        cache::client()->updateOutboundMegolmSession(room_id, message_index);

        return data;
}

} // namespace olm
