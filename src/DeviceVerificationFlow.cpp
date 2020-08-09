#include "DeviceVerificationFlow.h"

#include "Cache.h"
#include "ChatPage.h"
#include "Logging.h"
#include "timeline/TimelineModel.h"

#include <QDateTime>
#include <QTimer>

#include <iostream>

static constexpr int TIMEOUT = 2 * 60 * 1000; // 2 minutes

namespace msgs = mtx::events::msg;

DeviceVerificationFlow::DeviceVerificationFlow(QObject *, DeviceVerificationFlow::Type flow_type)
  : type(flow_type)
{
        timeout = new QTimer(this);
        timeout->setSingleShot(true);
        this->sas           = olm::client()->sas_init();
        this->isMacVerified = false;

        connect(this->model_,
                &TimelineModel::updateFlowEventId,
                this,
                [this](std::string event_id) { this->relation.in_reply_to.event_id = event_id; });

        connect(timeout, &QTimer::timeout, this, [this]() {
                emit timedout();
                this->cancelVerification(DeviceVerificationFlow::Error::Timeout);
                this->deleteLater();
        });

        connect(
          ChatPage::instance(),
          &ChatPage::recievedDeviceVerificationStart,
          this,
          [this](const mtx::events::msg::KeyVerificationStart &msg, std::string) {
                  if (msg.transaction_id.has_value()) {
                          if (msg.transaction_id.value() != this->transaction_id)
                                  return;
                  } else if (msg.relates_to.has_value()) {
                          if (msg.relates_to.value().in_reply_to.event_id !=
                              this->relation.in_reply_to.event_id)
                                  return;
                  }
                  if ((std::find(msg.key_agreement_protocols.begin(),
                                 msg.key_agreement_protocols.end(),
                                 "curve25519-hkdf-sha256") != msg.key_agreement_protocols.end()) &&
                      (std::find(msg.hashes.begin(), msg.hashes.end(), "sha256") !=
                       msg.hashes.end()) &&
                      (std::find(msg.message_authentication_codes.begin(),
                                 msg.message_authentication_codes.end(),
                                 "hmac-sha256") != msg.message_authentication_codes.end())) {
                          if (std::find(msg.short_authentication_string.begin(),
                                        msg.short_authentication_string.end(),
                                        mtx::events::msg::SASMethods::Decimal) !=
                              msg.short_authentication_string.end()) {
                                  this->method = DeviceVerificationFlow::Method::Emoji;
                          } else if (std::find(msg.short_authentication_string.begin(),
                                               msg.short_authentication_string.end(),
                                               mtx::events::msg::SASMethods::Emoji) !=
                                     msg.short_authentication_string.end()) {
                                  this->method = DeviceVerificationFlow::Method::Decimal;
                          } else {
                                  this->cancelVerification(
                                    DeviceVerificationFlow::Error::UnknownMethod);
                                  return;
                          }
                          this->acceptVerificationRequest();
                          this->canonical_json = nlohmann::json(msg);
                  } else {
                          this->cancelVerification(DeviceVerificationFlow::Error::UnknownMethod);
                  }
          });

        connect(ChatPage::instance(),
                &ChatPage::recievedDeviceVerificationAccept,
                this,
                [this](const mtx::events::msg::KeyVerificationAccept &msg) {
                        if (msg.transaction_id.has_value()) {
                                if (msg.transaction_id.value() != this->transaction_id)
                                        return;
                        } else if (msg.relates_to.has_value()) {
                                if (msg.relates_to.value().in_reply_to.event_id !=
                                    this->relation.in_reply_to.event_id)
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
                                        this->method = DeviceVerificationFlow::Method::Emoji;
                                } else {
                                        this->method = DeviceVerificationFlow::Method::Decimal;
                                }
                                this->mac_method = msg.message_authentication_code;
                                this->sendVerificationKey();
                        } else {
                                this->cancelVerification(
                                  DeviceVerificationFlow::Error::UnknownMethod);
                        }
                });

        connect(ChatPage::instance(),
                &ChatPage::recievedDeviceVerificationCancel,
                this,
                [this](const mtx::events::msg::KeyVerificationCancel &msg) {
                        if (msg.transaction_id.has_value()) {
                                if (msg.transaction_id.value() != this->transaction_id)
                                        return;
                        } else if (msg.relates_to.has_value()) {
                                if (msg.relates_to.value().in_reply_to.event_id !=
                                    this->relation.in_reply_to.event_id)
                                        return;
                        }
                        emit verificationCanceled();
                });

        connect(ChatPage::instance(),
                &ChatPage::recievedDeviceVerificationKey,
                this,
                [this](const mtx::events::msg::KeyVerificationKey &msg) {
                        if (msg.transaction_id.has_value()) {
                                if (msg.transaction_id.value() != this->transaction_id)
                                        return;
                        } else if (msg.relates_to.has_value()) {
                                if (msg.relates_to.value().in_reply_to.event_id !=
                                    this->relation.in_reply_to.event_id)
                                        return;
                        }
                        this->sas->set_their_key(msg.key);
                        std::string info;
                        if (this->sender == true) {
                                info = "MATRIX_KEY_VERIFICATION_SAS|" +
                                       http::client()->user_id().to_string() + "|" +
                                       http::client()->device_id() + "|" + this->sas->public_key() +
                                       "|" + this->toClient.to_string() + "|" +
                                       this->deviceId.toStdString() + "|" + msg.key + "|" +
                                       this->transaction_id;
                        } else {
                                info = "MATRIX_KEY_VERIFICATION_SAS|" + this->toClient.to_string() +
                                       "|" + this->deviceId.toStdString() + "|" + msg.key + "|" +
                                       http::client()->user_id().to_string() + "|" +
                                       http::client()->device_id() + "|" + this->sas->public_key() +
                                       "|" + this->transaction_id;
                        }

                        if (this->method == DeviceVerificationFlow::Method::Emoji) {
                                this->sasList = this->sas->generate_bytes_emoji(info);
                        } else if (this->method == DeviceVerificationFlow::Method::Decimal) {
                                this->sasList = this->sas->generate_bytes_decimal(info);
                        }
                        if (this->sender == false) {
                                emit this->verificationRequestAccepted(this->method);
                                this->sendVerificationKey();
                        } else {
                                if (this->commitment ==
                                    mtx::crypto::bin2base64_unpadded(
                                      mtx::crypto::sha256(msg.key + this->canonical_json.dump()))) {
                                        emit this->verificationRequestAccepted(this->method);
                                } else {
                                        this->cancelVerification(
                                          DeviceVerificationFlow::Error::MismatchedCommitment);
                                }
                        }
                });

        connect(
          ChatPage::instance(),
          &ChatPage::recievedDeviceVerificationMac,
          this,
          [this](const mtx::events::msg::KeyVerificationMac &msg) {
                  if (msg.transaction_id.has_value()) {
                          if (msg.transaction_id.value() != this->transaction_id)
                                  return;
                  } else if (msg.relates_to.has_value()) {
                          if (msg.relates_to.value().in_reply_to.event_id !=
                              this->relation.in_reply_to.event_id)
                                  return;
                  }
                  std::string info = "MATRIX_KEY_VERIFICATION_MAC" + this->toClient.to_string() +
                                     this->deviceId.toStdString() +
                                     http::client()->user_id().to_string() +
                                     http::client()->device_id() + this->transaction_id;

                  std::vector<std::string> key_list;
                  std::string key_string;
                  for (auto mac : msg.mac) {
                          key_string += mac.first + ",";
                          if (device_keys[mac.first] != "") {
                                  if (mac.second ==
                                      this->sas->calculate_mac(this->device_keys[mac.first],
                                                               info + mac.first)) {
                                  } else {
                                          this->cancelVerification(
                                            DeviceVerificationFlow::Error::KeyMismatch);
                                          return;
                                  }
                          }
                  }
                  key_string = key_string.substr(0, key_string.length() - 1);
                  if (msg.keys == this->sas->calculate_mac(key_string, info + "KEY_IDS")) {
                          // uncomment this in future to be compatible with the
                          // MSC2366 this->sendVerificationDone(); and remove the
                          // below line
                          if (this->isMacVerified == true) {
                                  this->acceptDevice();
                          } else
                                  this->isMacVerified = true;
                  } else {
                          this->cancelVerification(DeviceVerificationFlow::Error::KeyMismatch);
                  }
          });

        connect(ChatPage::instance(),
                &ChatPage::recievedDeviceVerificationReady,
                this,
                [this](const mtx::events::msg::KeyVerificationReady &msg) {
                        if (msg.transaction_id.has_value()) {
                                if (msg.transaction_id.value() != this->transaction_id)
                                        return;
                        } else if (msg.relates_to.has_value()) {
                                // this is just a workaround
                                this->relation.in_reply_to.event_id =
                                  msg.relates_to.value().in_reply_to.event_id;
                                if (msg.relates_to.value().in_reply_to.event_id !=
                                    this->relation.in_reply_to.event_id)
                                        return;
                        }
                        this->startVerificationRequest();
                });

        connect(ChatPage::instance(),
                &ChatPage::recievedDeviceVerificationDone,
                this,
                [this](const mtx::events::msg::KeyVerificationDone &msg) {
                        if (msg.transaction_id.has_value()) {
                                if (msg.transaction_id.value() != this->transaction_id)
                                        return;
                        } else if (msg.relates_to.has_value()) {
                                if (msg.relates_to.value().in_reply_to.event_id !=
                                    this->relation.in_reply_to.event_id)
                                        return;
                        }
                        this->acceptDevice();
                });

        timeout->start(TIMEOUT);
}

