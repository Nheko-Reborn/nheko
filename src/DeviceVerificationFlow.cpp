#include "DeviceVerificationFlow.h"
#include "ChatPage.h"
#include "Logging.h"

#include <QDateTime>
#include <QTimer>

static constexpr int TIMEOUT = 2 * 60 * 1000; // 2 minutes

namespace msgs = mtx::events::msg;

DeviceVerificationFlow::DeviceVerificationFlow(QObject *)
{
        timeout = new QTimer(this);
        timeout->setSingleShot(true);
        this->sas = olm::client()->sas_init();
        connect(timeout, &QTimer::timeout, this, [this]() {
                emit timedout();
                this->deleteLater();
        });

        connect(ChatPage::instance(),
                &ChatPage::recievedDeviceVerificationStart,
                this,
                [this](const mtx::events::collections::DeviceEvents &message) {
                        auto msg =
                          std::get<mtx::events::DeviceEvent<msgs::KeyVerificationStart>>(message);
                        if (msg.content.transaction_id == this->transaction_id) {
                                if ((std::find(msg.content.key_agreement_protocols.begin(),
                                               msg.content.key_agreement_protocols.end(),
                                               "curve25519-hkdf-sha256") !=
                                     msg.content.key_agreement_protocols.end()) &&
                                    (std::find(msg.content.hashes.begin(),
                                               msg.content.hashes.end(),
                                               "sha256") != msg.content.hashes.end()) &&
                                    (std::find(msg.content.message_authentication_codes.begin(),
                                               msg.content.message_authentication_codes.end(),
                                               "hmac-sha256") !=
                                     msg.content.message_authentication_codes.end()) &&
                                    ((std::find(msg.content.short_authentication_string.begin(),
                                                msg.content.short_authentication_string.end(),
                                                mtx::events::msg::SASMethods::Decimal) !=
                                      msg.content.short_authentication_string.end()) ||
                                     (std::find(msg.content.short_authentication_string.begin(),
                                                msg.content.short_authentication_string.end(),
                                                mtx::events::msg::SASMethods::Emoji) !=
                                      msg.content.short_authentication_string.end()))) {
                                        this->acceptVerificationRequest();
                                        this->canonical_json = nlohmann::json(msg.content);
                                } else {
                                        this->cancelVerification();
                                }
                        }
                });
        connect(
          ChatPage::instance(),
          &ChatPage::recievedDeviceVerificationAccept,
          this,
          [this](const mtx::events::collections::DeviceEvents &message) {
                  auto msg =
                    std::get<mtx::events::DeviceEvent<msgs::KeyVerificationAccept>>(message);
                  if (msg.content.transaction_id == this->transaction_id) {
                          if ((msg.content.key_agreement_protocol == "curve25519-hkdf-sha256") &&
                              (msg.content.hash == "sha256") &&
                              (msg.content.message_authentication_code == "hkdf-hmac-sha256")) {
                                  this->commitment = msg.content.commitment;
                                  if (std::find(msg.content.short_authentication_string.begin(),
                                                msg.content.short_authentication_string.end(),
                                                mtx::events::msg::SASMethods::Emoji) !=
                                      msg.content.short_authentication_string.end()) {
                                          this->method = DeviceVerificationFlow::Method::Emoji;
                                  } else {
                                          this->method = DeviceVerificationFlow::Method::Decimal;
                                  }
                                  this->mac_method = msg.content.message_authentication_code;
                                  this->sendVerificationKey();
                          } else {
                                  this->cancelVerification();
                          }
                  }
          });
        connect(ChatPage::instance(),
                &ChatPage::recievedDeviceVerificationCancel,
                this,
                [this](const mtx::events::collections::DeviceEvents &message) {
                        auto msg =
                          std::get<mtx::events::DeviceEvent<msgs::KeyVerificationCancel>>(message);
                        if (msg.content.transaction_id == this->transaction_id) {
                                emit verificationCanceled();
                        }
                });
        connect(
          ChatPage::instance(),
          &ChatPage::recievedDeviceVerificationKey,
          this,
          [this](const mtx::events::collections::DeviceEvents &message) {
                  auto msg = std::get<mtx::events::DeviceEvent<msgs::KeyVerificationKey>>(message);
                  if (msg.content.transaction_id == this->transaction_id) {
                          this->sas->set_their_key(msg.content.key);
                          std::string info;
                          if (this->sender == true) {
                                  info = "MATRIX_KEY_VERIFICATION_SAS|" +
                                         http::client()->user_id().to_string() + "|" +
                                         http::client()->device_id() + "|" +
                                         this->sas->public_key() + "|" +
                                         this->toClient.to_string() + "|" +
                                         this->deviceId.toStdString() + "|" + msg.content.key +
                                         "|" + this->transaction_id;
                          } else {
                                  info = "MATRIX_KEY_VERIFICATION_SAS|" +
                                         this->toClient.to_string() + "|" +
                                         this->deviceId.toStdString() + "|" + msg.content.key +
                                         "|" + http::client()->user_id().to_string() + "|" +
                                         http::client()->device_id() + "|" +
                                         this->sas->public_key() + "|" + this->transaction_id;
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
                                      mtx::crypto::bin2base64_unpadded(mtx::crypto::sha256(
                                        msg.content.key + this->canonical_json.dump()))) {
                                          emit this->verificationRequestAccepted(this->method);
                                  } else {
                                          this->cancelVerification();
                                  }
                          }
                  }
          });
        connect(ChatPage::instance(),
                &ChatPage::recievedDeviceVerificationMac,
                this,
                [this](const mtx::events::collections::DeviceEvents &message) {
                        auto msg =
                          std::get<mtx::events::DeviceEvent<msgs::KeyVerificationMac>>(message);
                        if (msg.content.transaction_id == this->transaction_id) {
                                std::string info =
                                  "MATRIX_KEY_VERIFICATION_MAC" + this->toClient.to_string() +
                                  this->deviceId.toStdString() +
                                  http::client()->user_id().to_string() +
                                  http::client()->device_id() + this->transaction_id;

                                std::vector<std::string> key_list;
                                std::string key_string;
                                for (auto mac : msg.content.mac) {
                                        if (mac.second ==
                                            this->sas->calculate_mac(this->device_keys[mac.first],
                                                                     info + mac.first)) {
                                                key_string += mac.first;
                                        } else {
                                                this->cancelVerification();
                                                return;
                                        }
                                }
                                if (msg.content.keys ==
                                    this->sas->calculate_mac(key_string, info + "KEY_IDS")) {
                                        // uncomment this in future to be compatible with the
                                        // MSC2366 this->sendVerificationDone(); and remoeve the
                                        // below line
                                        if (this->isMacVerified == true)
                                                emit this->deviceVerified();
                                        else
                                                this->isMacVerified = true;
                                } else {
                                        this->cancelVerification();
                                }
                        }
                });
        connect(ChatPage::instance(),
                &ChatPage::recievedDeviceVerificationReady,
                this,
                [this](const mtx::events::collections::DeviceEvents &message) {
                        auto msg =
                          std::get<mtx::events::DeviceEvent<msgs::KeyVerificationReady>>(message);
                        if (msg.content.transaction_id == this->transaction_id) {
                                this->startVerificationRequest();
                        }
                });
        connect(ChatPage::instance(),
                &ChatPage::recievedDeviceVerificationDone,
                this,
                [this](const mtx::events::collections::DeviceEvents &message) {
                        auto msg =
                          std::get<mtx::events::DeviceEvent<msgs::KeyVerificationDone>>(message);
                        if (msg.content.transaction_id == this->transaction_id) {
                                this->startVerificationRequest();
                                emit this->deviceVerified();
                        }
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
DeviceVerificationFlow::setTransactionId(QString transaction_id_)
{
        this->transaction_id = transaction_id_.toStdString();
}

void
DeviceVerificationFlow::setUserId(QString userID)
{
        this->userId   = userID;
        this->toClient = mtx::identifiers::parse<mtx::identifiers::User>(userID.toStdString());

        mtx::responses::QueryKeys res;
        mtx::requests::QueryKeys req;
        req.device_keys[userID.toStdString()] = {};
        http::client()->query_keys(
          req,
          [user_id = userID.toStdString(), this](const mtx::responses::QueryKeys &res,
                                                 mtx::http::RequestErr err) {
                  if (err) {
                          nhlog::net()->warn("failed to query device keys: {},{}",
                                             err->matrix_error.errcode,
                                             static_cast<int>(err->status_code));
                          return;
                  }

                  for (auto x : res.device_keys) {
                          for (auto y : x.second) {
                                  auto z = y.second;
                                  if (z.user_id == user_id &&
                                      z.device_id == this->deviceId.toStdString()) {
                                          for (auto a : z.keys) {
                                                  // TODO: Verify Signatures
                                                  this->device_keys[a.first] = a.second;
                                          }
                                  }
                          }
                  }
          });
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
DeviceVerificationFlow::setSender(bool sender_)
{
        this->sender = sender_;
        if (this->sender == true)
                this->transaction_id = http::client()->generate_txn_id();
}

//! accepts a verification
void
DeviceVerificationFlow::acceptVerificationRequest()
{
        mtx::requests::ToDeviceMessages<mtx::events::msg::KeyVerificationAccept> body;
        mtx::events::msg::KeyVerificationAccept req;

        req.transaction_id              = this->transaction_id;
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

        body[this->toClient][this->deviceId.toStdString()] = req;

        http::client()
          ->send_to_device<mtx::events::msg::KeyVerificationAccept,
                           mtx::events::EventType::KeyVerificationAccept>(
            this->transaction_id, body, [](mtx::http::RequestErr err) {
                    if (err)
                            nhlog::net()->warn("failed to accept verification request: {} {}",
                                               err->matrix_error.error,
                                               static_cast<int>(err->status_code));
            });
}
//! responds verification request
void
DeviceVerificationFlow::sendVerificationReady()
{
        mtx::requests::ToDeviceMessages<mtx::events::msg::KeyVerificationReady> body;
        mtx::events::msg::KeyVerificationReady req;

        req.from_device    = http::client()->device_id();
        req.transaction_id = this->transaction_id;
        req.methods        = {mtx::events::msg::VerificationMethods::SASv1};

        body[this->toClient][this->deviceId.toStdString()] = req;

        http::client()
          ->send_to_device<mtx::events::msg::KeyVerificationReady,
                           mtx::events::EventType::KeyVerificationReady>(
            this->transaction_id, body, [](mtx::http::RequestErr err) {
                    if (err)
                            nhlog::net()->warn("failed to send verification ready: {} {}",
                                               err->matrix_error.error,
                                               static_cast<int>(err->status_code));
            });
}
//! accepts a verification
void
DeviceVerificationFlow::sendVerificationDone()
{
        mtx::requests::ToDeviceMessages<mtx::events::msg::KeyVerificationDone> body;
        mtx::events::msg::KeyVerificationDone req;

        req.transaction_id = this->transaction_id;

        body[this->toClient][this->deviceId.toStdString()] = req;

        http::client()
          ->send_to_device<mtx::events::msg::KeyVerificationDone,
                           mtx::events::EventType::KeyVerificationDone>(
            this->transaction_id, body, [](mtx::http::RequestErr err) {
                    if (err)
                            nhlog::net()->warn("failed to send verification done: {} {}",
                                               err->matrix_error.error,
                                               static_cast<int>(err->status_code));
            });
}
//! starts the verification flow
void
DeviceVerificationFlow::startVerificationRequest()
{
        mtx::requests::ToDeviceMessages<mtx::events::msg::KeyVerificationStart> body;
        mtx::events::msg::KeyVerificationStart req;

        req.from_device                  = http::client()->device_id();
        req.transaction_id               = this->transaction_id;
        req.method                       = mtx::events::msg::VerificationMethods::SASv1;
        req.key_agreement_protocols      = {"curve25519-hkdf-sha256"};
        req.hashes                       = {"sha256"};
        req.message_authentication_codes = {"hkdf-hmac-sha256", "hmac-sha256"};
        req.short_authentication_string  = {mtx::events::msg::SASMethods::Decimal,
                                           mtx::events::msg::SASMethods::Emoji};

        body[this->toClient][this->deviceId.toStdString()] = req;
        this->canonical_json                               = nlohmann::json(req);

        http::client()
          ->send_to_device<mtx::events::msg::KeyVerificationStart,
                           mtx::events::EventType::KeyVerificationStart>(
            this->transaction_id, body, [body](mtx::http::RequestErr err) {
                    if (err)
                            nhlog::net()->warn("failed to start verification request: {} {}",
                                               err->matrix_error.error,
                                               static_cast<int>(err->status_code));
            });
}
//! sends a verification request
void
DeviceVerificationFlow::sendVerificationRequest()
{
        QDateTime CurrentTime = QDateTime::currentDateTimeUtc();

        mtx::requests::ToDeviceMessages<mtx::events::msg::KeyVerificationRequest> body;
        mtx::events::msg::KeyVerificationRequest req;

        req.from_device    = http::client()->device_id();
        req.transaction_id = this->transaction_id;
        req.methods.resize(1);
        req.methods[0] = mtx::events::msg::VerificationMethods::SASv1;
        req.timestamp  = (uint64_t)CurrentTime.toTime_t();

        body[this->toClient][this->deviceId.toStdString()] = req;

        http::client()
          ->send_to_device<mtx::events::msg::KeyVerificationRequest,
                           mtx::events::EventType::KeyVerificationRequest>(
            this->transaction_id, body, [](mtx::http::RequestErr err) {
                    if (err)
                            nhlog::net()->warn("failed to send verification request: {} {}",
                                               err->matrix_error.error,
                                               static_cast<int>(err->status_code));
            });
}
//! cancels a verification flow
void
DeviceVerificationFlow::cancelVerification()
{
        mtx::requests::ToDeviceMessages<mtx::events::msg::KeyVerificationCancel> body;
        mtx::events::msg::KeyVerificationCancel req;

        req.transaction_id = this->transaction_id;
        // TODO: Add Proper Error Messages and Code
        req.reason = "Device Verification Cancelled";
        req.code   = "400";

        body[this->toClient][deviceId.toStdString()] = req;

        emit this->verificationCanceled();

        http::client()
          ->send_to_device<mtx::events::msg::KeyVerificationCancel,
                           mtx::events::EventType::KeyVerificationCancel>(
            this->transaction_id, body, [this](mtx::http::RequestErr err) {
                    if (err)
                            nhlog::net()->warn("failed to cancel verification request: {} {}",
                                               err->matrix_error.error,
                                               static_cast<int>(err->status_code));
                    this->deleteLater();
            });
}
//! sends the verification key
void
DeviceVerificationFlow::sendVerificationKey()
{
        mtx::requests::ToDeviceMessages<mtx::events::msg::KeyVerificationKey> body;
        mtx::events::msg::KeyVerificationKey req;

        req.key            = this->sas->public_key();
        req.transaction_id = this->transaction_id;

        body[this->toClient][deviceId.toStdString()] = req;

        http::client()
          ->send_to_device<mtx::events::msg::KeyVerificationKey,
                           mtx::events::EventType::KeyVerificationKey>(
            this->transaction_id, body, [](mtx::http::RequestErr err) {
                    if (err)
                            nhlog::net()->warn("failed to send verification key: {} {}",
                                               err->matrix_error.error,
                                               static_cast<int>(err->status_code));
            });
}
//! sends the mac of the keys
void
DeviceVerificationFlow::sendVerificationMac()
{
        mtx::requests::ToDeviceMessages<mtx::events::msg::KeyVerificationMac> body;
        mtx::events::msg::KeyVerificationMac req;

        std::string info = "MATRIX_KEY_VERIFICATION_MAC" + http::client()->user_id().to_string() +
                           http::client()->device_id() + this->toClient.to_string() +
                           this->deviceId.toStdString() + this->transaction_id;

        req.transaction_id = this->transaction_id;
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

        body[this->toClient][deviceId.toStdString()] = req;

        http::client()
          ->send_to_device<mtx::events::msg::KeyVerificationMac,
                           mtx::events::EventType::KeyVerificationMac>(
            this->transaction_id, body, [this](mtx::http::RequestErr err) {
                    if (err)
                            nhlog::net()->warn("failed to send verification MAC: {} {}",
                                               err->matrix_error.error,
                                               static_cast<int>(err->status_code));

                    if (this->isMacVerified == true)
                            emit this->deviceVerified();
                    else
                            this->isMacVerified = true;
            });
}
//! Completes the verification flow
void
DeviceVerificationFlow::acceptDevice()
{
        emit deviceVerified();
        this->deleteLater();

        // Yet to add send to_device message
}
