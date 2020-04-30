#include "Olm.h"

#include <QObject>
#include <nlohmann/json.hpp>
#include <variant>

#include "Cache.h"
#include "Cache_p.h"
#include "ChatPage.h"
#include "DeviceVerificationFlow.h"
#include "Logging.h"
#include "MatrixClient.h"
#include "UserSettingsPage.h"
#include "Utils.h"

static const std::string STORAGE_SECRET_KEY("secret");
constexpr auto MEGOLM_ALGO = "m.megolm.v1.aes-sha2";

namespace {
auto client_ = std::make_unique<mtx::crypto::OlmClient>();
}

namespace olm {
void
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
                                olm::OlmMessage olm_msg = j_msg;
                                handle_olm_message(std::move(olm_msg));
                        } catch (const nlohmann::json::exception &e) {
                                nhlog::crypto()->warn(
                                  "parsing error for olm message: {} {}", e.what(), j_msg.dump(2));
                        } catch (const std::invalid_argument &e) {
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
                        auto message = std::get<
                          mtx::events::DeviceEvent<mtx::events::msg::KeyVerificationAccept>>(msg);
                        ChatPage::instance()->receivedDeviceVerificationAccept(message.content);
                } else if (msg_type == to_string(mtx::events::EventType::KeyVerificationRequest)) {
                        auto message = std::get<
                          mtx::events::DeviceEvent<mtx::events::msg::KeyVerificationRequest>>(msg);
                        ChatPage::instance()->receivedDeviceVerificationRequest(message.content,
                                                                                message.sender);
                } else if (msg_type == to_string(mtx::events::EventType::KeyVerificationCancel)) {
                        auto message = std::get<
                          mtx::events::DeviceEvent<mtx::events::msg::KeyVerificationCancel>>(msg);
                        ChatPage::instance()->receivedDeviceVerificationCancel(message.content);
                } else if (msg_type == to_string(mtx::events::EventType::KeyVerificationKey)) {
                        auto message =
                          std::get<mtx::events::DeviceEvent<mtx::events::msg::KeyVerificationKey>>(
                            msg);
                        ChatPage::instance()->receivedDeviceVerificationKey(message.content);
                } else if (msg_type == to_string(mtx::events::EventType::KeyVerificationMac)) {
                        auto message =
                          std::get<mtx::events::DeviceEvent<mtx::events::msg::KeyVerificationMac>>(
                            msg);
                        ChatPage::instance()->receivedDeviceVerificationMac(message.content);
                } else if (msg_type == to_string(mtx::events::EventType::KeyVerificationStart)) {
                        auto message = std::get<
                          mtx::events::DeviceEvent<mtx::events::msg::KeyVerificationStart>>(msg);
                        ChatPage::instance()->receivedDeviceVerificationStart(message.content,
                                                                              message.sender);
                } else if (msg_type == to_string(mtx::events::EventType::KeyVerificationReady)) {
                        auto message = std::get<
                          mtx::events::DeviceEvent<mtx::events::msg::KeyVerificationReady>>(msg);
                        ChatPage::instance()->receivedDeviceVerificationReady(message.content);
                } else if (msg_type == to_string(mtx::events::EventType::KeyVerificationDone)) {
                        auto message =
                          std::get<mtx::events::DeviceEvent<mtx::events::msg::KeyVerificationDone>>(
                            msg);
                        ChatPage::instance()->receivedDeviceVerificationDone(message.content);
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

                if (payload.is_null()) {
                        // Check for PRE_KEY message
                        if (cipher.second.type == 0) {
                                payload = handle_pre_key_olm_message(
                                  msg.sender, msg.sender_key, cipher.second);
                        } else {
                                nhlog::crypto()->error("Undecryptable olm message!");
                                continue;
                        }
                }

                if (!payload.is_null()) {
                        std::string msg_type = payload["type"];

                        if (msg_type == to_string(mtx::events::EventType::KeyVerificationAccept)) {
                                ChatPage::instance()->receivedDeviceVerificationAccept(
                                  payload["content"]);
                                return;
                        } else if (msg_type ==
                                   to_string(mtx::events::EventType::KeyVerificationRequest)) {
                                ChatPage::instance()->receivedDeviceVerificationRequest(
                                  payload["content"], payload["sender"]);
                                return;
                        } else if (msg_type ==
                                   to_string(mtx::events::EventType::KeyVerificationCancel)) {
                                ChatPage::instance()->receivedDeviceVerificationCancel(
                                  payload["content"]);
                                return;
                        } else if (msg_type ==
                                   to_string(mtx::events::EventType::KeyVerificationKey)) {
                                ChatPage::instance()->receivedDeviceVerificationKey(
                                  payload["content"]);
                                return;
                        } else if (msg_type ==
                                   to_string(mtx::events::EventType::KeyVerificationMac)) {
                                ChatPage::instance()->receivedDeviceVerificationMac(
                                  payload["content"]);
                                return;
                        } else if (msg_type ==
                                   to_string(mtx::events::EventType::KeyVerificationStart)) {
                                ChatPage::instance()->receivedDeviceVerificationStart(
                                  payload["content"], payload["sender"]);
                                return;
                        } else if (msg_type ==
                                   to_string(mtx::events::EventType::KeyVerificationReady)) {
                                ChatPage::instance()->receivedDeviceVerificationReady(
                                  payload["content"]);
                                return;
                        } else if (msg_type ==
                                   to_string(mtx::events::EventType::KeyVerificationDone)) {
                                ChatPage::instance()->receivedDeviceVerificationDone(
                                  payload["content"]);
                                return;
                        } else if (msg_type == to_string(mtx::events::EventType::RoomKey)) {
                                mtx::events::DeviceEvent<mtx::events::msg::RoomKey> roomKey =
                                  payload;
                                create_inbound_megolm_session(roomKey, msg.sender_key);
                                return;
                        } else if (msg_type ==
                                   to_string(mtx::events::EventType::ForwardedRoomKey)) {
                                mtx::events::DeviceEvent<mtx::events::msg::ForwardedRoomKey>
                                  roomKey = payload;
                                import_inbound_megolm_session(roomKey);
                                return;
                        }
                }
        }
}

