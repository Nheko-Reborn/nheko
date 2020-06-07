#include "DeviceVerificationFlow.h"

#include "Logging.h"
#include <QDateTime>
#include <QDebug> // only for debugging
#include <QTimer>
#include <iostream> // only for debugging

static constexpr int TIMEOUT = 2 * 60 * 1000; // 2 minutes

DeviceVerificationFlow::DeviceVerificationFlow(QObject *)
{
        timeout = new QTimer(this);
        timeout->setSingleShot(true);
        connect(timeout, &QTimer::timeout, this, [this]() {
                emit timedout();
                this->deleteLater();
        });
        timeout->start(TIMEOUT);
}

QString
DeviceVerificationFlow::getUserId()
{
        toClient = mtx::identifiers::parse<mtx::identifiers::User>((this->userId).toStdString());
        std::cout << http::client()->device_id() << std::endl;
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

void
DeviceVerificationFlow::setUserId(QString userID)
{
        this->userId = userID;
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

//! accepts a verification
void
DeviceVerificationFlow::acceptVerificationRequest()
{
        mtx::requests::ToDeviceMessages<mtx::events::msg::KeyVerificationAccept> body;
        mtx::events::msg::KeyVerificationAccept req;

        req.transaction_id              = this->transaction_id;
        req.method                      = mtx::events::msg::VerificationMethods::SASv1;
        req.key_agreement_protocol      = "curve25519";
        req.hash                        = "sha256";
        req.message_authentication_code = "";
        // req.short_authentication_string = "";
        req.commitment = "";

        emit this->verificationRequestAccepted(this->method);

        body[this->toClient][this->deviceId.toStdString()] = req;

        http::client()
          ->send_to_device<mtx::events::msg::KeyVerificationAccept,
                           mtx::events::EventType::KeyVerificationAccept>(
            "m.key.verification.accept", body, [](mtx::http::RequestErr err) {
                    if (err)
                            nhlog::net()->warn("failed to accept verification request: {} {}",
                                               err->matrix_error.error,
                                               static_cast<int>(err->status_code));
                    //     emit this->verificationRequestAccepted(rand() % 2 ? Emoji : Decimal);
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
        req.key_agreement_protocols      = {};
        req.hashes                       = {};
        req.message_authentication_codes = {};
        // req.short_authentication_string = "";

        body[this->toClient][this->deviceId.toStdString()] = req;

        http::client()
          ->send_to_device<mtx::events::msg::KeyVerificationStart,
                           mtx::events::EventType::KeyVerificationStart>(
            "m.key.verification.start", body, [](mtx::http::RequestErr err) {
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

        this->transaction_id = http::client()->generate_txn_id();

        req.from_device    = http::client()->device_id();
        req.transaction_id = this->transaction_id;
        req.methods.resize(1);
        req.methods[0] = mtx::events::msg::VerificationMethods::SASv1;
        req.timestamp  = (uint64_t)CurrentTime.toTime_t();

        body[this->toClient][this->deviceId.toStdString()] = req;

        http::client()
          ->send_to_device<mtx::events::msg::KeyVerificationRequest,
                           mtx::events::EventType::KeyVerificationRequest>(
            "m.key.verification.request", body, [](mtx::http::RequestErr err) {
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
            "m.key.verification.cancel", body, [this](mtx::http::RequestErr err) {
                    if (err)
                            nhlog::net()->warn("failed to cancel verification request: {} {}",
                                               err->matrix_error.error,
                                               static_cast<int>(err->status_code));
                    this->deleteLater();
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
