// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "DeviceVerificationFlow.h"

#include "Cache.h"
#include "Cache_p.h"
#include "ChatPage.h"
#include "Logging.h"
#include "Utils.h"
#include "timeline/TimelineModel.h"

#include <QDateTime>
#include <QTimer>
#include <iostream>
#include <tuple>

static constexpr int TIMEOUT = 2 * 60 * 1000; // 2 minutes

static mtx::events::msg::KeyVerificationMac
key_verification_mac(mtx::crypto::SAS *sas,
                     mtx::identifiers::User sender,
                     const std::string &senderDevice,
                     mtx::identifiers::User receiver,
                     const std::string &receiverDevice,
                     const std::string &transactionId,
                     std::map<std::string, std::string> keys);

DeviceVerificationFlow::DeviceVerificationFlow(QObject *,
                                               DeviceVerificationFlow::Type flow_type,
                                               TimelineModel *model,
                                               QString userID,
                                               std::vector<QString> deviceIds_)
  : sender(false)
  , type(flow_type)
  , deviceIds(std::move(deviceIds_))
  , model_(model)
{
    nhlog::crypto()->debug("CREATING NEW FLOW, {}, {}", flow_type, (void *)this);
    if (deviceIds.size() == 1)
        deviceId = deviceIds.front();

    timeout = new QTimer(this);
    timeout->setSingleShot(true);
    this->sas           = olm::client()->sas_init();
    this->isMacVerified = false;

    auto user_id_  = userID.toStdString();
    this->toClient = mtx::identifiers::parse<mtx::identifiers::User>(user_id_);
    cache::client()->query_keys(
      user_id_, [user_id_, this](const UserKeyCache &res, mtx::http::RequestErr err) {
          if (err) {
              nhlog::net()->warn("failed to query device keys: {},{}",
                                 mtx::errors::to_string(err->matrix_error.errcode),
                                 static_cast<int>(err->status_code));
              return;
          }

          if (!this->deviceId.isEmpty() &&
              (res.device_keys.find(deviceId.toStdString()) == res.device_keys.end())) {
              nhlog::net()->warn("no devices retrieved {}", user_id_);
              return;
          }

          this->their_keys = res;
      });

    cache::client()->query_keys(
      http::client()->user_id().to_string(),
      [this](const UserKeyCache &res, mtx::http::RequestErr err) {
          if (err) {
              nhlog::net()->warn("failed to query device keys: {},{}",
                                 mtx::errors::to_string(err->matrix_error.errcode),
                                 static_cast<int>(err->status_code));
              return;
          }

          if (res.master_keys.keys.empty())
              return;

          if (auto status = cache::verificationStatus(http::client()->user_id().to_string());
              status && status->user_verified == crypto::Trust::Verified)
              this->our_trusted_master_key = res.master_keys.keys.begin()->second;
      });

    if (model) {
        connect(
          this->model_, &TimelineModel::updateFlowEventId, this, [this](std::string event_id_) {
              this->relation.rel_type = mtx::common::RelationType::Reference;
              this->relation.event_id = event_id_;
              this->transaction_id    = event_id_;
          });
    }

    connect(timeout, &QTimer::timeout, this, [this]() {
        nhlog::crypto()->info("verification: timeout");
        if (state_ != Success && state_ != Failed)
            this->cancelVerification(DeviceVerificationFlow::Error::Timeout);
    });

    connect(ChatPage::instance(),
            &ChatPage::receivedDeviceVerificationStart,
            this,
            &DeviceVerificationFlow::handleStartMessage);
    connect(ChatPage::instance(),
            &ChatPage::receivedDeviceVerificationAccept,
            this,
            [this](const mtx::events::msg::KeyVerificationAccept &msg) {
                nhlog::crypto()->info("verification: received accept");
                if (msg.transaction_id.has_value()) {
                    if (msg.transaction_id.value() != this->transaction_id)
                        return;
                } else if (msg.relations.references()) {
                    if (msg.relations.references() != this->relation.event_id)
                        return;
                }
                if ((msg.key_agreement_protocol == "curve25519-hkdf-sha256") &&
                    (msg.hash == "sha256") &&
                    (msg.message_authentication_code == "hkdf-hmac-sha256")) {
                    this->commitment = msg.commitment;
                    if (std::find(msg.short_authentication_string.begin(),
                                  msg.short_authentication_string.end(),
                                  mtx::events::msg::SASMethods::Emoji) !=
                        msg.short_authentication_string.end()) {
                        this->method = mtx::events::msg::SASMethods::Emoji;
                    } else {
                        this->method = mtx::events::msg::SASMethods::Decimal;
                    }
                    this->mac_method = msg.message_authentication_code;
                    this->sendVerificationKey();
                } else {
                    this->cancelVerification(DeviceVerificationFlow::Error::UnknownMethod);
                }
            });

    connect(ChatPage::instance(),
            &ChatPage::receivedDeviceVerificationCancel,
            this,
            [this](const mtx::events::msg::KeyVerificationCancel &msg) {
                nhlog::crypto()->info(
                  "verification: received cancel, {} : {}", msg.code, msg.reason);
                if (msg.transaction_id.has_value()) {
                    if (msg.transaction_id.value() != this->transaction_id)
                        return;
                } else if (msg.relations.references()) {
                    if (msg.relations.references() != this->relation.event_id)
                        return;
                }
                error_ = User;
                emit errorChanged();
                setState(Failed);
            });

    connect(
      ChatPage::instance(),
      &ChatPage::receivedDeviceVerificationKey,
      this,
      [this](const mtx::events::msg::KeyVerificationKey &msg) {
          nhlog::crypto()->info(
            "verification: received key, sender {}, state {}", sender, state().toStdString());
          if (msg.transaction_id.has_value()) {
              if (msg.transaction_id.value() != this->transaction_id)
                  return;
          } else if (msg.relations.references()) {
              if (msg.relations.references() != this->relation.event_id)
                  return;
          }

          if (sender) {
              if (state_ != WaitingForOtherToAccept && state_ != WaitingForKeys) {
                  this->cancelVerification(OutOfOrder);
                  return;
              }
          } else {
              if (state_ != WaitingForKeys) {
                  this->cancelVerification(OutOfOrder);
                  return;
              }
          }

          this->sas->set_their_key(msg.key);
          std::string info;
          if (this->sender == true) {
              info = "MATRIX_KEY_VERIFICATION_SAS|" + http::client()->user_id().to_string() + "|" +
                     http::client()->device_id() + "|" + this->sas->public_key() + "|" +
                     this->toClient.to_string() + "|" + this->deviceId.toStdString() + "|" +
                     msg.key + "|" + this->transaction_id;
          } else {
              info = "MATRIX_KEY_VERIFICATION_SAS|" + this->toClient.to_string() + "|" +
                     this->deviceId.toStdString() + "|" + msg.key + "|" +
                     http::client()->user_id().to_string() + "|" + http::client()->device_id() +
                     "|" + this->sas->public_key() + "|" + this->transaction_id;
          }

          nhlog::ui()->info("Info is: '{}'", info);

          if (this->sender == false) {
              this->sendVerificationKey();
          } else {
              if (this->commitment != mtx::crypto::bin2base64_unpadded(
                                        mtx::crypto::sha256(msg.key + this->canonical_json))) {
                  this->cancelVerification(DeviceVerificationFlow::Error::MismatchedCommitment);
                  return;
              }
          }

          if (this->method == mtx::events::msg::SASMethods::Emoji) {
              this->sasList = this->sas->generate_bytes_emoji(info);
              setState(CompareEmoji);
          } else if (this->method == mtx::events::msg::SASMethods::Decimal) {
              this->sasList = this->sas->generate_bytes_decimal(info);
              setState(CompareNumber);
          }
      });

    connect(
      ChatPage::instance(),
      &ChatPage::receivedDeviceVerificationMac,
      this,
      [this](const mtx::events::msg::KeyVerificationMac &msg) {
          nhlog::crypto()->info("verification: received mac");
          if (msg.transaction_id.has_value()) {
              if (msg.transaction_id.value() != this->transaction_id)
                  return;
          } else if (msg.relations.references()) {
              if (msg.relations.references() != this->relation.event_id)
                  return;
          }

          std::map<std::string, std::string> key_list;
          std::string key_string;
          for (const auto &mac : msg.mac) {
              for (const auto &[deviceid, key] : their_keys.device_keys) {
                  (void)deviceid;
                  if (key.keys.count(mac.first))
                      key_list[mac.first] = key.keys.at(mac.first);
              }

              if (their_keys.master_keys.keys.count(mac.first))
                  key_list[mac.first] = their_keys.master_keys.keys[mac.first];
              if (their_keys.user_signing_keys.keys.count(mac.first))
                  key_list[mac.first] = their_keys.user_signing_keys.keys[mac.first];
              if (their_keys.self_signing_keys.keys.count(mac.first))
                  key_list[mac.first] = their_keys.self_signing_keys.keys[mac.first];
          }
          auto macs = key_verification_mac(sas.get(),
                                           toClient,
                                           this->deviceId.toStdString(),
                                           http::client()->user_id(),
                                           http::client()->device_id(),
                                           this->transaction_id,
                                           key_list);

          for (const auto &[key, mac] : macs.mac) {
              if (mac != msg.mac.at(key)) {
                  this->cancelVerification(DeviceVerificationFlow::Error::KeyMismatch);
                  return;
              }
          }

          if (msg.keys == macs.keys) {
              mtx::requests::KeySignaturesUpload req;
              if (utils::localUser().toStdString() == this->toClient.to_string()) {
                  // self verification, sign master key with device key, if we
                  // verified it
                  for (const auto &mac : msg.mac) {
                      if (their_keys.master_keys.keys.count(mac.first)) {
                          nlohmann::json j = their_keys.master_keys;
                          j.erase("signatures");
                          j.erase("unsigned");
                          mtx::crypto::CrossSigningKeys master_key =
                            j.get<mtx::crypto::CrossSigningKeys>();
                          master_key.signatures[utils::localUser().toStdString()]
                                               ["ed25519:" + http::client()->device_id()] =
                            olm::client()->sign_message(j.dump());
                          req.signatures[utils::localUser().toStdString()]
                                        [master_key.keys.at(mac.first)] = master_key;
                      } else if (mac.first == "ed25519:" + this->deviceId.toStdString()) {
                          // Sign their device key with self signing key

                          auto device_id = this->deviceId.toStdString();

                          if (their_keys.device_keys.count(device_id)) {
                              nlohmann::json j = their_keys.device_keys.at(device_id);
                              j.erase("signatures");
                              j.erase("unsigned");

                              auto secret = cache::secret(
                                mtx::secret_storage::secrets::cross_signing_self_signing);
                              if (!secret)
                                  continue;
                              auto ssk = mtx::crypto::PkSigning::from_seed(*secret);

                              mtx::crypto::DeviceKeys dev = j.get<mtx::crypto::DeviceKeys>();
                              dev.signatures[utils::localUser().toStdString()]
                                            ["ed25519:" + ssk.public_key()] = ssk.sign(j.dump());

                              req.signatures[utils::localUser().toStdString()][device_id] = dev;
                          }
                      }
                  }
              } else {
                  // Sign their master key with user signing key
                  for (const auto &mac : msg.mac) {
                      if (their_keys.master_keys.keys.count(mac.first)) {
                          nlohmann::json j = their_keys.master_keys;
                          j.erase("signatures");
                          j.erase("unsigned");

                          auto secret =
                            cache::secret(mtx::secret_storage::secrets::cross_signing_user_signing);
                          if (!secret)
                              continue;
                          auto usk = mtx::crypto::PkSigning::from_seed(*secret);

                          mtx::crypto::CrossSigningKeys master_key =
                            j.get<mtx::crypto::CrossSigningKeys>();
                          master_key.signatures[utils::localUser().toStdString()]
                                               ["ed25519:" + usk.public_key()] = usk.sign(j.dump());

                          req.signatures[toClient.to_string()][master_key.keys.at(mac.first)] =
                            master_key;
                      }
                  }
              }

              if (!req.signatures.empty()) {
                  http::client()->keys_signatures_upload(
                    req,
                    [](const mtx::responses::KeySignaturesUpload &res, mtx::http::RequestErr err) {
                        if (err) {
                            nhlog::net()->error("failed to upload signatures: {},{}",
                                                mtx::errors::to_string(err->matrix_error.errcode),
                                                static_cast<int>(err->status_code));
                        }

                        for (const auto &[user_id, tmp] : res.errors)
                            for (const auto &[key_id, e] : tmp)
                                nhlog::net()->error("signature error for user {} and key "
                                                    "id {}: {}, {}",
                                                    user_id,
                                                    key_id,
                                                    mtx::errors::to_string(e.errcode),
                                                    e.error);
                    });
              }

              this->isMacVerified = true;
              this->acceptDevice();
          } else {
              this->cancelVerification(DeviceVerificationFlow::Error::KeyMismatch);
          }
      });

    connect(
      ChatPage::instance(),
      &ChatPage::receivedDeviceVerificationReady,
      this,
      [this](const mtx::events::msg::KeyVerificationReady &msg) {
          nhlog::crypto()->info("verification: received ready {}", (void *)this);
          if (!sender) {
              if (msg.from_device != http::client()->device_id()) {
                  error_ = User;
                  emit errorChanged();
                  setState(Failed);
              }

              return;
          }

          if (msg.transaction_id.has_value()) {
              if (msg.transaction_id.value() != this->transaction_id)
                  return;

              if (this->deviceId.isEmpty() && this->deviceIds.size() > 1) {
                  auto from = QString::fromStdString(msg.from_device);
                  if (std::find(deviceIds.begin(), deviceIds.end(), from) != deviceIds.end()) {
                      mtx::events::msg::KeyVerificationCancel req{};
                      req.code           = "m.user";
                      req.reason         = "accepted by other device";
                      req.transaction_id = this->transaction_id;
                      mtx::requests::ToDeviceMessages<mtx::events::msg::KeyVerificationCancel> body;

                      for (const auto &d : this->deviceIds) {
                          if (d != from)
                              body[this->toClient][d.toStdString()] = req;
                      }

                      http::client()->send_to_device(
                        http::client()->generate_txn_id(), body, [](mtx::http::RequestErr err) {
                            if (err)
                                nhlog::net()->warn(
                                  "failed to send verification to_device message: {} {}",
                                  err->matrix_error.error,
                                  static_cast<int>(err->status_code));
                        });

                      this->deviceId  = from;
                      this->deviceIds = {from};
                  }
              }
          } else if (msg.relations.references()) {
              if (msg.relations.references() != this->relation.event_id)
                  return;
              else {
                  this->deviceId = QString::fromStdString(msg.from_device);
              }
          } else {
              return;
          }
          nhlog::crypto()->info("verification: received ready sending start {}", (void *)this);
          this->startVerificationRequest();
      });

    connect(ChatPage::instance(),
            &ChatPage::receivedDeviceVerificationDone,
            this,
            [this](const mtx::events::msg::KeyVerificationDone &msg) {
                nhlog::crypto()->info("verification: received done");
                if (msg.transaction_id.has_value()) {
                    if (msg.transaction_id.value() != this->transaction_id)
                        return;
                } else if (msg.relations.references()) {
                    if (msg.relations.references() != this->relation.event_id)
                        return;
                }
                nhlog::ui()->info("Flow done on other side");
            });

    timeout->start(TIMEOUT);
}

