#include "DeviceVerificationFlow.h"
#include "ChatPage.h"
#include "Logging.h"

#include <QDateTime>
#include <QDebug> // only for debugging
#include <QTimer>
#include <iostream> // only for debugging

static constexpr int TIMEOUT = 2 * 60 * 1000; // 2 minutes

namespace msgs = mtx::events::msg;

DeviceVerificationFlow::DeviceVerificationFlow(QObject *)
{
        timeout = new QTimer(this);
        timeout->setSingleShot(true);
        if (this->sender == true)
                this->transaction_id = http::client()->generate_txn_id();
        connect(timeout, &QTimer::timeout, this, [this]() {
                emit timedout();
                this->deleteLater();
        });
        connect(ChatPage::instance(),
                &ChatPage::recievedDeviceVerificationAccept,
                this,
                [this](const mtx::events::collections::DeviceEvents &message) {
                        auto msg =
                          std::get<mtx::events::DeviceEvent<msgs::KeyVerificationAccept>>(message);
                        if (msg.content.transaction_id == this->transaction_id) {
                                std::cout << "Recieved Event Accept" << std::endl;
                        }
                });
        connect(ChatPage::instance(),
                &ChatPage::recievedDeviceVerificationRequest,
                this,
                [this](const mtx::events::collections::DeviceEvents &message) {
                        auto msg =
                          std::get<mtx::events::DeviceEvent<msgs::KeyVerificationRequest>>(message);
                        if (msg.content.transaction_id == this->transaction_id) {
                                std::cout << "Recieved Event Request" << std::endl;
                        }
                });
        connect(ChatPage::instance(),
                &ChatPage::recievedDeviceVerificationCancel,
                this,
                [this](const mtx::events::collections::DeviceEvents &message) {
                        auto msg =
                          std::get<mtx::events::DeviceEvent<msgs::KeyVerificationCancel>>(message);
                        if (msg.content.transaction_id == this->transaction_id) {
                                std::cout << "Recieved Event Cancel" << std::endl;
                        }
                });
        connect(ChatPage::instance(),
                &ChatPage::recievedDeviceVerificationKey,
                this,
                [this](const mtx::events::collections::DeviceEvents &message) {
                        auto msg =
                          std::get<mtx::events::DeviceEvent<msgs::KeyVerificationKey>>(message);
                        if (msg.content.transaction_id == this->transaction_id) {
                                std::cout << "Recieved Event Key" << std::endl;
                        }
                });
        connect(ChatPage::instance(),
                &ChatPage::recievedDeviceVerificationMac,
                this,
                [this](const mtx::events::collections::DeviceEvents &message) {
                        auto msg =
                          std::get<mtx::events::DeviceEvent<msgs::KeyVerificationMac>>(message);
                        if (msg.content.transaction_id == this->transaction_id) {
                                std::cout << "Recieved Event Mac" << std::endl;
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
        req.short_authentication_string = {mtx::events::msg::SASMethods::Decimal,
                                           mtx::events::msg::SASMethods::Emoji};
        req.commitment                  = "";

        emit this->verificationRequestAccepted(this->method);

        body[this->toClient][this->deviceId.toStdString()] = req;

        http::client()
          ->send_to_device<mtx::events::msg::KeyVerificationAccept,
                           mtx::events::EventType::KeyVerificationAccept>(
            this->transaction_id, body, [this](mtx::http::RequestErr err) {
                    if (err)
                            nhlog::net()->warn("failed to accept verification request: {} {}",
                                               err->matrix_error.error,
                                               static_cast<int>(err->status_code));
                    emit this->verificationRequestAccepted(rand() % 2 ? Emoji : Decimal);
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

        http::client()
          ->send_to_device<mtx::events::msg::KeyVerificationStart,
                           mtx::events::EventType::KeyVerificationStart>(
            this->transaction_id, body, [body](mtx::http::RequestErr err) {
                    if (err)
                            nhlog::net()->warn("failed to start verification request: {} {}",
                                               err->matrix_error.error,
                                               static_cast<int>(err->status_code));
                    std::cout << nlohmann::json(body).dump(2) << std::endl;
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

        req.key            = "";
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

        req.transaction_id = this->transaction_id;
        // req.mac = "";
        req.keys = "";

        body[this->toClient][deviceId.toStdString()] = req;

        http::client()
          ->send_to_device<mtx::events::msg::KeyVerificationMac,
                           mtx::events::EventType::KeyVerificationMac>(
            this->transaction_id, body, [](mtx::http::RequestErr err) {
                    if (err)
                            nhlog::net()->warn("failed to send verification MAC: {} {}",
                                               err->matrix_error.error,
                                               static_cast<int>(err->status_code));
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
