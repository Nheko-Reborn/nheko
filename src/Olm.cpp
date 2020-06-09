#include <QObject>
#include <variant>

#include "Olm.h"

#include "Cache.h"
#include "ChatPage.h"
#include "Logging.h"
#include "MatrixClient.h"
#include "Utils.h"
#include <DeviceVerificationFlow.h>
#include <iostream> // only for debugging

static const std::string STORAGE_SECRET_KEY("secret");
constexpr auto MEGOLM_ALGO = "m.megolm.v1.aes-sha2";

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
handle_to_device_messages(const std::vector<mtx::events::collections::DeviceEvents> &msgs)
{
        if (msgs.empty())
                return;
        nhlog::crypto()->info("received {} to_device messages", msgs.size());
        nlohmann::json j_msg;

        for (const auto &msg : msgs) {
                j_msg = std::visit([](auto &e) { return json(e); }, std::move(msg));
                if (j_msg.count("type") == 0) {
                        nhlog::crypto()->warn("received message with no type field: {}",
                                              j_msg.dump(2));
                        continue;
                }

                std::string msg_type = j_msg.at("type");

                if (msg_type == to_string(mtx::events::EventType::RoomEncrypted)) {
                        try {
                                OlmMessage olm_msg = j_msg;
                                handle_olm_message(std::move(olm_msg));
                        } catch (const nlohmann::json::exception &e) {
                                nhlog::crypto()->warn(
                                  "parsing error for olm message: {} {}", e.what(), j_msg.dump(2));
                        } catch (const std::invalid_argument &e) {
                                nhlog::crypto()->warn("validation error for olm message: {} {}",
                                                      e.what(),
                                                      j_msg.dump(2));

                                nhlog::crypto()->warn("validation error for olm message: {} {}",
                                                      e.what(),
                                                      j_msg.dump(2));
                        }

                } else if (msg_type == to_string(mtx::events::EventType::RoomKeyRequest)) {
                        nhlog::crypto()->warn("handling key request event: {}", j_msg.dump(2));
                        try {
                                mtx::events::DeviceEvent<mtx::events::msg::KeyRequest> req = j_msg;
                                if (req.content.action == mtx::events::msg::RequestAction::Request)
                                        handle_key_request_message(req);
                                else
                                        nhlog::crypto()->warn(
                                          "ignore key request (unhandled action): {}",
                                          req.content.request_id);
                        } catch (const nlohmann::json::exception &e) {
                                nhlog::crypto()->warn(
                                  "parsing error for key_request message: {} {}",
                                  e.what(),
                                  j_msg.dump(2));
                        }
                } else if (msg_type == to_string(mtx::events::EventType::KeyVerificationAccept)) {
                        ChatPage::instance()->recievedDeviceVerificationAccept(msg);
                        std::cout << j_msg.dump(2) << std::endl;
                } else if (msg_type == to_string(mtx::events::EventType::KeyVerificationRequest)) {
                        ChatPage::instance()->recievedDeviceVerificationRequest(msg);
                        std::cout << j_msg.dump(2) << std::endl;
                } else if (msg_type == to_string(mtx::events::EventType::KeyVerificationCancel)) {
                        ChatPage::instance()->recievedDeviceVerificationCancel(msg);
                        std::cout << j_msg.dump(2) << std::endl;
                } else if (msg_type == to_string(mtx::events::EventType::KeyVerificationKey)) {
                        ChatPage::instance()->recievedDeviceVerificationKey(msg);
                        std::cout << j_msg.dump(2) << std::endl;
                } else if (msg_type == to_string(mtx::events::EventType::KeyVerificationMac)) {
                        ChatPage::instance()->recievedDeviceVerificationMac(msg);
                        std::cout << j_msg.dump(2) << std::endl;
                } else if (msg_type == to_string(mtx::events::EventType::KeyVerificationStart)) {
                        ChatPage::instance()->recievedDeviceVerificationStart(msg);
                        std::cout << j_msg.dump(2) << std::endl;
                } else {
                        nhlog::crypto()->warn("unhandled event: {}", j_msg.dump(2));
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

                auto payload = try_olm_decryption(msg.sender_key, cipher.second);

                if (!payload.is_null()) {
                        nhlog::crypto()->debug("decrypted olm payload: {}", payload.dump(2));
                        create_inbound_megolm_session(msg.sender, msg.sender_key, payload);
                        return;
                }

                // Not a PRE_KEY message
                if (cipher.second.type != 0) {
                        // TODO: log that it should have matched something
                        return;
                }

                handle_pre_key_olm_message(msg.sender, msg.sender_key, cipher.second);
        }
}

void
handle_pre_key_olm_message(const std::string &sender,
                           const std::string &sender_key,
                           const mtx::events::msg::OlmCipherContent &content)
{
        nhlog::crypto()->info("opening olm session with {}", sender);

        mtx::crypto::OlmSessionPtr inbound_session = nullptr;
        try {
                inbound_session =
                  olm::client()->create_inbound_session_from(sender_key, content.body);

                // We also remove the one time key used to establish that
                // session so we'll have to update our copy of the account object.
                cache::saveOlmAccount(olm::client()->save("secret"));
        } catch (const mtx::crypto::olm_exception &e) {
                nhlog::crypto()->critical(
                  "failed to create inbound session with {}: {}", sender, e.what());
                return;
        }

        if (!mtx::crypto::matches_inbound_session_from(
              inbound_session.get(), sender_key, content.body)) {
                nhlog::crypto()->warn("inbound olm session doesn't match sender's key ({})",
                                      sender);
                return;
        }

        mtx::crypto::BinaryBuf output;
        try {
                output =
                  olm::client()->decrypt_message(inbound_session.get(), content.type, content.body);
        } catch (const mtx::crypto::olm_exception &e) {
                nhlog::crypto()->critical(
                  "failed to decrypt olm message {}: {}", content.body, e.what());
                return;
        }

        auto plaintext = json::parse(std::string((char *)output.data(), output.size()));
        nhlog::crypto()->debug("decrypted message: \n {}", plaintext.dump(2));

        try {
                cache::saveOlmSession(sender_key, std::move(inbound_session));
        } catch (const lmdb::error &e) {
                nhlog::db()->warn(
                  "failed to save inbound olm session from {}: {}", sender, e.what());
        }

        create_inbound_megolm_session(sender, sender_key, plaintext);
}

mtx::events::msg::Encrypted
encrypt_group_message(const std::string &room_id, const std::string &device_id, nlohmann::json body)
{
        using namespace mtx::events;

        // relations shouldn't be encrypted...
        mtx::common::ReplyRelatesTo relation;
        if (body["content"].contains("m.relates_to") &&
            body["content"]["m.relates_to"].contains("m.in_reply_to")) {
                relation = body["content"]["m.relates_to"];
                body["content"].erase("m.relates_to");
        }

        // Always check before for existence.
        auto res     = cache::getOutboundMegolmSession(room_id);
        auto payload = olm::client()->encrypt_group_message(res.session, body.dump());

        // Prepare the m.room.encrypted event.
        msg::Encrypted data;
        data.ciphertext = std::string((char *)payload.data(), payload.size());
        data.sender_key = olm::client()->identity_keys().curve25519;
        data.session_id = res.data.session_id;
        data.device_id  = device_id;
        data.algorithm  = MEGOLM_ALGO;
        data.relates_to = relation;

        auto message_index = olm_outbound_group_session_message_index(res.session);
        nhlog::crypto()->debug("next message_index {}", message_index);

        // We need to re-pickle the session after we send a message to save the new message_index.
        cache::updateOutboundMegolmSession(room_id, message_index);

        return data;
}

nlohmann::json
try_olm_decryption(const std::string &sender_key, const mtx::events::msg::OlmCipherContent &msg)
{
        auto session_ids = cache::getOlmSessions(sender_key);

        nhlog::crypto()->info("attempt to decrypt message with {} known session_ids",
                              session_ids.size());

        for (const auto &id : session_ids) {
                auto session = cache::getOlmSession(sender_key, id);

                if (!session)
                        continue;

                mtx::crypto::BinaryBuf text;

                try {
                        text = olm::client()->decrypt_message(session->get(), msg.type, msg.body);
                        cache::saveOlmSession(id, std::move(session.value()));
                } catch (const mtx::crypto::olm_exception &e) {
                        nhlog::crypto()->debug("failed to decrypt olm message ({}, {}) with {}: {}",
                                               msg.type,
                                               sender_key,
                                               id,
                                               e.what());
                        continue;
                } catch (const lmdb::error &e) {
                        nhlog::crypto()->critical("failed to save session: {}", e.what());
                        return {};
                }

                try {
                        return json::parse(std::string((char *)text.data(), text.size()));
                } catch (const json::exception &e) {
                        nhlog::crypto()->critical("failed to parse the decrypted session msg: {}",
                                                  e.what());
                }
        }

        return {};
}

void
create_inbound_megolm_session(const std::string &sender,
                              const std::string &sender_key,
                              const nlohmann::json &payload)
{
        std::string room_id, session_id, session_key;

        try {
                room_id     = payload.at("content").at("room_id");
                session_id  = payload.at("content").at("session_id");
                session_key = payload.at("content").at("session_key");
        } catch (const nlohmann::json::exception &e) {
                nhlog::crypto()->critical(
                  "failed to parse plaintext olm message: {} {}", e.what(), payload.dump(2));
                return;
        }

        MegolmSessionIndex index;
        index.room_id    = room_id;
        index.session_id = session_id;
        index.sender_key = sender_key;

        try {
                auto megolm_session = olm::client()->init_inbound_group_session(session_key);
                cache::saveInboundMegolmSession(index, std::move(megolm_session));
        } catch (const lmdb::error &e) {
                nhlog::crypto()->critical("failed to save inbound megolm session: {}", e.what());
                return;
        } catch (const mtx::crypto::olm_exception &e) {
                nhlog::crypto()->critical("failed to create inbound megolm session: {}", e.what());
                return;
        }

        nhlog::crypto()->info("established inbound megolm session ({}, {})", room_id, sender);
}

void
mark_keys_as_published()
{
        olm::client()->mark_keys_as_published();
        cache::saveOlmAccount(olm::client()->save(STORAGE_SECRET_KEY));
}

void
request_keys(const std::string &room_id, const std::string &event_id)
{
        nhlog::crypto()->info("requesting keys for event {} at {}", event_id, room_id);

        http::client()->get_event(
          room_id,
          event_id,
          [event_id, room_id](const mtx::events::collections::TimelineEvents &res,
                              mtx::http::RequestErr err) {
                  using namespace mtx::events;

                  if (err) {
                          nhlog::net()->warn(
                            "failed to retrieve event {} from {}", event_id, room_id);
                          return;
                  }

                  if (!std::holds_alternative<EncryptedEvent<msg::Encrypted>>(res)) {
                          nhlog::net()->info(
                            "retrieved event is not encrypted: {} from {}", event_id, room_id);
                          return;
                  }

                  olm::send_key_request_for(room_id, std::get<EncryptedEvent<msg::Encrypted>>(res));
          });
}

void
send_key_request_for(const std::string &room_id,
                     const mtx::events::EncryptedEvent<mtx::events::msg::Encrypted> &e)
{
        using namespace mtx::events;

        nhlog::crypto()->debug("sending key request: {}", json(e).dump(2));
        auto payload = json{{"action", "request"},
                            {"request_id", http::client()->generate_txn_id()},
                            {"requesting_device_id", http::client()->device_id()},
                            {"body",
                             {{"algorithm", MEGOLM_ALGO},
                              {"room_id", room_id},
                              {"sender_key", e.content.sender_key},
                              {"session_id", e.content.session_id}}}};

        json body;
        body["messages"][e.sender]                      = json::object();
        body["messages"][e.sender][e.content.device_id] = payload;

        nhlog::crypto()->debug("m.room_key_request: {}", body.dump(2));

        http::client()->send_to_device("m.room_key_request", body, [e](mtx::http::RequestErr err) {
                if (err) {
                        nhlog::net()->warn("failed to send "
                                           "send_to_device "
                                           "message: {}",
                                           err->matrix_error.error);
                }

                nhlog::net()->info(
                  "m.room_key_request sent to {}:{}", e.sender, e.content.device_id);
        });
}

void
handle_key_request_message(const mtx::events::DeviceEvent<mtx::events::msg::KeyRequest> &req)
{
        if (req.content.algorithm != MEGOLM_ALGO) {
                nhlog::crypto()->debug("ignoring key request {} with invalid algorithm: {}",
                                       req.content.request_id,
                                       req.content.algorithm);
                return;
        }

        // Check if we were the sender of the session being requested.
        if (req.content.sender_key != olm::client()->identity_keys().curve25519) {
                nhlog::crypto()->debug("ignoring key request {} because we were not the sender: "
                                       "\nrequested({}) ours({})",
                                       req.content.request_id,
                                       req.content.sender_key,
                                       olm::client()->identity_keys().curve25519);
                return;
        }

        // Check if we have the keys for the requested session.
        if (!cache::outboundMegolmSessionExists(req.content.room_id)) {
                nhlog::crypto()->warn("requested session not found in room: {}",
                                      req.content.room_id);

                nhlog::crypto()->warn("requested session not found in room: {}",
                                      req.content.room_id);

                return;
        }

        // Check that the requested session_id and the one we have saved match.
        const auto session = cache::getOutboundMegolmSession(req.content.room_id);
        if (req.content.session_id != session.data.session_id) {
                nhlog::crypto()->warn("session id of retrieved session doesn't match the request: "
                                      "requested({}), ours({})",
                                      req.content.session_id,
                                      session.data.session_id);
                return;
        }

        if (!cache::isRoomMember(req.sender, req.content.room_id)) {
                nhlog::crypto()->warn(
                  "user {} that requested the session key is not member of the room {}",
                  req.sender,
                  req.content.room_id);
                return;
        }

        if (!utils::respondsToKeyRequests(req.content.room_id)) {
                nhlog::crypto()->debug("ignoring all key requests for room {}",
                                       req.content.room_id);

                nhlog::crypto()->debug("ignoring all key requests for room {}",
                                       req.content.room_id);

                nhlog::crypto()->debug("ignoring all key requests for room {}",
                                       req.content.room_id);
                return;
        }

        //
        // Prepare the m.room_key event.
        //
        auto payload = json{{"algorithm", "m.megolm.v1.aes-sha2"},
                            {"room_id", req.content.room_id},
                            {"session_id", req.content.session_id},
                            {"session_key", session.data.session_key}};

        send_megolm_key_to_device(req.sender, req.content.requesting_device_id, payload);
}

void
send_megolm_key_to_device(const std::string &user_id,
                          const std::string &device_id,
                          const json &payload)
{
        mtx::requests::QueryKeys req;
        req.device_keys[user_id] = {device_id};

        http::client()->query_keys(
          req,
          [payload, user_id, device_id](const mtx::responses::QueryKeys &res,
                                        mtx::http::RequestErr err) {
                  if (err) {
                          nhlog::net()->warn("failed to query device keys: {} {}",
                                             err->matrix_error.error,
                                             static_cast<int>(err->status_code));
                          return;
                  }

                  nhlog::net()->warn("retrieved device keys from {}, {}", user_id, device_id);

                  if (res.device_keys.empty()) {
                          nhlog::net()->warn("no devices retrieved {}", user_id);
                          return;
                  }

                  auto device = res.device_keys.begin()->second;
                  if (device.empty()) {
                          nhlog::net()->warn("no keys retrieved from user, device {}", user_id);
                          return;
                  }

                  const auto device_keys = device.begin()->second.keys;
                  const auto curveKey    = "curve25519:" + device_id;
                  const auto edKey       = "ed25519:" + device_id;

                  if ((device_keys.find(curveKey) == device_keys.end()) ||
                      (device_keys.find(edKey) == device_keys.end())) {
                          nhlog::net()->debug("ignoring malformed keys for device {}", device_id);
                          return;
                  }

                  DevicePublicKeys pks;
                  pks.ed25519    = device_keys.at(edKey);
                  pks.curve25519 = device_keys.at(curveKey);

                  try {
                          if (!mtx::crypto::verify_identity_signature(json(device.begin()->second),
                                                                      DeviceId(device_id),
                                                                      UserId(user_id))) {
                                  nhlog::crypto()->warn("failed to verify identity keys: {}",
                                                        json(device).dump(2));
                                  return;
                          }
                  } catch (const json::exception &e) {
                          nhlog::crypto()->warn("failed to parse device key json: {}", e.what());
                          return;
                  } catch (const mtx::crypto::olm_exception &e) {
                          nhlog::crypto()->warn("failed to verify device key json: {}", e.what());
                          return;
                  }

                  auto room_key = olm::client()
                                    ->create_room_key_event(UserId(user_id), pks.ed25519, payload)
                                    .dump();

                  http::client()->claim_keys(
                    user_id,
                    {device_id},
                    [room_key, user_id, device_id, pks](const mtx::responses::ClaimKeys &res,
                                                        mtx::http::RequestErr err) {
                            if (err) {
                                    nhlog::net()->warn("claim keys error: {} {} {}",
                                                       err->matrix_error.error,
                                                       err->parse_error,
                                                       static_cast<int>(err->status_code));
                                    return;
                            }

                            nhlog::net()->info("claimed keys for {}", user_id);

                            if (res.one_time_keys.size() == 0) {
                                    nhlog::net()->info("no one-time keys found for user_id: {}",
                                                       user_id);
                                    return;
                            }

                            if (res.one_time_keys.find(user_id) == res.one_time_keys.end()) {
                                    nhlog::net()->info("no one-time keys found for user_id: {}",
                                                       user_id);
                                    return;
                            }

                            auto retrieved_devices = res.one_time_keys.at(user_id);
                            if (retrieved_devices.empty()) {
                                    nhlog::net()->info("claiming keys for {}: no retrieved devices",
                                                       device_id);
                                    return;
                            }

                            json body;
                            body["messages"][user_id] = json::object();

                            auto device = retrieved_devices.begin()->second;
                            nhlog::net()->debug("{} : \n {}", device_id, device.dump(2));

                            json device_msg;

                            try {
                                    auto olm_session = olm::client()->create_outbound_session(
                                      pks.curve25519, device.begin()->at("key"));

                                    device_msg = olm::client()->create_olm_encrypted_content(
                                      olm_session.get(), room_key, pks.curve25519);

                                    cache::saveOlmSession(pks.curve25519, std::move(olm_session));
                            } catch (const json::exception &e) {
                                    nhlog::crypto()->warn("creating outbound session: {}",
                                                          e.what());
                                    return;
                            } catch (const mtx::crypto::olm_exception &e) {
                                    nhlog::crypto()->warn("creating outbound session: {}",
                                                          e.what());
                                    return;
                            }

                            body["messages"][user_id][device_id] = device_msg;

                            nhlog::net()->info(
                              "sending m.room_key event to {}:{}", user_id, device_id);
                            http::client()->send_to_device(
                              "m.room.encrypted", body, [user_id](mtx::http::RequestErr err) {
                                      if (err) {
                                              nhlog::net()->warn("failed to send "
                                                                 "send_to_device "
                                                                 "message: {}",
                                                                 err->matrix_error.error);
                                      }

                                      nhlog::net()->info("m.room_key send to {}", user_id);
                              });
                    });
          });
}

} // namespace olm