QString
DeviceVerificationFlow::state()
{
    switch (state_) {
    case PromptStartVerification:
        return QStringLiteral("PromptStartVerification");
    case CompareEmoji:
        return QStringLiteral("CompareEmoji");
    case CompareNumber:
        return QStringLiteral("CompareNumber");
    case WaitingForKeys:
        return QStringLiteral("WaitingForKeys");
    case WaitingForOtherToAccept:
        return QStringLiteral("WaitingForOtherToAccept");
    case WaitingForMac:
        return QStringLiteral("WaitingForMac");
    case Success:
        return QStringLiteral("Success");
    case Failed:
        return QStringLiteral("Failed");
    default:
        return QString();
    }
}

void
DeviceVerificationFlow::next()
{
    if (sender) {
        switch (state_) {
        case PromptStartVerification:
            sendVerificationRequest();
            break;
        case CompareEmoji:
        case CompareNumber:
            sendVerificationMac();
            break;
        case WaitingForKeys:
        case WaitingForOtherToAccept:
        case WaitingForMac:
        case Success:
        case Failed:
            nhlog::db()->error("verification: Invalid state transition!");
            break;
        }
    } else {
        switch (state_) {
        case PromptStartVerification:
            if (canonical_json.empty())
                sendVerificationReady();
            else // legacy path without request and ready
                acceptVerificationRequest();
            break;
        case CompareEmoji:
            [[fallthrough]];
        case CompareNumber:
            sendVerificationMac();
            break;
        case WaitingForKeys:
        case WaitingForOtherToAccept:
        case WaitingForMac:
        case Success:
        case Failed:
            nhlog::db()->error("verification: Invalid state transition!");
            break;
        }
    }
}