nlohmann::json
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
                return {};
        }

        if (!mtx::crypto::matches_inbound_session_from(
              inbound_session.get(), sender_key, content.body)) {
                nhlog::crypto()->warn("inbound olm session doesn't match sender's key ({})",
                                      sender);
                return {};
        }

        mtx::crypto::BinaryBuf output;
        try {
                output =
                  olm::client()->decrypt_message(inbound_session.get(), content.type, content.body);
        } catch (const mtx::crypto::olm_exception &e) {
                nhlog::crypto()->critical(
                  "failed to decrypt olm message {}: {}", content.body, e.what());
                return {};
        }

        auto plaintext = json::parse(std::string((char *)output.data(), output.size()));
        nhlog::crypto()->debug("decrypted message: \n {}", plaintext.dump(2));

        try {
                nhlog::crypto()->debug("New olm session: {}",
                                       mtx::crypto::session_id(inbound_session.get()));
                cache::saveOlmSession(
                  sender_key, std::move(inbound_session), QDateTime::currentMSecsSinceEpoch());
        } catch (const lmdb::error &e) {
                nhlog::db()->warn(
                  "failed to save inbound olm session from {}: {}", sender, e.what());
        }

        return plaintext;
}

mtx::events::msg::Encrypted
encrypt_group_message(const std::string &room_id, const std::string &device_id, nlohmann::json body)
{
        using namespace mtx::events;

        // relations shouldn't be encrypted...
        mtx::common::ReplyRelatesTo relation;
        mtx::common::RelatesTo r_relation;

        if (body["content"].contains("m.relates_to") &&
            body["content"]["m.relates_to"].contains("m.in_reply_to")) {
                relation = body["content"]["m.relates_to"];
                body["content"].erase("m.relates_to");
        } else if (body["content"]["m.relates_to"].contains("event_id")) {
                r_relation = body["content"]["m.relates_to"];
                body["content"].erase("m.relates_to");
        }

        // Always check before for existence.
        auto res     = cache::getOutboundMegolmSession(room_id);
        auto payload = olm::client()->encrypt_group_message(res.session, body.dump());

        // Prepare the m.room.encrypted event.
        msg::Encrypted data;
        data.ciphertext   = std::string((char *)payload.data(), payload.size());
        data.sender_key   = olm::client()->identity_keys().curve25519;
        data.session_id   = res.data.session_id;
        data.device_id    = device_id;
        data.algorithm    = MEGOLM_ALGO;
        data.relates_to   = relation;
        data.r_relates_to = r_relation;

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
                        nhlog::crypto()->debug("Updated olm session: {}",
                                               mtx::crypto::session_id(session->get()));
                        cache::saveOlmSession(
                          id, std::move(session.value()), QDateTime::currentMSecsSinceEpoch());
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
                        return json::parse(std::string_view((char *)text.data(), text.size()));
                } catch (const json::exception &e) {
                        nhlog::crypto()->critical(
                          "failed to parse the decrypted session msg: {} {}",
                          e.what(),
                          std::string_view((char *)text.data(), text.size()));
                }
        }

        return {};
}

