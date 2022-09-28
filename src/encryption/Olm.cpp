// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Olm.h"

#include <QObject>
#include <QTimer>

#include <nlohmann/json.hpp>
#include <variant>

#include <mtx/responses/common.hpp>
#include <mtx/secret_storage.hpp>

#include "Cache.h"
#include "Cache_p.h"
#include "ChatPage.h"
#include "DeviceVerificationFlow.h"
#include "EventAccessors.h"
#include "Logging.h"
#include "MatrixClient.h"
#include "UserSettingsPage.h"
#include "Utils.h"

namespace {
auto client_ = std::make_unique<mtx::crypto::OlmClient>();

std::map<std::string, std::string> request_id_to_secret_name;

constexpr auto MEGOLM_ALGO = "m.megolm.v1.aes-sha2";
}

namespace olm {
static void
backup_session_key(const MegolmSessionIndex &idx,
                   const GroupSessionData &data,
                   mtx::crypto::InboundGroupSessionPtr &session);

void
from_json(const nlohmann::json &obj, OlmMessage &msg)
{
    if (obj.at("type") != "m.room.encrypted")
        throw std::invalid_argument("invalid type for olm message");

    if (obj.at("content").at("algorithm") != OLM_ALGO)
        throw std::invalid_argument("invalid algorithm for olm message");

    msg.sender     = obj.at("sender").get<std::string>();
    msg.sender_key = obj.at("content").at("sender_key").get<std::string>();
    msg.ciphertext = obj.at("content")
                       .at("ciphertext")
                       .get<std::map<std::string, mtx::events::msg::OlmCipherContent>>();
}

mtx::crypto::OlmClient *
client()
{
    return client_.get();
}

static void
handle_secret_request(const mtx::events::DeviceEvent<mtx::events::msg::SecretRequest> *e,
                      const std::string &sender)
{
    using namespace mtx::events;

    if (e->content.action != mtx::events::msg::RequestAction::Request)
        return;

    auto local_user = http::client()->user_id();

    if (sender != local_user.to_string())
        return;

    auto verificationStatus = cache::verificationStatus(local_user.to_string());

    if (!verificationStatus)
        return;

    auto deviceKeys = cache::userKeys(local_user.to_string());
    if (!deviceKeys)
        return;

    if (std::find(verificationStatus->verified_devices.begin(),
                  verificationStatus->verified_devices.end(),
                  e->content.requesting_device_id) == verificationStatus->verified_devices.end())
        return;

    // this is a verified device
    mtx::events::DeviceEvent<mtx::events::msg::SecretSend> secretSend;
    secretSend.type               = EventType::SecretSend;
    secretSend.content.request_id = e->content.request_id;

    auto secret = cache::client()->secret(e->content.name);
    if (!secret)
        return;
    secretSend.content.secret = secret.value();

    send_encrypted_to_device_messages(
      {{local_user.to_string(), {{e->content.requesting_device_id}}}}, secretSend);

    nhlog::net()->info("Sent secret '{}' to ({},{})",
                       e->content.name,
                       local_user.to_string(),
                       e->content.requesting_device_id);
}

void
handle_to_device_messages(const std::vector<mtx::events::collections::DeviceEvents> &msgs)
{
    if (msgs.empty())
        return;
    nhlog::crypto()->info("received {} to_device messages", msgs.size());
    nlohmann::json j_msg;

    for (const auto &msg : msgs) {
        j_msg = std::visit([](auto &e) { return nlohmann::json(e); }, std::move(msg));
        if (j_msg.count("type") == 0) {
            nhlog::crypto()->warn("received message with no type field: {}", j_msg.dump(2));
            continue;
        }

        std::string msg_type = j_msg.at("type").get<std::string>();

        if (msg_type == to_string(mtx::events::EventType::RoomEncrypted)) {
            try {
                olm::OlmMessage olm_msg = j_msg.get<olm::OlmMessage>();
                cache::client()->query_keys(
                  olm_msg.sender, [olm_msg](const UserKeyCache &userKeys, mtx::http::RequestErr e) {
                      if (e) {
                          nhlog::crypto()->error("Failed to query user keys, dropping olm "
                                                 "message");
                          return;
                      }
                      handle_olm_message(std::move(olm_msg), userKeys);
                  });
            } catch (const nlohmann::json::exception &e) {
                nhlog::crypto()->warn(
                  "parsing error for olm message: {} {}", e.what(), j_msg.dump(2));
            } catch (const std::invalid_argument &e) {
                nhlog::crypto()->warn(
                  "validation error for olm message: {} {}", e.what(), j_msg.dump(2));
            }

        } else if (msg_type == to_string(mtx::events::EventType::RoomKeyRequest)) {
            nhlog::crypto()->warn("handling key request event: {}", j_msg.dump(2));
            try {
                mtx::events::DeviceEvent<mtx::events::msg::KeyRequest> req =
                  j_msg.get<mtx::events::DeviceEvent<mtx::events::msg::KeyRequest>>();
                if (req.content.action == mtx::events::msg::RequestAction::Request)
                    handle_key_request_message(req);
                else
                    nhlog::crypto()->warn("ignore key request (unhandled action): {}",
                                          req.content.request_id);
            } catch (const nlohmann::json::exception &e) {
                nhlog::crypto()->warn(
                  "parsing error for key_request message: {} {}", e.what(), j_msg.dump(2));
            }
        } else if (msg_type == to_string(mtx::events::EventType::KeyVerificationAccept)) {
            auto message =
              std::get<mtx::events::DeviceEvent<mtx::events::msg::KeyVerificationAccept>>(msg);
            ChatPage::instance()->receivedDeviceVerificationAccept(message.content);
        } else if (msg_type == to_string(mtx::events::EventType::KeyVerificationRequest)) {
            auto message =
              std::get<mtx::events::DeviceEvent<mtx::events::msg::KeyVerificationRequest>>(msg);
            ChatPage::instance()->receivedDeviceVerificationRequest(message.content,
                                                                    message.sender);
        } else if (msg_type == to_string(mtx::events::EventType::KeyVerificationCancel)) {
            auto message =
              std::get<mtx::events::DeviceEvent<mtx::events::msg::KeyVerificationCancel>>(msg);
            ChatPage::instance()->receivedDeviceVerificationCancel(message.content);
        } else if (msg_type == to_string(mtx::events::EventType::KeyVerificationKey)) {
            auto message =
              std::get<mtx::events::DeviceEvent<mtx::events::msg::KeyVerificationKey>>(msg);
            ChatPage::instance()->receivedDeviceVerificationKey(message.content);
        } else if (msg_type == to_string(mtx::events::EventType::KeyVerificationMac)) {
            auto message =
              std::get<mtx::events::DeviceEvent<mtx::events::msg::KeyVerificationMac>>(msg);
            ChatPage::instance()->receivedDeviceVerificationMac(message.content);
        } else if (msg_type == to_string(mtx::events::EventType::KeyVerificationStart)) {
            auto message =
              std::get<mtx::events::DeviceEvent<mtx::events::msg::KeyVerificationStart>>(msg);
            ChatPage::instance()->receivedDeviceVerificationStart(message.content, message.sender);
        } else if (msg_type == to_string(mtx::events::EventType::KeyVerificationReady)) {
            auto message =
              std::get<mtx::events::DeviceEvent<mtx::events::msg::KeyVerificationReady>>(msg);
            ChatPage::instance()->receivedDeviceVerificationReady(message.content);
        } else if (msg_type == to_string(mtx::events::EventType::KeyVerificationDone)) {
            auto message =
              std::get<mtx::events::DeviceEvent<mtx::events::msg::KeyVerificationDone>>(msg);
            ChatPage::instance()->receivedDeviceVerificationDone(message.content);
        } else if (auto e =
                     std::get_if<mtx::events::DeviceEvent<mtx::events::msg::SecretRequest>>(&msg)) {
            handle_secret_request(e, e->sender);
        } else {
            nhlog::crypto()->warn("unhandled event: {}", j_msg.dump(2));
        }
    }
}

void
handle_olm_message(const OlmMessage &msg, const UserKeyCache &otherUserDeviceKeys)
{
    nhlog::crypto()->info("sender    : {}", msg.sender);
    nhlog::crypto()->info("sender_key: {}", msg.sender_key);

    if (msg.sender_key == olm::client()->identity_keys().ed25519) {
        nhlog::crypto()->warn("Ignoring olm message from ourselves!");
        return;
    }

    const auto my_key = olm::client()->identity_keys().curve25519;

    bool failed_decryption = false;

    for (const auto &cipher : msg.ciphertext) {
        // We skip messages not meant for the current device.
        if (cipher.first != my_key) {
            nhlog::crypto()->debug(
              "Skipping message for {} since we are {}.", cipher.first, my_key);
            continue;
        }

        const auto type = cipher.second.type;
        nhlog::crypto()->info("type: {}", type == 0 ? "OLM_PRE_KEY" : "OLM_MESSAGE");

        auto payload = try_olm_decryption(msg.sender_key, cipher.second);

        if (payload.is_null()) {
            // Check for PRE_KEY message
            if (cipher.second.type == 0) {
                payload = handle_pre_key_olm_message(msg.sender, msg.sender_key, cipher.second);
            } else {
                nhlog::crypto()->error("Undecryptable olm message!");
                failed_decryption = true;
                continue;
            }
        }

        if (!payload.is_null()) {
            mtx::events::collections::DeviceEvents device_event;

            // Other properties are included in order to prevent an attacker from
            // publishing someone else's curve25519 keys as their own and subsequently
            // claiming to have sent messages which they didn't. sender must correspond
            // to the user who sent the event, recipient to the local user, and
            // recipient_keys to the local ed25519 key.
            std::string receiver_ed25519 = payload["recipient_keys"]["ed25519"].get<std::string>();
            if (receiver_ed25519.empty() ||
                receiver_ed25519 != olm::client()->identity_keys().ed25519) {
                nhlog::crypto()->warn("Decrypted event doesn't include our ed25519: {}",
                                      payload.dump());
                return;
            }
            std::string receiver = payload["recipient"].get<std::string>();
            if (receiver.empty() || receiver != http::client()->user_id().to_string()) {
                nhlog::crypto()->warn("Decrypted event doesn't include our user_id: {}",
                                      payload.dump());
                return;
            }

            // Clients must confirm that the sender_key and the ed25519 field value
            // under the keys property match the keys returned by /keys/query for the
            // given user, and must also verify the signature of the payload. Without
            // this check, a client cannot be sure that the sender device owns the
            // private part of the ed25519 key it claims to have in the Olm payload.
            // This is crucial when the ed25519 key corresponds to a verified device.
            std::string sender_ed25519 = payload["keys"]["ed25519"].get<std::string>();
            if (sender_ed25519.empty()) {
                nhlog::crypto()->warn("Decrypted event doesn't include sender ed25519: {}",
                                      payload.dump());
                return;
            }

            bool from_their_device = false;
            for (const auto &[device_id, key] : otherUserDeviceKeys.device_keys) {
                auto c_key = key.keys.find("curve25519:" + device_id);
                auto e_key = key.keys.find("ed25519:" + device_id);

                if (c_key == key.keys.end() || e_key == key.keys.end()) {
                    nhlog::crypto()->warn("Skipping device {} as we have no keys for it.",
                                          device_id);
                } else if (c_key->second == msg.sender_key && e_key->second == sender_ed25519) {
                    from_their_device = true;
                    break;
                }
            }
            if (!from_their_device) {
                nhlog::crypto()->warn("Decrypted event isn't sent from a device "
                                      "listed by that user! {}",
                                      payload.dump());
                return;
            }

            {
                std::string msg_type       = payload["type"].get<std::string>();
                nlohmann::json event_array = nlohmann::json::array();
                event_array.push_back(payload);

                std::vector<mtx::events::collections::DeviceEvents> temp_events;
                mtx::responses::utils::parse_device_events(event_array, temp_events);
                if (temp_events.empty()) {
                    nhlog::crypto()->warn("Decrypted unknown event: {}", payload.dump());
                    return;
                }
                device_event = temp_events.at(0);
            }

            using namespace mtx::events;
            if (auto e1 = std::get_if<DeviceEvent<msg::KeyVerificationAccept>>(&device_event)) {
                ChatPage::instance()->receivedDeviceVerificationAccept(e1->content);
            } else if (auto e2 =
                         std::get_if<DeviceEvent<msg::KeyVerificationRequest>>(&device_event)) {
                ChatPage::instance()->receivedDeviceVerificationRequest(e2->content, e2->sender);
            } else if (auto e3 =
                         std::get_if<DeviceEvent<msg::KeyVerificationCancel>>(&device_event)) {
                ChatPage::instance()->receivedDeviceVerificationCancel(e3->content);
            } else if (auto e4 = std::get_if<DeviceEvent<msg::KeyVerificationKey>>(&device_event)) {
                ChatPage::instance()->receivedDeviceVerificationKey(e4->content);
            } else if (auto e5 = std::get_if<DeviceEvent<msg::KeyVerificationMac>>(&device_event)) {
                ChatPage::instance()->receivedDeviceVerificationMac(e5->content);
            } else if (auto e6 =
                         std::get_if<DeviceEvent<msg::KeyVerificationStart>>(&device_event)) {
                ChatPage::instance()->receivedDeviceVerificationStart(e6->content, e6->sender);
            } else if (auto e7 =
                         std::get_if<DeviceEvent<msg::KeyVerificationReady>>(&device_event)) {
                ChatPage::instance()->receivedDeviceVerificationReady(e7->content);
            } else if (auto e8 =
                         std::get_if<DeviceEvent<msg::KeyVerificationDone>>(&device_event)) {
                ChatPage::instance()->receivedDeviceVerificationDone(e8->content);
            } else if (auto roomKey = std::get_if<DeviceEvent<msg::RoomKey>>(&device_event)) {
                create_inbound_megolm_session(*roomKey, msg.sender_key, sender_ed25519);
            } else if (auto forwardedRoomKey =
                         std::get_if<DeviceEvent<msg::ForwardedRoomKey>>(&device_event)) {
                forwardedRoomKey->content.forwarding_curve25519_key_chain.push_back(msg.sender_key);
                import_inbound_megolm_session(*forwardedRoomKey);
            } else if (auto e = std::get_if<DeviceEvent<msg::SecretSend>>(&device_event)) {
                auto local_user = http::client()->user_id();

                if (msg.sender != local_user.to_string())
                    return;

                auto secret_name_it = request_id_to_secret_name.find(e->content.request_id);

                if (secret_name_it != request_id_to_secret_name.end()) {
                    auto secret_name = secret_name_it->second;
                    request_id_to_secret_name.erase(secret_name_it);

                    nhlog::crypto()->info("Received secret: {}", secret_name);

                    mtx::events::msg::SecretRequest secretRequest{};
                    secretRequest.action = mtx::events::msg::RequestAction::Cancellation;
                    secretRequest.requesting_device_id = http::client()->device_id();
                    secretRequest.request_id           = e->content.request_id;

                    auto verificationStatus = cache::verificationStatus(local_user.to_string());

                    if (!verificationStatus)
                        return;

                    auto deviceKeys = cache::userKeys(local_user.to_string());
                    if (!deviceKeys)
                        return;

                    std::string sender_device_id;
                    for (auto &[dev, key] : deviceKeys->device_keys) {
                        if (key.keys["curve25519:" + dev] == msg.sender_key) {
                            sender_device_id = dev;
                            break;
                        }
                    }
                    if (!verificationStatus->verified_devices.count(sender_device_id) ||
                        !verificationStatus->verified_device_keys.count(msg.sender_key) ||
                        verificationStatus->verified_device_keys.at(msg.sender_key) !=
                          crypto::Trust::Verified) {
                        nhlog::net()->critical(
                          "Received secret from unverified device {}! Ignoring!", sender_device_id);
                        return;
                    }

                    std::map<mtx::identifiers::User,
                             std::map<std::string, mtx::events::msg::SecretRequest>>
                      body;

                    for (const auto &dev : verificationStatus->verified_devices) {
                        if (dev != secretRequest.requesting_device_id && dev != sender_device_id)
                            body[local_user][dev] = secretRequest;
                    }

                    http::client()->send_to_device<mtx::events::msg::SecretRequest>(
                      http::client()->generate_txn_id(),
                      body,
                      [secret_name](mtx::http::RequestErr err) {
                          if (err) {
                              nhlog::net()->error("Failed to send request cancellation "
                                                  "for secrect "
                                                  "'{}'",
                                                  secret_name);
                          }
                      });

                    nhlog::crypto()->info("Storing secret {}", secret_name);
                    cache::client()->storeSecret(secret_name, e->content.secret);
                }

            } else if (auto sec_req = std::get_if<DeviceEvent<msg::SecretRequest>>(&device_event)) {
                handle_secret_request(sec_req, msg.sender);
            }

            return;
        } else {
            failed_decryption = true;
        }
    }

    if (failed_decryption) {
        try {
            std::map<std::string, std::vector<std::string>> targets;
            for (const auto &[device_id, key] : otherUserDeviceKeys.device_keys) {
                if (key.keys.at("curve25519:" + device_id) == msg.sender_key)
                    targets[msg.sender].push_back(device_id);
            }

            send_encrypted_to_device_messages(
              targets, mtx::events::DeviceEvent<mtx::events::msg::Dummy>{}, true);
            nhlog::crypto()->info(
              "Recovering from broken olm channel with {}:{}", msg.sender, msg.sender_key);
        } catch (std::exception &e) {
            nhlog::crypto()->error("Failed to recover from broken olm sessions: {}", e.what());
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
        inbound_session = olm::client()->create_inbound_session_from(sender_key, content.body);

        // We also remove the one time key used to establish that
        // session so we'll have to update our copy of the account object.
        cache::saveOlmAccount(olm::client()->save(cache::client()->pickleSecret()));
    } catch (const mtx::crypto::olm_exception &e) {
        nhlog::crypto()->critical("failed to create inbound session with {}: {}", sender, e.what());
        return {};
    }

    if (!mtx::crypto::matches_inbound_session_from(
          inbound_session.get(), sender_key, content.body)) {
        nhlog::crypto()->warn("inbound olm session doesn't match sender's key ({})", sender);
        return {};
    }

    mtx::crypto::BinaryBuf output;
    try {
        output = olm::client()->decrypt_message(inbound_session.get(), content.type, content.body);
    } catch (const mtx::crypto::olm_exception &e) {
        nhlog::crypto()->critical("failed to decrypt olm message {}: {}", content.body, e.what());
        return {};
    }

    auto plaintext = nlohmann::json::parse(std::string((char *)output.data(), output.size()));
    nhlog::crypto()->debug("decrypted message: \n {}", plaintext.dump(2));

    try {
        nhlog::crypto()->debug("New olm session: {}",
                               mtx::crypto::session_id(inbound_session.get()));
        cache::saveOlmSession(
          sender_key, std::move(inbound_session), QDateTime::currentMSecsSinceEpoch());
    } catch (const lmdb::error &e) {
        nhlog::db()->warn("failed to save inbound olm session from {}: {}", sender, e.what());
    }

    return plaintext;
}

mtx::events::msg::Encrypted
encrypt_group_message_with_session(mtx::crypto::OutboundGroupSessionPtr &session,
                                   const std::string &device_id,
                                   nlohmann::json body)
{
    using namespace mtx::events;

    // relations shouldn't be encrypted...
    mtx::common::Relations relations = mtx::common::parse_relations(body["content"]);

    auto payload = olm::client()->encrypt_group_message(session.get(), body.dump());

    // Prepare the m.room.encrypted event.
    msg::Encrypted data;
    data.ciphertext = std::string((char *)payload.data(), payload.size());
    data.sender_key = olm::client()->identity_keys().curve25519;
    data.session_id = mtx::crypto::session_id(session.get());
    data.device_id  = device_id;
    data.algorithm  = MEGOLM_ALGO;
    data.relations  = relations;

    return data;
}

mtx::events::msg::Encrypted
encrypt_group_message(const std::string &room_id, const std::string &device_id, nlohmann::json body)
{
    using namespace mtx::events;
    using namespace mtx::identifiers;

    auto own_user_id = http::client()->user_id().to_string();

    auto members = cache::client()->getMembersWithKeys(
      room_id, UserSettings::instance()->onlyShareKeysWithVerifiedUsers());

    std::map<std::string, std::vector<std::string>> sendSessionTo;
    mtx::crypto::OutboundGroupSessionPtr session = nullptr;
    GroupSessionData group_session_data;

    if (cache::outboundMegolmSessionExists(room_id)) {
        auto res                = cache::getOutboundMegolmSession(room_id);
        auto encryptionSettings = cache::client()->roomEncryptionSettings(room_id);
        mtx::events::state::Encryption defaultSettings;

        // rotate if we crossed the limits for this key
        if (res.data.message_index <
              encryptionSettings.value_or(defaultSettings).rotation_period_msgs &&
            (QDateTime::currentMSecsSinceEpoch() - res.data.timestamp) <
              encryptionSettings.value_or(defaultSettings).rotation_period_ms) {
            auto member_it             = members.begin();
            auto session_member_it     = res.data.currently.keys.begin();
            auto session_member_it_end = res.data.currently.keys.end();

            while (member_it != members.end() || session_member_it != session_member_it_end) {
                if (member_it == members.end()) {
                    // a member left, purge session!
                    nhlog::crypto()->debug("Rotating megolm session because of left member");
                    break;
                }

                if (session_member_it == session_member_it_end) {
                    // share with all remaining members
                    while (member_it != members.end()) {
                        sendSessionTo[member_it->first] = {};

                        if (member_it->second)
                            for (const auto &dev : member_it->second->device_keys)
                                if (member_it->first != own_user_id || dev.first != device_id)
                                    sendSessionTo[member_it->first].push_back(dev.first);

                        ++member_it;
                    }

                    session = std::move(res.session);
                    break;
                }

                if (member_it->first > session_member_it->first) {
                    // a member left, purge session
                    nhlog::crypto()->debug("Rotating megolm session because of left member");
                    break;
                } else if (member_it->first < session_member_it->first) {
                    // new member, send them the session at this index
                    sendSessionTo[member_it->first] = {};

                    if (member_it->second) {
                        for (const auto &dev : member_it->second->device_keys)
                            if (member_it->first != own_user_id || dev.first != device_id)
                                sendSessionTo[member_it->first].push_back(dev.first);
                    }

                    ++member_it;
                } else {
                    // compare devices
                    bool device_removed = false;
                    for (const auto &dev : session_member_it->second.deviceids) {
                        if (!member_it->second ||
                            !member_it->second->device_keys.count(dev.first)) {
                            device_removed = true;
                            break;
                        }
                    }

                    if (device_removed) {
                        // device removed, rotate session!
                        nhlog::crypto()->debug("Rotating megolm session because of removed "
                                               "device of {}",
                                               member_it->first);
                        break;
                    }

                    // check for new devices to share with
                    if (member_it->second)
                        for (const auto &dev : member_it->second->device_keys)
                            if (!session_member_it->second.deviceids.count(dev.first) &&
                                (member_it->first != own_user_id || dev.first != device_id))
                                sendSessionTo[member_it->first].push_back(dev.first);

                    ++member_it;
                    ++session_member_it;
                    if (member_it == members.end() && session_member_it == session_member_it_end) {
                        // all devices match or are newly added
                        session = std::move(res.session);
                    }
                }
            }
        }

        group_session_data = std::move(res.data);
    }

    if (!session) {
        nhlog::ui()->debug("creating new outbound megolm session");

        // Create a new outbound megolm session.
        session                = olm::client()->init_outbound_group_session();
        const auto session_id  = mtx::crypto::session_id(session.get());
        const auto session_key = mtx::crypto::session_key(session.get());

        // Saving the new megolm session.
        GroupSessionData session_data{};
        session_data.message_index              = 0;
        session_data.timestamp                  = QDateTime::currentMSecsSinceEpoch();
        session_data.sender_claimed_ed25519_key = olm::client()->identity_keys().ed25519;
        session_data.sender_key                 = olm::client()->identity_keys().curve25519;

        sendSessionTo.clear();

        for (const auto &[user, devices] : members) {
            sendSessionTo[user]               = {};
            session_data.currently.keys[user] = {};
            if (devices) {
                for (const auto &[device_id_, key] : devices->device_keys) {
                    (void)key;
                    if (device_id != device_id_ || user != own_user_id) {
                        sendSessionTo[user].push_back(device_id_);
                        session_data.currently.keys[user].deviceids[device_id_] = 0;
                    }
                }
            }
        }

        {
            MegolmSessionIndex index;
            index.room_id       = room_id;
            index.session_id    = session_id;
            auto megolm_session = olm::client()->init_inbound_group_session(session_key);
            backup_session_key(index, session_data, megolm_session);
            cache::saveInboundMegolmSession(index, std::move(megolm_session), session_data);
        }

        cache::saveOutboundMegolmSession(room_id, session_data, session);
        group_session_data = std::move(session_data);
    }

    mtx::events::DeviceEvent<mtx::events::msg::RoomKey> megolm_payload{};
    megolm_payload.content.algorithm   = MEGOLM_ALGO;
    megolm_payload.content.room_id     = room_id;
    megolm_payload.content.session_id  = mtx::crypto::session_id(session.get());
    megolm_payload.content.session_key = mtx::crypto::session_key(session.get());
    megolm_payload.type                = mtx::events::EventType::RoomKey;

    if (!sendSessionTo.empty())
        olm::send_encrypted_to_device_messages(sendSessionTo, megolm_payload);

    auto data = encrypt_group_message_with_session(session, device_id, body);

    group_session_data.message_index = olm_outbound_group_session_message_index(session.get());
    nhlog::crypto()->debug("next message_index {}", group_session_data.message_index);

    // update current set of members for the session with the new members and that message_index
    for (const auto &[user, devices] : sendSessionTo) {
        if (!group_session_data.currently.keys.count(user))
            group_session_data.currently.keys[user] = {};

        for (const auto &device_id_ : devices) {
            if (!group_session_data.currently.keys[user].deviceids.count(device_id_))
                group_session_data.currently.keys[user].deviceids[device_id_] =
                  group_session_data.message_index;
        }
    }

    // We need to re-pickle the session after we send a message to save the new message_index.
    cache::updateOutboundMegolmSession(room_id, group_session_data, session);

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

        if (!session) {
            nhlog::crypto()->warn("Unknown olm session: {}:{}", sender_key, id);
            continue;
        }

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
            return nlohmann::json::parse(std::string_view((char *)text.data(), text.size()));
        } catch (const nlohmann::json::exception &e) {
            nhlog::crypto()->critical("failed to parse the decrypted session msg: {} {}",
                                      e.what(),
                                      std::string_view((char *)text.data(), text.size()));
        }
    }