QString
DeviceVerificationFlow::getUserId()
{
    return QString::fromStdString(this->toClient.to_string());
}

QString
DeviceVerificationFlow::getDeviceId()
{
    return this->deviceId;
}

bool
DeviceVerificationFlow::getSender()
{
    return this->sender;
}

std::vector<int>
DeviceVerificationFlow::getSasList()
{
    return this->sasList;
}

bool
DeviceVerificationFlow::isSelfVerification() const
{
    return this->toClient.to_string() == http::client()->user_id().to_string();
}

void
DeviceVerificationFlow::setEventId(std::string event_id_)
{
    this->relation.rel_type = mtx::common::RelationType::Reference;
    this->relation.event_id = event_id_;
    this->transaction_id    = event_id_;
}

void
DeviceVerificationFlow::handleStartMessage(const mtx::events::msg::KeyVerificationStart &msg,
                                           std::string)
{
    if (msg.transaction_id.has_value()) {
        if (msg.transaction_id.value() != this->transaction_id)
            return;
    } else if (msg.relations.references()) {
        if (msg.relations.references() != this->relation.event_id)
            return;
    } else {
        return;
    }

    if ((std::find(msg.key_agreement_protocols.begin(),
                   msg.key_agreement_protocols.end(),
                   "curve25519-hkdf-sha256") != msg.key_agreement_protocols.end()) &&
        (std::find(msg.hashes.begin(), msg.hashes.end(), "sha256") != msg.hashes.end()) &&
        (std::find(msg.message_authentication_codes.begin(),
                   msg.message_authentication_codes.end(),
                   "hkdf-hmac-sha256") != msg.message_authentication_codes.end())) {
        if (std::find(msg.short_authentication_string.begin(),
                      msg.short_authentication_string.end(),
                      mtx::events::msg::SASMethods::Emoji) !=
            msg.short_authentication_string.end()) {
            this->method = mtx::events::msg::SASMethods::Emoji;
        } else if (std::find(msg.short_authentication_string.begin(),
                             msg.short_authentication_string.end(),
                             mtx::events::msg::SASMethods::Decimal) !=
                   msg.short_authentication_string.end()) {
            this->method = mtx::events::msg::SASMethods::Decimal;
        } else {
            this->cancelVerification(DeviceVerificationFlow::Error::UnknownMethod);
            return;
        }
        if (!sender)
            this->canonical_json = nlohmann::json(msg).dump();
        else {
            // resolve glare
            if (std::tuple(this->toClient.to_string(), this->deviceId.toStdString()) <
                std::tuple(utils::localUser().toStdString(), http::client()->device_id())) {
                // treat this as if the user with the smaller mxid or smaller deviceid (if the mxid
                // was the same) was the sender of "start"
                this->canonical_json = nlohmann::json(msg).dump();
                this->sender         = false;
            }

            if (msg.method != mtx::events::msg::VerificationMethods::SASv1) {
                cancelVerification(DeviceVerificationFlow::Error::UnknownMethod);
                return;
            }
        }

        // If we didn't send "start", accept the verification (otherwise wait for the other side to
        // accept
        if (state_ != PromptStartVerification && !sender)
            this->acceptVerificationRequest();
    } else {
        this->cancelVerification(DeviceVerificationFlow::Error::UnknownMethod);
    }
}