void
create_inbound_megolm_session(const mtx::events::DeviceEvent<mtx::events::msg::RoomKey> &roomKey,
                              const std::string &sender_key)
{
        MegolmSessionIndex index;
        index.room_id    = roomKey.content.room_id;
        index.session_id = roomKey.content.session_id;
        index.sender_key = sender_key;

        try {
                auto megolm_session =
                  olm::client()->init_inbound_group_session(roomKey.content.session_key);
                cache::saveInboundMegolmSession(index, std::move(megolm_session));
        } catch (const lmdb::error &e) {
                nhlog::crypto()->critical("failed to save inbound megolm session: {}", e.what());
                return;
        } catch (const mtx::crypto::olm_exception &e) {
                nhlog::crypto()->critical("failed to create inbound megolm session: {}", e.what());
                return;
        }

        nhlog::crypto()->info(
          "established inbound megolm session ({}, {})", roomKey.content.room_id, roomKey.sender);

        ChatPage::instance()->receivedSessionKey(index.room_id, index.session_id);
}

void
import_inbound_megolm_session(
  const mtx::events::DeviceEvent<mtx::events::msg::ForwardedRoomKey> &roomKey)
{
        MegolmSessionIndex index;
        index.room_id    = roomKey.content.room_id;
        index.session_id = roomKey.content.session_id;
        index.sender_key = roomKey.content.sender_key;

        try {
                auto megolm_session =
                  olm::client()->import_inbound_group_session(roomKey.content.session_key);
                cache::saveInboundMegolmSession(index, std::move(megolm_session));
        } catch (const lmdb::error &e) {
                nhlog::crypto()->critical("failed to save inbound megolm session: {}", e.what());
                return;
        } catch (const mtx::crypto::olm_exception &e) {
                nhlog::crypto()->critical("failed to import inbound megolm session: {}", e.what());
                return;
        }

        nhlog::crypto()->info(
          "established inbound megolm session ({}, {})", roomKey.content.room_id, roomKey.sender);

        ChatPage::instance()->receivedSessionKey(index.room_id, index.session_id);
}

void
mark_keys_as_published()
{
        olm::client()->mark_keys_as_published();
        cache::saveOlmAccount(olm::client()->save(STORAGE_SECRET_KEY));
}

