#include "DeviceVerificationFlow.h"

#include <MatrixClient.h>
#include <QDateTime>
#include <QDebug> // only for debugging
#include <QTimer>

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

std::string
DeviceVerificationFlow::generate_txn_id()
{
        this->transaction_id = mtx::client::utils::random_token(32, false);
        return this->transaction_id;
}

//! accepts a verification
void
DeviceVerificationFlow::acceptVerificationRequest()
{
        mtx::requests::ToDeviceMessages<mtx::events::msg::KeyVerificationAccept> body;
        mtx::events::msg::KeyVerificationAccept req;

        req.transaction_id              = this->transaction_id;
        req.method                      = mtx::events::msg::VerificationMethods::SASv1;
        req.key_agreement_protocol      = "";
        req.hash                        = "";
        req.message_authentication_code = "";
        // req.short_authentication_string = "";
        req.commitment = "";

        emit verificationRequestAccepted(rand() % 2 ? Emoji : Decimal);

        // Yet to add send to_device message
}
//! starts the verification flow
void
DeviceVerificationFlow::startVerificationRequest()
{
        mtx::requests::ToDeviceMessages<mtx::events::msg::KeyVerificationAccept> body;
        mtx::events::msg::KeyVerificationAccept req;

        // req.from_device = "";
        req.transaction_id         = this->transaction_id;
        req.method                 = mtx::events::msg::VerificationMethods::SASv1;
        req.key_agreement_protocol = {};
        // req.hashes = {};
        req.message_authentication_code = {};
        // req.short_authentication_string = "";

        // Yet to add send to_device message
}
//! sends a verification request
void
DeviceVerificationFlow::sendVerificationRequest()
{
        QDateTime CurrentTime = QDateTime::currentDateTimeUtc();

        mtx::requests::ToDeviceMessages<mtx::events::msg::KeyVerificationRequest> body;
        mtx::events::msg::KeyVerificationRequest req;

        req.from_device    = "";
        req.transaction_id = generate_txn_id();
        req.methods.resize(1);
        req.methods[0] = mtx::events::msg::VerificationMethods::SASv1;
        req.timestamp  = (uint64_t)CurrentTime.toTime_t();

        // Yet to add send to_device message
}
//! cancels a verification flow
void
DeviceVerificationFlow::cancelVerification()
{
        mtx::requests::ToDeviceMessages<mtx::events::msg::KeyVerificationCancel> body;
        mtx::events::msg::KeyVerificationCancel req;

        req.transaction_id = this->transaction_id;
        req.reason         = "";
        req.code           = "";

        this->deleteLater();

        // Yet to add send to_device message
}
//! Completes the verification flow
void
DeviceVerificationFlow::acceptDevice()
{
        emit deviceVerified();
        this->deleteLater();

        // Yet to add send to_device message
}