//! accepts a verification
void
DeviceVerificationFlow::acceptVerificationRequest()
{
    if (acceptSent)
        return;
    acceptSent = true;

    mtx::events::msg::KeyVerificationAccept req;

    req.method                      = mtx::events::msg::VerificationMethods::SASv1;
    req.key_agreement_protocol      = "curve25519-hkdf-sha256";
    req.hash                        = "sha256";
    req.message_authentication_code = "hkdf-hmac-sha256";
    if (this->method == mtx::events::msg::SASMethods::Emoji)
        req.short_authentication_string = {mtx::events::msg::SASMethods::Emoji};
    else if (this->method == mtx::events::msg::SASMethods::Decimal)
        req.short_authentication_string = {mtx::events::msg::SASMethods::Decimal};
    req.commitment = mtx::crypto::bin2base64_unpadded(
      mtx::crypto::sha256(this->sas->public_key() + this->canonical_json));

    send(req);
    setState(WaitingForKeys);
}
//! responds verification request
void
DeviceVerificationFlow::sendVerificationReady()
{
    mtx::events::msg::KeyVerificationReady req;

    req.from_device = http::client()->device_id();
    req.methods     = {mtx::events::msg::VerificationMethods::SASv1};

    send(req);
    setState(WaitingForKeys);
}
//! accepts a verification
void
DeviceVerificationFlow::sendVerificationDone()
{
    mtx::events::msg::KeyVerificationDone req;

    send(req);
}
//! starts the verification flow
void
DeviceVerificationFlow::startVerificationRequest()
{
    if (startSent)
        return;
    startSent = true;

    mtx::events::msg::KeyVerificationStart req;

    req.from_device                  = http::client()->device_id();
    req.method                       = mtx::events::msg::VerificationMethods::SASv1;
    req.key_agreement_protocols      = {"curve25519-hkdf-sha256"};
    req.hashes                       = {"sha256"};
    req.message_authentication_codes = {"hkdf-hmac-sha256"};
    req.short_authentication_string  = {mtx::events::msg::SASMethods::Decimal,
                                       mtx::events::msg::SASMethods::Emoji};

    if (this->type == DeviceVerificationFlow::Type::ToDevice) {
        mtx::requests::ToDeviceMessages<mtx::events::msg::KeyVerificationStart> body;
        req.transaction_id   = this->transaction_id;
        this->canonical_json = nlohmann::json(req).dump();
    } else if (this->type == DeviceVerificationFlow::Type::RoomMsg && model_) {
        req.relations.relations.push_back(this->relation);
        // Set synthesized to surpress the nheko relation extensions
        req.relations.synthesized = true;
        this->canonical_json      = nlohmann::json(req).dump();
    }
    send(req);
    setState(WaitingForOtherToAccept);
}
//! sends a verification request
void
DeviceVerificationFlow::sendVerificationRequest()
{
    mtx::events::msg::KeyVerificationRequest req;

    req.from_device = http::client()->device_id();
    req.methods     = {mtx::events::msg::VerificationMethods::SASv1};

    if (this->type == DeviceVerificationFlow::Type::ToDevice) {
        QDateTime currentTime = QDateTime::currentDateTimeUtc();

        req.timestamp = (uint64_t)currentTime.toMSecsSinceEpoch();

    } else if (this->type == DeviceVerificationFlow::Type::RoomMsg && model_) {
        req.to      = this->toClient.to_string();
        req.msgtype = "m.key.verification.request";
        // clang-format off
        // clang-format < 12 is buggy on this
        req.body    = "User is requesting to verify keys with you. However, your client does "
                      "not support this method, so you will need to use the legacy method of "
                      "key verification.";
        // clang-format on
    }

    send(req);
    setState(WaitingForOtherToAccept);
}
//! cancels a verification flow
void
DeviceVerificationFlow::cancelVerification(DeviceVerificationFlow::Error error_code)
{
    if (state_ == State::Success || state_ == State::Failed)
        return;

    mtx::events::msg::KeyVerificationCancel req;

    if (error_code == DeviceVerificationFlow::Error::UnknownMethod) {
        req.code   = "m.unknown_method";
        req.reason = "unknown method received";
    } else if (error_code == DeviceVerificationFlow::Error::MismatchedCommitment) {
        req.code   = "m.mismatched_commitment";
        req.reason = "commitment didn't match";
    } else if (error_code == DeviceVerificationFlow::Error::MismatchedSAS) {
        req.code   = "m.mismatched_sas";
        req.reason = "sas didn't match";
    } else if (error_code == DeviceVerificationFlow::Error::KeyMismatch) {
        req.code   = "m.key_match";
        req.reason = "keys did not match";
    } else if (error_code == DeviceVerificationFlow::Error::Timeout) {
        req.code   = "m.timeout";
        req.reason = "timed out";
    } else if (error_code == DeviceVerificationFlow::Error::User) {
        req.code   = "m.user";
        req.reason = "user cancelled the verification";
    } else if (error_code == DeviceVerificationFlow::Error::OutOfOrder) {
        req.code   = "m.unexpected_message";
        req.reason = "received messages out of order";
    }

    this->error_ = error_code;
    this->setState(Failed);
    emit errorChanged();

    send(req);
}
//! sends the verification key
void
DeviceVerificationFlow::sendVerificationKey()
{
    if (keySent)
        return;
    keySent = true;

    mtx::events::msg::KeyVerificationKey req;

    req.key = this->sas->public_key();

    send(req);
}