    return {};
}

void
create_inbound_megolm_session(const mtx::events::DeviceEvent<mtx::events::msg::RoomKey> &roomKey,
                              const std::string &sender_key,
                              const std::string &sender_ed25519)
{
    MegolmSessionIndex index;
    index.room_id    = roomKey.content.room_id;
    index.session_id = roomKey.content.session_id;

    try {
        GroupSessionData data{};
        data.forwarding_curve25519_key_chain = {sender_key};
        data.sender_claimed_ed25519_key      = sender_ed25519;
        data.sender_key                      = sender_key;

        auto megolm_session =
          olm::client()->init_inbound_group_session(roomKey.content.session_key);
        backup_session_key(index, data, megolm_session);
        cache::saveInboundMegolmSession(index, std::move(megolm_session), data);
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

    try {
        auto megolm_session =
          olm::client()->import_inbound_group_session(roomKey.content.session_key);

        GroupSessionData data{};
        data.forwarding_curve25519_key_chain = roomKey.content.forwarding_curve25519_key_chain;
        data.sender_claimed_ed25519_key      = roomKey.content.sender_claimed_ed25519_key;
        data.sender_key                      = roomKey.content.sender_key;
        // may have come from online key backup, so we can't trust it...
        data.trusted = false;
        // if we got it forwarded from the sender, assume it is trusted. They may still have
        // used key backup, but it is unlikely.
        if (roomKey.content.forwarding_curve25519_key_chain.size() == 1 &&
            roomKey.content.forwarding_curve25519_key_chain.back() == roomKey.content.sender_key) {
            data.trusted = true;
        }

        backup_session_key(index, data, megolm_session);
        cache::saveInboundMegolmSession(index, std::move(megolm_session), data);
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
backup_session_key(const MegolmSessionIndex &idx,
                   const GroupSessionData &data,
                   mtx::crypto::InboundGroupSessionPtr &session)
{
    try {
        if (!UserSettings::instance()->useOnlineKeyBackup()) {
            // Online key backup disabled
            return;
        }

        auto backupVersion = cache::client()->backupVersion();
        if (!backupVersion) {
            // no trusted OKB
            return;
        }

        using namespace mtx::crypto;

        auto decryptedSecret = cache::secret(mtx::secret_storage::secrets::megolm_backup_v1);
        if (!decryptedSecret) {
            // no backup key available
            return;
        }
        auto sessionDecryptionKey = to_binary_buf(base642bin(*decryptedSecret));

        auto public_key = mtx::crypto::CURVE25519_public_key_from_private(sessionDecryptionKey);

        mtx::responses::backup::SessionData sessionData;
        sessionData.algorithm                       = mtx::crypto::MEGOLM_ALGO;
        sessionData.forwarding_curve25519_key_chain = data.forwarding_curve25519_key_chain;
        sessionData.sender_claimed_keys["ed25519"]  = data.sender_claimed_ed25519_key;
        sessionData.sender_key                      = data.sender_key;
        sessionData.session_key = mtx::crypto::export_session(session.get(), -1);

        auto encrypt_session = mtx::crypto::encrypt_session(sessionData, public_key);

        mtx::responses::backup::SessionBackup bk;
        bk.first_message_index = olm_inbound_group_session_first_known_index(session.get());
        bk.forwarded_count     = data.forwarding_curve25519_key_chain.size();
        bk.is_verified         = false;
        bk.session_data        = std::move(encrypt_session);

        http::client()->put_room_keys(
          backupVersion->version,
          idx.room_id,
          idx.session_id,
          bk,
          [idx](mtx::http::RequestErr err) {
              if (err) {
                  nhlog::net()->warn("failed to backup session key ({}:{}): {} ({})",
                                     idx.room_id,
                                     idx.session_id,
                                     err->matrix_error.error,
                                     static_cast<int>(err->status_code));
              } else {
                  nhlog::crypto()->debug(
                    "backed up session key ({}:{})", idx.room_id, idx.session_id);
              }
          });
    } catch (std::exception &e) {
        nhlog::net()->warn("failed to backup session key: {}", e.what());
    }
}

void
mark_keys_as_published()
{
    olm::client()->mark_keys_as_published();
    cache::saveOlmAccount(olm::client()->save(cache::client()->pickleSecret()));
}

void
download_full_keybackup()
{
    if (!UserSettings::instance()->useOnlineKeyBackup()) {
        // Online key backup disabled
        return;
    }

    auto backupVersion = cache::client()->backupVersion();
    if (!backupVersion) {
        // no trusted OKB
        return;
    }

    using namespace mtx::crypto;

    auto decryptedSecret = cache::secret(mtx::secret_storage::secrets::megolm_backup_v1);
    if (!decryptedSecret) {
        // no backup key available
        return;
    }
    auto sessionDecryptionKey = to_binary_buf(base642bin(*decryptedSecret));

    http::client()->room_keys(
      backupVersion->version,
      [sessionDecryptionKey](const mtx::responses::backup::KeysBackup &bk,
                             mtx::http::RequestErr err) {
          if (err) {
              if (err->status_code != 404)
                  nhlog::crypto()->error("Failed to dowload backup: {} - {}",
                                         mtx::errors::to_string(err->matrix_error.errcode),
                                         err->matrix_error.error);
              return;
          }

          mtx::crypto::ExportedSessionKeys allKeys;
          try {
              for (const auto &[room, roomKey] : bk.rooms) {
                  for (const auto &[session_id, encSession] : roomKey.sessions) {
                      auto session = decrypt_session(encSession.session_data, sessionDecryptionKey);

                      if (session.algorithm != mtx::crypto::MEGOLM_ALGO)
                          // don't know this algorithm
                          return;

                      ExportedSession sess{};
                      sess.session_id = session_id;
                      sess.room_id    = room;
                      sess.algorithm  = mtx::crypto::MEGOLM_ALGO;
                      sess.forwarding_curve25519_key_chain =
                        std::move(session.forwarding_curve25519_key_chain);
                      sess.sender_claimed_keys = std::move(session.sender_claimed_keys);
                      sess.sender_key          = std::move(session.sender_key);
                      sess.session_key         = std::move(session.session_key);
                      allKeys.sessions.push_back(std::move(sess));
                  }
              }

              // call on UI thread
              QTimer::singleShot(0, ChatPage::instance(), [keys = std::move(allKeys)] {
                  cache::importSessionKeys(keys);
              });
          } catch (const lmdb::error &e) {
              nhlog::crypto()->critical("failed to save inbound megolm session: {}", e.what());
          }
      });
}
void
lookup_keybackup(const std::string room, const std::string session_id)
{
    if (!UserSettings::instance()->useOnlineKeyBackup()) {
        // Online key backup disabled
        return;
    }

    auto backupVersion = cache::client()->backupVersion();
    if (!backupVersion) {
        // no trusted OKB
        return;
    }

    using namespace mtx::crypto;

    auto decryptedSecret = cache::secret(mtx::secret_storage::secrets::megolm_backup_v1);
    if (!decryptedSecret) {
        // no backup key available
        return;
    }
    auto sessionDecryptionKey = to_binary_buf(base642bin(*decryptedSecret));

    http::client()->room_keys(
      backupVersion->version,
      room,
      session_id,
      [room, session_id, sessionDecryptionKey](const mtx::responses::backup::SessionBackup &bk,
                                               mtx::http::RequestErr err) {
          if (err) {
              if (err->status_code != 404)
                  nhlog::crypto()->error("Failed to dowload key {}:{}: {} - {}",
                                         room,
                                         session_id,
                                         mtx::errors::to_string(err->matrix_error.errcode),
                                         err->matrix_error.error);
              return;
          }
          try {
              auto session = decrypt_session(bk.session_data, sessionDecryptionKey);

              if (session.algorithm != mtx::crypto::MEGOLM_ALGO)
                  // don't know this algorithm
                  return;

              MegolmSessionIndex index;
              index.room_id    = room;
              index.session_id = session_id;

              GroupSessionData data{};
              data.forwarding_curve25519_key_chain = session.forwarding_curve25519_key_chain;
              data.sender_claimed_ed25519_key      = session.sender_claimed_keys["ed25519"];
              data.sender_key                      = session.sender_key;
              // online key backup can't be trusted, because anyone can upload to it.
              data.trusted = false;

              auto megolm_session =
                olm::client()->import_inbound_group_session(session.session_key);

              if (!cache::inboundMegolmSessionExists(index) ||
                  olm_inbound_group_session_first_known_index(megolm_session.get()) <
                    olm_inbound_group_session_first_known_index(
                      cache::getInboundMegolmSession(index).get())) {
                  cache::saveInboundMegolmSession(index, std::move(megolm_session), data);

                  nhlog::crypto()->info("imported inbound megolm session "
                                        "from key backup ({}, {})",
                                        room,
                                        session_id);

                  // call on UI thread
                  QTimer::singleShot(0, ChatPage::instance(), [index] {
                      ChatPage::instance()->receivedSessionKey(index.room_id, index.session_id);
                  });
              }
          } catch (const lmdb::error &e) {
              nhlog::crypto()->critical("failed to save inbound megolm session: {}", e.what());
              return;
          } catch (const mtx::crypto::olm_exception &e) {
              nhlog::crypto()->critical("failed to import inbound megolm session: {}", e.what());
              return;
          }
      });
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
    request.action = cancel ? mtx::events::msg::RequestAction::Cancellation
                            : mtx::events::msg::RequestAction::Request;

    request.algorithm            = MEGOLM_ALGO;
    request.room_id              = e.room_id;
    request.sender_key           = e.content.sender_key;
    request.session_id           = e.content.session_id;
    request.request_id           = request_id;
    request.requesting_device_id = http::client()->device_id();

    nhlog::crypto()->debug("m.room_key_request: {}", nlohmann::json(request).dump(2));

    std::map<mtx::identifiers::User, std::map<std::string, decltype(request)>> body;
    body[mtx::identifiers::parse<mtx::identifiers::User>(e.sender)]["*"] = request;
    body[http::client()->user_id()]["*"]                                 = request;

    http::client()->send_to_device(
      http::client()->generate_txn_id(), body, [e](mtx::http::RequestErr err) {
          if (err) {
              nhlog::net()->warn("failed to send "
                                 "send_to_device "
                                 "message: {}",
                                 err->matrix_error.error);
          }

          nhlog::net()->info(
            "m.room_key_request sent to {}:{} and your own devices", e.sender, e.content.device_id);
      });

    // http::client()->room_keys
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

    // Check that the requested session_id and the one we have saved match.
    MegolmSessionIndex index{};
    index.room_id    = req.content.room_id;
    index.session_id = req.content.session_id;

    // Check if we have the keys for the requested session.
    auto sessionData = cache::getMegolmSessionData(index);
    if (!sessionData) {
        nhlog::crypto()->warn("requested session not found in room: {}", req.content.room_id);
        return;
    }

    // Check if we were the sender of the session being requested (unless it is actually us
    // requesting the session).
    if (req.sender != http::client()->user_id().to_string() &&
        sessionData->sender_key != olm::client()->identity_keys().curve25519) {
        nhlog::crypto()->debug(
          "ignoring key request {} because we did not create the requested session: "
          "\nrequested({}) ours({})",
          req.content.request_id,
          sessionData->sender_key,
          olm::client()->identity_keys().curve25519);
        return;
    }

    const auto session = cache::getInboundMegolmSession(index);
    if (!session) {
        nhlog::crypto()->warn("No session with id {} in db", req.content.session_id);
        return;
    }

    if (!cache::isRoomMember(req.sender, req.content.room_id)) {
        nhlog::crypto()->warn("user {} that requested the session key is not member of the room {}",
                              req.sender,
                              req.content.room_id);
        return;
    }

    // check if device is verified
    auto verificationStatus = cache::verificationStatus(req.sender);
    bool verifiedDevice     = false;
    if (verificationStatus &&
        // Share keys, if the option to share with trusted users is enabled or with yourself
        (ChatPage::instance()->userSettings()->shareKeysWithTrustedUsers() ||
         req.sender == http::client()->user_id().to_string())) {
        for (const auto &dev : verificationStatus->verified_devices) {
            if (dev == req.content.requesting_device_id) {
                verifiedDevice = true;
                nhlog::crypto()->debug("Verified device: {}", dev);
                break;
            }
        }
    }

    bool shouldSeeKeys    = false;
    uint64_t minimumIndex = -1;
    if (sessionData->currently.keys.count(req.sender)) {
        if (sessionData->currently.keys.at(req.sender)
              .deviceids.count(req.content.requesting_device_id)) {
            shouldSeeKeys = true;
            minimumIndex  = sessionData->currently.keys.at(req.sender)
                             .deviceids.at(req.content.requesting_device_id);
        }
    }

    if (!verifiedDevice && !shouldSeeKeys) {
        nhlog::crypto()->debug("ignoring key request for room {}", req.content.room_id);
        return;
    }

    if (verifiedDevice) {
        // share the minimum index we have
        minimumIndex = -1;
    }

    try {
        auto session_key = mtx::crypto::export_session(session.get(), minimumIndex);

        //
        // Prepare the m.room_key event.
        //
        mtx::events::msg::ForwardedRoomKey forward_key{};
        forward_key.algorithm   = MEGOLM_ALGO;
        forward_key.room_id     = index.room_id;
        forward_key.session_id  = index.session_id;
        forward_key.session_key = session_key;
        forward_key.sender_key  = sessionData->sender_key;

        // TODO(Nico): Figure out if this is correct
        forward_key.sender_claimed_ed25519_key      = sessionData->sender_claimed_ed25519_key;
        forward_key.forwarding_curve25519_key_chain = sessionData->forwarding_curve25519_key_chain;

        send_megolm_key_to_device(req.sender, req.content.requesting_device_id, forward_key);
    } catch (std::exception &e) {
        nhlog::crypto()->error("Failed to forward session key: {}", e.what());
    }
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
    nhlog::crypto()->debug("Forwarded key to {}:{}", user_id, device_id);
}

DecryptionResult
decryptEvent(const MegolmSessionIndex &index,
             const mtx::events::EncryptedEvent<mtx::events::msg::Encrypted> &event,
             bool dont_write_db)
{
    try {
        if (!cache::client()->inboundMegolmSessionExists(index)) {
            return {DecryptionErrorCode::MissingSession, std::nullopt, std::nullopt};
        }
    } catch (const lmdb::error &e) {
        return {DecryptionErrorCode::DbError, e.what(), std::nullopt};
    }

    std::string msg_str;
    try {
        auto session = cache::client()->getInboundMegolmSession(index);
        if (!session) {
            return {DecryptionErrorCode::MissingSession, std::nullopt, std::nullopt};
        }

        auto sessionData =
          cache::client()->getMegolmSessionData(index).value_or(GroupSessionData{});

        auto res = olm::client()->decrypt_group_message(session.get(), event.content.ciphertext);
        msg_str  = std::string((char *)res.data.data(), res.data.size());

        if (!event.event_id.empty() && event.event_id[0] == '$') {
            auto oldIdx = sessionData.indices.find(res.message_index);
            if (oldIdx != sessionData.indices.end()) {
                if (oldIdx->second != event.event_id)
                    return {DecryptionErrorCode::ReplayAttack, std::nullopt, std::nullopt};
            } else if (!dont_write_db) {
                sessionData.indices[res.message_index] = event.event_id;
                cache::client()->saveInboundMegolmSession(index, std::move(session), sessionData);
            }
        }
    } catch (const lmdb::error &e) {
        return {DecryptionErrorCode::DbError, e.what(), std::nullopt};
    } catch (const mtx::crypto::olm_exception &e) {
        if (e.error_code() == mtx::crypto::OlmErrorCode::UNKNOWN_MESSAGE_INDEX)
            return {DecryptionErrorCode::MissingSessionIndex, e.what(), std::nullopt};
        return {DecryptionErrorCode::DecryptionFailed, e.what(), std::nullopt};
    }

    try {
        // Add missing fields for the event.
        nlohmann::json body      = nlohmann::json::parse(msg_str);
        body["event_id"]         = event.event_id;
        body["sender"]           = event.sender;
        body["origin_server_ts"] = event.origin_server_ts;
        body["unsigned"]         = event.unsigned_data;

        mtx::events::collections::TimelineEvent te;
        from_json(body, te);

        // relations are unencrypted in content...
        mtx::accessors::set_relations(te.data, std::move(event.content.relations));

        return {DecryptionErrorCode::NoError, std::nullopt, std::move(te.data)};
    } catch (std::exception &e) {
        return {DecryptionErrorCode::ParsingFailed, e.what(), std::nullopt};
    }
}

crypto::Trust
calculate_trust(const std::string &user_id, const MegolmSessionIndex &index)
{
    auto status              = cache::client()->verificationStatus(user_id);
    auto megolmData          = cache::client()->getMegolmSessionData(index);
    crypto::Trust trustlevel = crypto::Trust::Unverified;

    if (megolmData && megolmData->trusted &&
        status.verified_device_keys.count(megolmData->sender_key))
        trustlevel = status.verified_device_keys.at(megolmData->sender_key);

    return trustlevel;
}

//! Send encrypted to device messages, targets is a map from userid to device ids or {} for all
//! devices
void
send_encrypted_to_device_messages(const std::map<std::string, std::vector<std::string>> targets,
                                  const mtx::events::collections::DeviceEvents &event,
                                  bool force_new_session)
{
    static QMap<QPair<std::string, std::string>, qint64> rateLimit;

    nlohmann::json ev_json = std::visit([](const auto &e) { return nlohmann::json(e); }, event);

    std::map<std::string, std::vector<std::string>> keysToQuery;
    mtx::requests::ClaimKeys claims;
    std::map<mtx::identifiers::User, std::map<std::string, mtx::events::msg::OlmEncrypted>>
      messages;
    std::map<std::string, std::map<std::string, DevicePublicKeys>> pks;

    auto our_curve = olm::client()->identity_keys().curve25519;

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
            deviceTargets.reserve(deviceKeys->device_keys.size());
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

            if (!d.keys.count("curve25519:" + device) || !d.keys.count("ed25519:" + device)) {
                nhlog::crypto()->warn("Skipping device {} since it has no keys!", device);
                continue;
            }

            auto device_curve = d.keys.at("curve25519:" + device);
            if (device_curve == our_curve) {
                nhlog::crypto()->warn("Skipping our own device, since sending "
                                      "ourselves olm messages makes no sense.");
                continue;
            }

            auto session = cache::getLatestOlmSession(device_curve);
            if (!session || force_new_session) {
                auto currentTime = QDateTime::currentSecsSinceEpoch();
                if (rateLimit.value(QPair(user, device)) + 60 * 60 * 10 < currentTime) {
                    claims.one_time_keys[user][device] = mtx::crypto::SIGNED_CURVE25519;
                    pks[user][device].ed25519          = d.keys.at("ed25519:" + device);
                    pks[user][device].curve25519       = d.keys.at("curve25519:" + device);

                    rateLimit.insert(QPair(user, device), currentTime);
                } else {
                    nhlog::crypto()->warn("Not creating new session with {}:{} "
                                          "because of rate limit",
                                          user,
                                          device);
                }
                continue;
            }

            messages[mtx::identifiers::parse<mtx::identifiers::User>(user)][device] =
              olm::client()
                ->create_olm_encrypted_content(session->get(),
                                               ev_json,
                                               UserId(user),
                                               d.keys.at("ed25519:" + device),
                                               device_curve)
                .get<mtx::events::msg::OlmEncrypted>();

            try {
                nhlog::crypto()->debug("Updated olm session: {}",
                                       mtx::crypto::session_id(session->get()));
                cache::saveOlmSession(d.keys.at("curve25519:" + device),
                                      std::move(*session),
                                      QDateTime::currentMSecsSinceEpoch());
            } catch (const lmdb::error &e) {
                nhlog::db()->critical("failed to save outbound olm session: {}", e.what());
            } catch (const mtx::crypto::olm_exception &e) {
                nhlog::crypto()->critical("failed to pickle outbound olm session: {}", e.what());
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
            std::map<mtx::identifiers::User, std::map<std::string, mtx::events::msg::OlmEncrypted>>
              messages;
            for (const auto &[user_id, retrieved_devices] : res.one_time_keys) {
                nhlog::net()->debug("claimed keys for {}", user_id);
                if (retrieved_devices.size() == 0) {
                    nhlog::net()->debug("no one-time keys found for user_id: {}", user_id);
                    continue;
                }

                for (const auto &rd : retrieved_devices) {
                    const auto device_id = rd.first;

                    nhlog::net()->debug("{} : \n {}", device_id, rd.second.dump(2));

                    if (rd.second.empty() || !rd.second.begin()->contains("key")) {
                        nhlog::net()->warn("Skipping device {} as it has no key.", device_id);
                        continue;
                    }

                    auto otk = rd.second.begin()->at("key").get<std::string>();

                    auto sign_key = pks.at(user_id).at(device_id).ed25519;
                    auto id_key   = pks.at(user_id).at(device_id).curve25519;

                    // Verify signature
                    {
                        auto signedKey = *rd.second.begin();
                        std::string signature =
                          signedKey["signatures"][user_id].value("ed25519:" + device_id, "");

                        if (signature.empty() || !mtx::crypto::ed25519_verify_signature(
                                                   sign_key, signedKey, signature)) {
                            nhlog::net()->warn("Skipping device {} as its one time key "
                                               "has an invalid signature.",
                                               device_id);
                            continue;
                        }
                    }

                    auto session = olm::client()->create_outbound_session(id_key, otk);

                    messages[mtx::identifiers::parse<mtx::identifiers::User>(user_id)][device_id] =
                      olm::client()
                        ->create_olm_encrypted_content(
                          session.get(), ev_json, UserId(user_id), sign_key, id_key)
                        .get<mtx::events::msg::OlmEncrypted>();

                    try {
                        nhlog::crypto()->debug("Updated olm session: {}",
                                               mtx::crypto::session_id(session.get()));
                        cache::saveOlmSession(
                          id_key, std::move(session), QDateTime::currentMSecsSinceEpoch());
                    } catch (const lmdb::error &e) {
                        nhlog::db()->critical("failed to save outbound olm session: {}", e.what());
                    } catch (const mtx::crypto::olm_exception &e) {
                        nhlog::crypto()->critical("failed to pickle outbound olm session: {}",
                                                  e.what());
                    }
                }
                nhlog::net()->info("send_to_device: {}", user_id);
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
        };
    };

    if (!claims.one_time_keys.empty())
        http::client()->claim_keys(claims, BindPks(pks));

    if (!keysToQuery.empty()) {
        mtx::requests::QueryKeys req;
        req.device_keys = keysToQuery;
        http::client()->query_keys(
          req,
          [ev_json, BindPks, our_curve](const mtx::responses::QueryKeys &res,
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

                      if (user_id.get() == http::client()->user_id().to_string() &&
                          device_id.get() == http::client()->device_id())
                          continue;

                      const auto device_keys = dev.second.keys;
                      const auto curveKey    = "curve25519:" + device_id.get();
                      const auto edKey       = "ed25519:" + device_id.get();

                      if ((device_keys.find(curveKey) == device_keys.end()) ||
                          (device_keys.find(edKey) == device_keys.end())) {
                          nhlog::net()->debug("ignoring malformed keys for device {}",
                                              device_id.get());
                          continue;
                      }

                      DevicePublicKeys pks;
                      pks.ed25519    = device_keys.at(edKey);
                      pks.curve25519 = device_keys.at(curveKey);

                      if (pks.curve25519 == our_curve) {
                          nhlog::crypto()->warn("Skipping our own device, since sending "
                                                "ourselves olm messages makes no sense.");
                          continue;
                      }

                      try {
                          if (!mtx::crypto::verify_identity_signature(
                                dev.second, device_id, user_id)) {
                              nhlog::crypto()->warn("failed to verify identity keys: {}",
                                                    nlohmann::json(dev.second).dump(2));
                              continue;
                          }
                      } catch (const nlohmann::json::exception &e) {
                          nhlog::crypto()->warn("failed to parse device key json: {}", e.what());
                          continue;
                      } catch (const mtx::crypto::olm_exception &e) {
                          nhlog::crypto()->warn("failed to verify device key json: {}", e.what());
                          continue;
                      }

                      auto currentTime = QDateTime::currentSecsSinceEpoch();
                      if (rateLimit.value(QPair(user.first, device_id.get())) + 60 * 60 * 10 <
                          currentTime) {
                          deviceKeys[user_id].emplace(device_id, pks);
                          claim_keys.one_time_keys[user.first][device_id] =
                            mtx::crypto::SIGNED_CURVE25519;

                          rateLimit.insert(QPair(user.first, device_id.get()), currentTime);
                      } else {
                          nhlog::crypto()->warn("Not creating new session with {}:{} "
                                                "because of rate limit",
                                                user.first,
                                                device_id.get());
                          continue;
                      }

                      nhlog::net()->info("{}", device_id.get());
                      nhlog::net()->info("  curve25519 {}", pks.curve25519);
                      nhlog::net()->info("  ed25519 {}", pks.ed25519);
                  }
              }

              if (!claim_keys.one_time_keys.empty())
                  http::client()->claim_keys(claim_keys, BindPks(deviceKeys));
          });
    }
}

void
request_cross_signing_keys()
{
    mtx::events::msg::SecretRequest secretRequest{};
    secretRequest.action               = mtx::events::msg::RequestAction::Request;
    secretRequest.requesting_device_id = http::client()->device_id();

    auto local_user = http::client()->user_id();

    auto verificationStatus = cache::verificationStatus(local_user.to_string());

    if (!verificationStatus)
        return;

    auto request = [&](std::string secretName) {
        secretRequest.name       = secretName;
        secretRequest.request_id = "ss." + http::client()->generate_txn_id();

        request_id_to_secret_name[secretRequest.request_id] = secretRequest.name;

        std::map<mtx::identifiers::User, std::map<std::string, mtx::events::msg::SecretRequest>>
          body;

        for (const auto &dev : verificationStatus->verified_devices) {
            if (dev != secretRequest.requesting_device_id)
                body[local_user][dev] = secretRequest;
        }

        http::client()->send_to_device<mtx::events::msg::SecretRequest>(
          http::client()->generate_txn_id(),
          body,
          [request_id = secretRequest.request_id, secretName](mtx::http::RequestErr err) {
              if (err) {
                  nhlog::net()->error("Failed to send request for secrect '{}'", secretName);
                  // Cancel request on UI thread
                  QTimer::singleShot(1, cache::client(), [request_id]() {
                      request_id_to_secret_name.erase(request_id);
                  });
                  return;
              }
          });

        for (const auto &dev : verificationStatus->verified_devices) {
            if (dev != secretRequest.requesting_device_id)
                body[local_user][dev].action = mtx::events::msg::RequestAction::Cancellation;
        }

        // timeout after 15 min
        QTimer::singleShot(15 * 60 * 1000, ChatPage::instance(), [secretRequest, body]() {
            if (request_id_to_secret_name.count(secretRequest.request_id)) {
                request_id_to_secret_name.erase(secretRequest.request_id);
                http::client()->send_to_device<mtx::events::msg::SecretRequest>(
                  http::client()->generate_txn_id(),
                  body,
                  [secretRequest](mtx::http::RequestErr err) {
                      if (err) {
                          nhlog::net()->error("Failed to cancel request for secrect '{}'",
                                              secretRequest.name);
                          return;
                      }
                  });
            }
        });
    };

    request(mtx::secret_storage::secrets::cross_signing_master);
    request(mtx::secret_storage::secrets::cross_signing_self_signing);
    request(mtx::secret_storage::secrets::cross_signing_user_signing);
    request(mtx::secret_storage::secrets::megolm_backup_v1);
}

namespace {
void
unlock_secrets(const std::string &key,
               const std::map<std::string, mtx::secret_storage::AesHmacSha2EncryptedData> &secrets)
{
    http::client()->secret_storage_key(
      key,
      [secrets](mtx::secret_storage::AesHmacSha2KeyDescription keyDesc, mtx::http::RequestErr err) {
          if (err) {
              nhlog::net()->error("Failed to download secret storage key");
              return;
          }

          emit ChatPage::instance()->downloadedSecrets(keyDesc, secrets);
      });
}
}

void
download_cross_signing_keys()
{
    using namespace mtx::secret_storage;
    http::client()->secret_storage_secret(
      secrets::megolm_backup_v1, [](Secret secret, mtx::http::RequestErr err) {
          std::optional<Secret> backup_key;
          if (!err)
              backup_key = secret;

          http::client()->secret_storage_secret(
            secrets::cross_signing_master, [backup_key](Secret secret, mtx::http::RequestErr err) {
                std::optional<Secret> master_key;
                if (!err)
                    master_key = secret;

                http::client()->secret_storage_secret(
                  secrets::cross_signing_self_signing,
                  [backup_key, master_key](Secret secret, mtx::http::RequestErr err) {
                      std::optional<Secret> self_signing_key;
                      if (!err)
                          self_signing_key = secret;

                      http::client()->secret_storage_secret(
                        secrets::cross_signing_user_signing,
                        [backup_key, self_signing_key, master_key](Secret secret,
                                                                   mtx::http::RequestErr err) {
                            std::optional<Secret> user_signing_key;
                            if (!err)
                                user_signing_key = secret;

                            std::map<std::string, std::map<std::string, AesHmacSha2EncryptedData>>
                              secrets;

                            if (backup_key && !backup_key->encrypted.empty())
                                secrets[backup_key->encrypted.begin()->first]
                                       [secrets::megolm_backup_v1] =
                                         backup_key->encrypted.begin()->second;

                            if (master_key && !master_key->encrypted.empty())
                                secrets[master_key->encrypted.begin()->first]
                                       [secrets::cross_signing_master] =
                                         master_key->encrypted.begin()->second;

                            if (self_signing_key && !self_signing_key->encrypted.empty())
                                secrets[self_signing_key->encrypted.begin()->first]
                                       [secrets::cross_signing_self_signing] =
                                         self_signing_key->encrypted.begin()->second;

                            if (user_signing_key && !user_signing_key->encrypted.empty())
                                secrets[user_signing_key->encrypted.begin()->first]
                                       [secrets::cross_signing_user_signing] =
                                         user_signing_key->encrypted.begin()->second;

                            for (const auto &[key, secret_] : secrets)
                                unlock_secrets(key, secret_);
                        });
                  });
            });
      });
}

} // namespace olm