QString
DeviceVerificationFlow::getTransactionId()
{
        return QString::fromStdString(this->transaction_id);
}

QString
DeviceVerificationFlow::getUserId()
{
        return this->userId;
}

QString
DeviceVerificationFlow::getDeviceId()
{
        return this->deviceId;
}

DeviceVerificationFlow::Method
DeviceVerificationFlow::getMethod()
{
        return this->method;
}

DeviceVerificationFlow::Type
DeviceVerificationFlow::getType()
{
        return this->type;
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

void
DeviceVerificationFlow::setModel(TimelineModel *&model)
{
        this->model_ = model;
}

void
DeviceVerificationFlow::setTransactionId(QString transaction_id_)
{
        this->transaction_id = transaction_id_.toStdString();
}

void
DeviceVerificationFlow::setUserId(QString userID)
{
        this->userId    = userID;
        this->toClient  = mtx::identifiers::parse<mtx::identifiers::User>(userID.toStdString());
        auto user_cache = cache::getUserCache(userID.toStdString());

        if (user_cache.has_value()) {
                this->callback_fn(user_cache->keys, {}, userID.toStdString());
        } else {
                mtx::requests::QueryKeys req;
                req.device_keys[userID.toStdString()] = {};
                http::client()->query_keys(
                  req,
                  [user_id = userID.toStdString(), this](const mtx::responses::QueryKeys &res,
                                                         mtx::http::RequestErr err) {
                          this->callback_fn(res, err, user_id);
                  });
        }
}

void
DeviceVerificationFlow::setDeviceId(QString deviceID)
{
        this->deviceId = deviceID;
}

void
DeviceVerificationFlow::setMethod(DeviceVerificationFlow::Method method_)
{
        this->method = method_;
}

void
DeviceVerificationFlow::setType(Type type)
{
        this->type = type;
}

void
DeviceVerificationFlow::setSender(bool sender_)
{
        this->sender         = sender_;
        this->transaction_id = http::client()->generate_txn_id();
}

void
DeviceVerificationFlow::setEventId(std::string event_id)
{
        this->relation.in_reply_to.event_id = event_id;
        this->transaction_id                = event_id;
}

//! accepts a verification
void
DeviceVerificationFlow::acceptVerificationRequest()
{
        mtx::events::msg::KeyVerificationAccept req;

        req.method                      = mtx::events::msg::VerificationMethods::SASv1;
        req.key_agreement_protocol      = "curve25519-hkdf-sha256";
        req.hash                        = "sha256";
        req.message_authentication_code = "hkdf-hmac-sha256";
        if (this->method == DeviceVerificationFlow::Method::Emoji)
                req.short_authentication_string = {mtx::events::msg::SASMethods::Emoji};
        else if (this->method == DeviceVerificationFlow::Method::Decimal)
                req.short_authentication_string = {mtx::events::msg::SASMethods::Decimal};
        req.commitment = mtx::crypto::bin2base64_unpadded(
          mtx::crypto::sha256(this->sas->public_key() + this->canonical_json.dump()));

        if (this->type == DeviceVerificationFlow::Type::ToDevice) {
                mtx::requests::ToDeviceMessages<mtx::events::msg::KeyVerificationAccept> body;
                req.transaction_id = this->transaction_id;

                body[this->toClient][this->deviceId.toStdString()] = req;

                http::client()->send_to_device<mtx::events::msg::KeyVerificationAccept>(
                  this->transaction_id, body, [](mtx::http::RequestErr err) {
                          if (err)
                                  nhlog::net()->warn("failed to accept verification request: {} {}",
                                                     err->matrix_error.error,
                                                     static_cast<int>(err->status_code));
                  });
        } else if (this->type == DeviceVerificationFlow::Type::RoomMsg && model_) {
                req.relates_to = this->relation;
                (model_)->sendMessage(req);
        }
}
//! responds verification request
void
DeviceVerificationFlow::sendVerificationReady()
{
        mtx::events::msg::KeyVerificationReady req;

        req.from_device = http::client()->device_id();
        req.methods     = {mtx::events::msg::VerificationMethods::SASv1};

        if (this->type == DeviceVerificationFlow::Type::ToDevice) {
                req.transaction_id = this->transaction_id;
                mtx::requests::ToDeviceMessages<mtx::events::msg::KeyVerificationReady> body;

                body[this->toClient][this->deviceId.toStdString()] = req;

                http::client()->send_to_device<mtx::events::msg::KeyVerificationReady>(
                  this->transaction_id, body, [](mtx::http::RequestErr err) {
                          if (err)
                                  nhlog::net()->warn("failed to send verification ready: {} {}",
                                                     err->matrix_error.error,
                                                     static_cast<int>(err->status_code));
                  });
        } else if (this->type == DeviceVerificationFlow::Type::RoomMsg && model_) {
                req.relates_to = this->relation;
                (model_)->sendMessage(req);
        }
}
//! accepts a verification
void
DeviceVerificationFlow::sendVerificationDone()
{
        mtx::events::msg::KeyVerificationDone req;

        if (this->type == DeviceVerificationFlow::Type::ToDevice) {
                mtx::requests::ToDeviceMessages<mtx::events::msg::KeyVerificationDone> body;
                req.transaction_id = this->transaction_id;

                body[this->toClient][this->deviceId.toStdString()] = req;

                http::client()->send_to_device<mtx::events::msg::KeyVerificationDone>(
                  this->transaction_id, body, [](mtx::http::RequestErr err) {
                          if (err)
                                  nhlog::net()->warn("failed to send verification done: {} {}",
                                                     err->matrix_error.error,
                                                     static_cast<int>(err->status_code));
                  });
        } else if (this->type == DeviceVerificationFlow::Type::RoomMsg && model_) {
                req.relates_to = this->relation;
                (model_)->sendMessage(req);
        }
}
//! starts the verification flow
void
DeviceVerificationFlow::startVerificationRequest()
{
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
                req.transaction_id                                 = this->transaction_id;
                this->canonical_json                               = nlohmann::json(req);
                body[this->toClient][this->deviceId.toStdString()] = req;

                http::client()->send_to_device<mtx::events::msg::KeyVerificationStart>(
                  this->transaction_id, body, [body](mtx::http::RequestErr err) {
                          if (err)
                                  nhlog::net()->warn("failed to start verification request: {} {}",
                                                     err->matrix_error.error,
                                                     static_cast<int>(err->status_code));
                  });
        } else if (this->type == DeviceVerificationFlow::Type::RoomMsg && model_) {
                req.relates_to = this->relation;
                (model_)->sendMessage(req);
        }
}
//! sends a verification request
void
DeviceVerificationFlow::sendVerificationRequest()
{
        mtx::events::msg::KeyVerificationRequest req;

        req.from_device = http::client()->device_id();
        req.methods.resize(1);
        req.methods[0] = mtx::events::msg::VerificationMethods::SASv1;

        if (this->type == DeviceVerificationFlow::Type::ToDevice) {
                QDateTime CurrentTime = QDateTime::currentDateTimeUtc();

                req.transaction_id = this->transaction_id;
                req.timestamp      = (uint64_t)CurrentTime.toTime_t();

                mtx::requests::ToDeviceMessages<mtx::events::msg::KeyVerificationRequest> body;

                body[this->toClient][this->deviceId.toStdString()] = req;

                http::client()->send_to_device<mtx::events::msg::KeyVerificationRequest>(
                  this->transaction_id, body, [](mtx::http::RequestErr err) {
                          if (err)
                                  nhlog::net()->warn("failed to send verification request: {} {}",
                                                     err->matrix_error.error,
                                                     static_cast<int>(err->status_code));
                  });
        } else if (this->type == DeviceVerificationFlow::Type::RoomMsg && model_) {
                req.to      = this->userId.toStdString();
                req.msgtype = "m.key.verification.request";
                req.body = "User is requesting to verify keys with you. However, your client does "
                           "not support this method, so you will need to use the legacy method of "
                           "key verification.";
                (model_)->sendMessage(req);
        }
}
//! cancels a verification flow
void
DeviceVerificationFlow::cancelVerification(DeviceVerificationFlow::Error error_code)
{
        mtx::events::msg::KeyVerificationCancel req;

        if (error_code == DeviceVerificationFlow::Error::UnknownMethod) {
                req.code   = "m.unknown_method";
                req.reason = "unknown method recieved";
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
        }

        emit this->verificationCanceled();

        if (this->type == DeviceVerificationFlow::Type::ToDevice) {
                req.transaction_id = this->transaction_id;
                mtx::requests::ToDeviceMessages<mtx::events::msg::KeyVerificationCancel> body;

                body[this->toClient][deviceId.toStdString()] = req;

                http::client()->send_to_device<mtx::events::msg::KeyVerificationCancel>(
                  this->transaction_id, body, [this](mtx::http::RequestErr err) {
                          if (err)
                                  nhlog::net()->warn("failed to cancel verification request: {} {}",
                                                     err->matrix_error.error,
                                                     static_cast<int>(err->status_code));

                          this->deleteLater();
                  });
        } else if (this->type == DeviceVerificationFlow::Type::RoomMsg && model_) {
                req.relates_to = this->relation;
                (model_)->sendMessage(req);
        }

        // TODO : Handle Blocking user better
        // auto verified_cache = cache::getVerifiedCache(this->userId.toStdString());
        //     if (verified_cache.has_value()) {
        //             verified_cache->device_blocked.push_back(this->deviceId.toStdString());
        //             cache::setVerifiedCache(this->userId.toStdString(),
        //                                     verified_cache.value());
        //     } else {
        //             cache::setVerifiedCache(
        //               this->userId.toStdString(),
        //               DeviceVerifiedCache{{}, {}, {this->deviceId.toStdString()}});
        //     }
}
//! sends the verification key
void
DeviceVerificationFlow::sendVerificationKey()
{
        mtx::events::msg::KeyVerificationKey req;

        req.key = this->sas->public_key();

        if (this->type == DeviceVerificationFlow::Type::ToDevice) {
                mtx::requests::ToDeviceMessages<mtx::events::msg::KeyVerificationKey> body;
                req.transaction_id = this->transaction_id;

                body[this->toClient][deviceId.toStdString()] = req;

                http::client()->send_to_device<mtx::events::msg::KeyVerificationKey>(
                  this->transaction_id, body, [](mtx::http::RequestErr err) {
                          if (err)
                                  nhlog::net()->warn("failed to send verification key: {} {}",
                                                     err->matrix_error.error,
                                                     static_cast<int>(err->status_code));
                  });
        } else if (this->type == DeviceVerificationFlow::Type::RoomMsg && model_) {
                req.relates_to = this->relation;
                (model_)->sendMessage(req);
        }
}
//! sends the mac of the keys
void
DeviceVerificationFlow::sendVerificationMac()
{
        mtx::events::msg::KeyVerificationMac req;

        std::string info = "MATRIX_KEY_VERIFICATION_MAC" + http::client()->user_id().to_string() +
                           http::client()->device_id() + this->toClient.to_string() +
                           this->deviceId.toStdString() + this->transaction_id;

        //! this vector stores the type of the key and the key
        std::vector<std::pair<std::string, std::string>> key_list;
        key_list.push_back(make_pair("ed25519", olm::client()->identity_keys().ed25519));
        std::sort(key_list.begin(), key_list.end());
        for (auto x : key_list) {
                req.mac.insert(
                  std::make_pair(x.first + ":" + http::client()->device_id(),
                                 this->sas->calculate_mac(
                                   x.second, info + x.first + ":" + http::client()->device_id())));
                req.keys += x.first + ":" + http::client()->device_id() + ",";
        }

        req.keys =
          this->sas->calculate_mac(req.keys.substr(0, req.keys.size() - 1), info + "KEY_IDS");

        if (this->type == DeviceVerificationFlow::Type::ToDevice) {
                mtx::requests::ToDeviceMessages<mtx::events::msg::KeyVerificationMac> body;
                req.transaction_id                           = this->transaction_id;
                body[this->toClient][deviceId.toStdString()] = req;

                http::client()->send_to_device<mtx::events::msg::KeyVerificationMac>(
                  this->transaction_id, body, [this](mtx::http::RequestErr err) {
                          if (err)
                                  nhlog::net()->warn("failed to send verification MAC: {} {}",
                                                     err->matrix_error.error,
                                                     static_cast<int>(err->status_code));

                          if (this->isMacVerified == true)
                                  this->acceptDevice();
                          else
                                  this->isMacVerified = true;
                  });
        } else if (this->type == DeviceVerificationFlow::Type::RoomMsg && model_) {
                req.relates_to = this->relation;
                (model_)->sendMessage(req);
        }
}
//! Completes the verification flow
void
DeviceVerificationFlow::acceptDevice()
{
        auto verified_cache = cache::getVerifiedCache(this->userId.toStdString());
        if (verified_cache.has_value()) {
                verified_cache->device_verified.push_back(this->deviceId.toStdString());
                verified_cache->device_blocked.erase(
                  std::remove(verified_cache->device_blocked.begin(),
                              verified_cache->device_blocked.end(),
                              this->deviceId.toStdString()),
                  verified_cache->device_blocked.end());
        } else {
                cache::setVerifiedCache(
                  this->userId.toStdString(),
                  DeviceVerifiedCache{{this->deviceId.toStdString()}, {}, {}});
        }

        emit deviceVerified();
        emit refreshProfile();
        this->deleteLater();
}
//! callback function to keep track of devices
void
DeviceVerificationFlow::callback_fn(const mtx::responses::QueryKeys &res,
                                    mtx::http::RequestErr err,
                                    std::string user_id)
{
        if (err) {
                nhlog::net()->warn("failed to query device keys: {},{}",
                                   err->matrix_error.errcode,
                                   static_cast<int>(err->status_code));
                return;
        }

        if (res.device_keys.empty() || (res.device_keys.find(user_id) == res.device_keys.end())) {
                nhlog::net()->warn("no devices retrieved {}", user_id);
                return;
        }

        for (auto x : res.device_keys) {
                for (auto y : x.second) {
                        auto z = y.second;
                        if (z.user_id == user_id && z.device_id == this->deviceId.toStdString()) {
                                for (auto a : z.keys) {
                                        // TODO: Verify Signatures
                                        this->device_keys[a.first] = a.second;
                                }
                        }
                }
        }
}

void
DeviceVerificationFlow::unverify()
{
        auto verified_cache = cache::getVerifiedCache(this->userId.toStdString());
        if (verified_cache.has_value()) {
                auto it = std::remove(verified_cache->device_verified.begin(),
                                      verified_cache->device_verified.end(),
                                      this->deviceId.toStdString());
                verified_cache->device_verified.erase(it);
                cache::setVerifiedCache(this->userId.toStdString(), verified_cache.value());
        }

        emit refreshProfile();
}