mtx::events::msg::KeyVerificationMac
key_verification_mac(mtx::crypto::SAS *sas,
                     mtx::identifiers::User sender,
                     const std::string &senderDevice,
                     mtx::identifiers::User receiver,
                     const std::string &receiverDevice,
                     const std::string &transactionId,
                     std::map<std::string, std::string> keys)
{
    mtx::events::msg::KeyVerificationMac req;

    std::string info = "MATRIX_KEY_VERIFICATION_MAC" + sender.to_string() + senderDevice +
                       receiver.to_string() + receiverDevice + transactionId;

    std::string key_list;
    bool first = true;
    for (const auto &[key_id, key] : keys) {
        req.mac[key_id] = sas->calculate_mac(key, info + key_id);

        if (!first)
            key_list += ",";
        key_list += key_id;
        first = false;
    }

    req.keys = sas->calculate_mac(key_list, info + "KEY_IDS");

    return req;
}

//! sends the mac of the keys
void
DeviceVerificationFlow::sendVerificationMac()
{
    if (macSent)
        return;
    macSent = true;

    std::map<std::string, std::string> key_list;
    key_list["ed25519:" + http::client()->device_id()] = olm::client()->identity_keys().ed25519;

    // send our master key, if we trust it
    if (!this->our_trusted_master_key.empty())
        key_list["ed25519:" + our_trusted_master_key] = our_trusted_master_key;

    mtx::events::msg::KeyVerificationMac req = key_verification_mac(sas.get(),
                                                                    http::client()->user_id(),
                                                                    http::client()->device_id(),
                                                                    this->toClient,
                                                                    this->deviceId.toStdString(),
                                                                    this->transaction_id,
                                                                    key_list);

    send(req);

    setState(WaitingForMac);
    acceptDevice();
}
//! Completes the verification flow
void
DeviceVerificationFlow::acceptDevice()
{
    if (!isMacVerified) {
        setState(WaitingForMac);
    } else if (state_ == WaitingForMac) {
        cache::markDeviceVerified(this->toClient.to_string(), this->deviceId.toStdString());
        this->sendVerificationDone();
        setState(Success);

        // Request secrets. We should probably check somehow, if a device knowns about the
        // secrets.
        if (utils::localUser().toStdString() == this->toClient.to_string() &&
            (!cache::secret(mtx::secret_storage::secrets::cross_signing_self_signing) ||
             !cache::secret(mtx::secret_storage::secrets::cross_signing_user_signing))) {
            olm::request_cross_signing_keys();
        }
    }
}