void
send_key_request_for(mtx::events::EncryptedEvent<mtx::events::msg::Encrypted> e,
                     const std::string &request_id,
                     bool cancel)
{
        using namespace mtx::events;

        nhlog::crypto()->debug("sending key request: sender_key {}, session_id {}",
                               e.content.sender_key,
                               e.content.session_id);

        mtx::events::msg::KeyRequest request;
        request.action = !cancel ? mtx::events::msg::RequestAction::Request
                                 : mtx::events::msg::RequestAction::Cancellation;
        request.algorithm            = MEGOLM_ALGO;
        request.room_id              = e.room_id;
        request.sender_key           = e.content.sender_key;
        request.session_id           = e.content.session_id;
        request.request_id           = request_id;
        request.requesting_device_id = http::client()->device_id();

        nhlog::crypto()->debug("m.room_key_request: {}", json(request).dump(2));

        std::map<mtx::identifiers::User, std::map<std::string, decltype(request)>> body;
        body[mtx::identifiers::parse<mtx::identifiers::User>(e.sender)][e.content.device_id] =
          request;
        body[http::client()->user_id()]["*"] = request;

        http::client()->send_to_device(
          http::client()->generate_txn_id(), body, [e](mtx::http::RequestErr err) {
                  if (err) {
                          nhlog::net()->warn("failed to send "
                                             "send_to_device "
                                             "message: {}",
                                             err->matrix_error.error);
                  }

                  nhlog::net()->info("m.room_key_request sent to {}:{} and your own devices",
                                     e.sender,
                                     e.content.device_id);
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

                return;
        }

        // Check that the requested session_id and the one we have saved match.
        MegolmSessionIndex index{};
        index.room_id    = req.content.room_id;
        index.session_id = req.content.session_id;
        index.sender_key = olm::client()->identity_keys().curve25519;

        const auto session = cache::getInboundMegolmSession(index);
        if (!session) {
                nhlog::crypto()->warn("No session with id {} in db", req.content.session_id);
                return;
        }

        if (!cache::isRoomMember(req.sender, req.content.room_id)) {
                nhlog::crypto()->warn(
                  "user {} that requested the session key is not member of the room {}",
                  req.sender,
                  req.content.room_id);
                return;
        }

        // check if device is verified
        auto verificationStatus = cache::verificationStatus(req.sender);
        bool verifiedDevice     = false;
        if (verificationStatus &&
            ChatPage::instance()->userSettings()->shareKeysWithTrustedUsers()) {
                for (const auto &dev : verificationStatus->verified_devices) {
                        if (dev == req.content.requesting_device_id) {
                                verifiedDevice = true;
                                nhlog::crypto()->debug("Verified device: {}", dev);
                                break;
                        }
                }
        }

        if (!utils::respondsToKeyRequests(req.content.room_id) && !verifiedDevice) {
                nhlog::crypto()->debug("ignoring all key requests for room {}",
                                       req.content.room_id);
                return;
        }

        auto session_key = mtx::crypto::export_session(session);
        //
        // Prepare the m.room_key event.
        //
        mtx::events::msg::ForwardedRoomKey forward_key{};
        forward_key.algorithm   = MEGOLM_ALGO;
        forward_key.room_id     = index.room_id;
        forward_key.session_id  = index.session_id;
        forward_key.session_key = session_key;
        forward_key.sender_key  = index.sender_key;

        // TODO(Nico): Figure out if this is correct
        forward_key.sender_claimed_ed25519_key      = olm::client()->identity_keys().ed25519;
        forward_key.forwarding_curve25519_key_chain = {};

        send_megolm_key_to_device(req.sender, req.content.requesting_device_id, forward_key);
}

void
send_megolm_key_to_device(const std::string &user_id,
                          const std::string &device_id,
                          const mtx::events::msg::ForwardedRoomKey &payload)
{
        mtx::events::DeviceEvent<mtx::events::msg::ForwardedRoomKey> room_key;
        room_key.content = payload;
        room_key.type    = mtx::events::EventType::ForwardedRoomKey;

        std::map<std::string, std::vector<std::string>> targets;
        targets[user_id] = {device_id};
        send_encrypted_to_device_messages(targets, room_key);
}

DecryptionResult
decryptEvent(const MegolmSessionIndex &index,
             const mtx::events::EncryptedEvent<mtx::events::msg::Encrypted> &event)
{
        try {
                if (!cache::client()->inboundMegolmSessionExists(index)) {
                        return {DecryptionErrorCode::MissingSession, std::nullopt, std::nullopt};
                }
        } catch (const lmdb::error &e) {
                return {DecryptionErrorCode::DbError, e.what(), std::nullopt};
        }

        // TODO: Lookup index,event_id,origin_server_ts tuple for replay attack errors
        // TODO: Verify sender_key

        std::string msg_str;
        try {
                auto session = cache::client()->getInboundMegolmSession(index);
                auto res = olm::client()->decrypt_group_message(session, event.content.ciphertext);
                msg_str  = std::string((char *)res.data.data(), res.data.size());
        } catch (const lmdb::error &e) {
                return {DecryptionErrorCode::DbError, e.what(), std::nullopt};
        } catch (const mtx::crypto::olm_exception &e) {
                return {DecryptionErrorCode::DecryptionFailed, e.what(), std::nullopt};
        }

        // Add missing fields for the event.
        json body                = json::parse(msg_str);
        body["event_id"]         = event.event_id;
        body["sender"]           = event.sender;
        body["origin_server_ts"] = event.origin_server_ts;
        body["unsigned"]         = event.unsigned_data;

        // relations are unencrypted in content...
        if (json old_ev = event; old_ev["content"].count("m.relates_to") != 0)
                body["content"]["m.relates_to"] = old_ev["content"]["m.relates_to"];

        mtx::events::collections::TimelineEvent te;
        try {
                mtx::events::collections::from_json(body, te);
        } catch (std::exception &e) {
                return {DecryptionErrorCode::ParsingFailed, e.what(), std::nullopt};
        }

        return {std::nullopt, std::nullopt, std::move(te.data)};
}

//! Send encrypted to device messages, targets is a map from userid to device ids or {} for all
//! devices
void
send_encrypted_to_device_messages(const std::map<std::string, std::vector<std::string>> targets,
                                  const mtx::events::collections::DeviceEvents &event,
                                  bool force_new_session)
{
        nlohmann::json ev_json = std::visit([](const auto &e) { return json(e); }, event);

        std::map<std::string, std::vector<std::string>> keysToQuery;
        mtx::requests::ClaimKeys claims;
        std::map<mtx::identifiers::User, std::map<std::string, mtx::events::msg::OlmEncrypted>>
          messages;
        std::map<std::string, std::map<std::string, DevicePublicKeys>> pks;

        for (const auto &[user, devices] : targets) {
                auto deviceKeys = cache::client()->userKeys(user);

                // no keys for user, query them
                if (!deviceKeys) {
                        keysToQuery[user] = devices;
                        continue;
                }

                auto deviceTargets = devices;
                if (devices.empty()) {
                        deviceTargets.clear();
                        for (const auto &[device, keys] : deviceKeys->device_keys) {
                                (void)keys;
                                deviceTargets.push_back(device);
                        }
                }

                for (const auto &device : deviceTargets) {
                        if (!deviceKeys->device_keys.count(device)) {
                                keysToQuery[user] = {};
                                break;
                        }

                        auto d = deviceKeys->device_keys.at(device);

                        auto session =
                          cache::getLatestOlmSession(d.keys.at("curve25519:" + device));
                        if (!session || force_new_session) {
                                claims.one_time_keys[user][device] = mtx::crypto::SIGNED_CURVE25519;
                                pks[user][device].ed25519          = d.keys.at("ed25519:" + device);
                                pks[user][device].curve25519 = d.keys.at("curve25519:" + device);
                                continue;
                        }

                        messages[mtx::identifiers::parse<mtx::identifiers::User>(user)][device] =
                          olm::client()
                            ->create_olm_encrypted_content(session->get(),
                                                           ev_json,
                                                           UserId(user),
                                                           d.keys.at("ed25519:" + device),
                                                           d.keys.at("curve25519:" + device))
                            .get<mtx::events::msg::OlmEncrypted>();

                        try {
                                nhlog::crypto()->debug("Updated olm session: {}",
                                                       mtx::crypto::session_id(session->get()));
                                cache::saveOlmSession(d.keys.at("curve25519:" + device),
                                                      std::move(*session),
                                                      QDateTime::currentMSecsSinceEpoch());
                        } catch (const lmdb::error &e) {
                                nhlog::db()->critical("failed to save outbound olm session: {}",
                                                      e.what());
                        } catch (const mtx::crypto::olm_exception &e) {
                                nhlog::crypto()->critical(
                                  "failed to pickle outbound olm session: {}", e.what());
                        }
                }
        }

        if (!messages.empty())
                http::client()->send_to_device<mtx::events::msg::OlmEncrypted>(
                  http::client()->generate_txn_id(), messages, [](mtx::http::RequestErr err) {
                          if (err) {
                                  nhlog::net()->warn("failed to send "
                                                     "send_to_device "
                                                     "message: {}",
                                                     err->matrix_error.error);
                          }
                  });

        auto BindPks = [ev_json](decltype(pks) pks_temp) {
                return [pks = pks_temp, ev_json](const mtx::responses::ClaimKeys &res,
                                                 mtx::http::RequestErr) {
                        std::map<mtx::identifiers::User,
                                 std::map<std::string, mtx::events::msg::OlmEncrypted>>
                          messages;
                        for (const auto &[user_id, retrieved_devices] : res.one_time_keys) {
                                nhlog::net()->debug("claimed keys for {}", user_id);
                                if (retrieved_devices.size() == 0) {
                                        nhlog::net()->debug(
                                          "no one-time keys found for user_id: {}", user_id);
                                        continue;
                                }

                                for (const auto &rd : retrieved_devices) {
                                        const auto device_id = rd.first;

                                        nhlog::net()->debug(
                                          "{} : \n {}", device_id, rd.second.dump(2));

                                        if (rd.second.empty() ||
                                            !rd.second.begin()->contains("key")) {
                                                nhlog::net()->warn(
                                                  "Skipping device {} as it has no key.",
                                                  device_id);
                                                continue;
                                        }

                                        // TODO: Verify signatures
                                        auto otk = rd.second.begin()->at("key");

                                        auto id_key = pks.at(user_id).at(device_id).curve25519;
                                        auto session =
                                          olm::client()->create_outbound_session(id_key, otk);

                                        messages[mtx::identifiers::parse<mtx::identifiers::User>(
                                          user_id)][device_id] =
                                          olm::client()
                                            ->create_olm_encrypted_content(
                                              session.get(),
                                              ev_json,
                                              UserId(user_id),
                                              pks.at(user_id).at(device_id).ed25519,
                                              id_key)
                                            .get<mtx::events::msg::OlmEncrypted>();

                                        try {
                                                nhlog::crypto()->debug(
                                                  "Updated olm session: {}",
                                                  mtx::crypto::session_id(session.get()));
                                                cache::saveOlmSession(
                                                  id_key,
                                                  std::move(session),
                                                  QDateTime::currentMSecsSinceEpoch());
                                        } catch (const lmdb::error &e) {
                                                nhlog::db()->critical(
                                                  "failed to save outbound olm session: {}",
                                                  e.what());
                                        } catch (const mtx::crypto::olm_exception &e) {
                                                nhlog::crypto()->critical(
                                                  "failed to pickle outbound olm session: {}",
                                                  e.what());
                                        }
                                }
                                nhlog::net()->info("send_to_device: {}", user_id);
                        }

                        if (!messages.empty())
                                http::client()->send_to_device<mtx::events::msg::OlmEncrypted>(
                                  http::client()->generate_txn_id(),
                                  messages,
                                  [](mtx::http::RequestErr err) {
                                          if (err) {
                                                  nhlog::net()->warn("failed to send "
                                                                     "send_to_device "
                                                                     "message: {}",
                                                                     err->matrix_error.error);
                                          }
                                  });
                };
        };

        http::client()->claim_keys(claims, BindPks(pks));

        if (!keysToQuery.empty()) {
                mtx::requests::QueryKeys req;
                req.device_keys = keysToQuery;
                http::client()->query_keys(
                  req,
                  [ev_json, BindPks](const mtx::responses::QueryKeys &res,
                                     mtx::http::RequestErr err) {
                          if (err) {
                                  nhlog::net()->warn("failed to query device keys: {} {}",
                                                     err->matrix_error.error,
                                                     static_cast<int>(err->status_code));
                                  return;
                          }

                          nhlog::net()->info("queried keys");

                          cache::client()->updateUserKeys(cache::nextBatchToken(), res);

                          mtx::requests::ClaimKeys claim_keys;

                          std::map<std::string, std::map<std::string, DevicePublicKeys>> deviceKeys;

                          for (const auto &user : res.device_keys) {
                                  for (const auto &dev : user.second) {
                                          const auto user_id   = ::UserId(dev.second.user_id);
                                          const auto device_id = DeviceId(dev.second.device_id);

                                          if (user_id.get() ==
                                                http::client()->user_id().to_string() &&
                                              device_id.get() == http::client()->device_id())
                                                  continue;

                                          const auto device_keys = dev.second.keys;
                                          const auto curveKey    = "curve25519:" + device_id.get();
                                          const auto edKey       = "ed25519:" + device_id.get();

                                          if ((device_keys.find(curveKey) == device_keys.end()) ||
                                              (device_keys.find(edKey) == device_keys.end())) {
                                                  nhlog::net()->debug(
                                                    "ignoring malformed keys for device {}",
                                                    device_id.get());
                                                  continue;
                                          }

                                          DevicePublicKeys pks;
                                          pks.ed25519    = device_keys.at(edKey);
                                          pks.curve25519 = device_keys.at(curveKey);

                                          try {
                                                  if (!mtx::crypto::verify_identity_signature(
                                                        dev.second, device_id, user_id)) {
                                                          nhlog::crypto()->warn(
                                                            "failed to verify identity keys: {}",
                                                            json(dev.second).dump(2));
                                                          continue;
                                                  }
                                          } catch (const json::exception &e) {
                                                  nhlog::crypto()->warn(
                                                    "failed to parse device key json: {}",
                                                    e.what());
                                                  continue;
                                          } catch (const mtx::crypto::olm_exception &e) {
                                                  nhlog::crypto()->warn(
                                                    "failed to verify device key json: {}",
                                                    e.what());
                                                  continue;
                                          }

                                          deviceKeys[user_id].emplace(device_id, pks);
                                          claim_keys.one_time_keys[user.first][device_id] =
                                            mtx::crypto::SIGNED_CURVE25519;

                                          nhlog::net()->info("{}", device_id.get());
                                          nhlog::net()->info("  curve25519 {}", pks.curve25519);
                                          nhlog::net()->info("  ed25519 {}", pks.ed25519);
                                  }
                          }

                          http::client()->claim_keys(claim_keys, BindPks(deviceKeys));
                  });
        }
}

} // namespace olm