void
DeviceVerificationFlow::unverify()
{
    cache::markDeviceUnverified(this->toClient.to_string(), this->deviceId.toStdString());

    emit refreshProfile();
}

QSharedPointer<DeviceVerificationFlow>
DeviceVerificationFlow::NewInRoomVerification(QObject *parent_,
                                              TimelineModel *timelineModel_,
                                              const mtx::events::msg::KeyVerificationRequest &msg,
                                              QString other_user_,
                                              QString event_id_)
{
    QSharedPointer<DeviceVerificationFlow> flow(
      new DeviceVerificationFlow(parent_,
                                 Type::RoomMsg,
                                 timelineModel_,
                                 other_user_,
                                 {QString::fromStdString(msg.from_device)}));

    flow->setEventId(event_id_.toStdString());

    if (std::find(msg.methods.begin(),
                  msg.methods.end(),
                  mtx::events::msg::VerificationMethods::SASv1) == msg.methods.end()) {
        flow->cancelVerification(UnknownMethod);
    }

    return flow;
}
QSharedPointer<DeviceVerificationFlow>
DeviceVerificationFlow::NewToDeviceVerification(QObject *parent_,
                                                const mtx::events::msg::KeyVerificationRequest &msg,
                                                QString other_user_,
                                                QString txn_id_)
{
    QSharedPointer<DeviceVerificationFlow> flow(new DeviceVerificationFlow(
      parent_, Type::ToDevice, nullptr, other_user_, {QString::fromStdString(msg.from_device)}));
    flow->transaction_id = txn_id_.toStdString();

    if (std::find(msg.methods.begin(),
                  msg.methods.end(),
                  mtx::events::msg::VerificationMethods::SASv1) == msg.methods.end()) {
        flow->cancelVerification(UnknownMethod);
    }

    return flow;
}
QSharedPointer<DeviceVerificationFlow>
DeviceVerificationFlow::NewToDeviceVerification(QObject *parent_,
                                                const mtx::events::msg::KeyVerificationStart &msg,
                                                QString other_user_,
                                                QString txn_id_)
{
    QSharedPointer<DeviceVerificationFlow> flow(new DeviceVerificationFlow(
      parent_, Type::ToDevice, nullptr, other_user_, {QString::fromStdString(msg.from_device)}));
    flow->transaction_id = txn_id_.toStdString();

    flow->handleStartMessage(msg, "");

    return flow;
}
QSharedPointer<DeviceVerificationFlow>
DeviceVerificationFlow::InitiateUserVerification(QObject *parent_,
                                                 TimelineModel *timelineModel_,
                                                 QString userid)
{
    QSharedPointer<DeviceVerificationFlow> flow(
      new DeviceVerificationFlow(parent_, Type::RoomMsg, timelineModel_, userid, {}));
    flow->sender = true;
    return flow;
}
QSharedPointer<DeviceVerificationFlow>
DeviceVerificationFlow::InitiateDeviceVerification(QObject *parent_,
                                                   QString userid,
                                                   std::vector<QString> devices)
{
    assert(!devices.empty());

    QSharedPointer<DeviceVerificationFlow> flow(
      new DeviceVerificationFlow(parent_, Type::ToDevice, nullptr, userid, devices));

    flow->sender         = true;
    flow->transaction_id = http::client()->generate_txn_id();

    return flow;
}
