#include "DeviceVerificationFlow.h"

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

//! accepts a verification and starts the verification flow
void
DeviceVerificationFlow::acceptVerificationRequest()
{
        emit verificationRequestAccepted(rand() % 2 ? Emoji : Decimal);
}
//! cancels a verification flow
void
DeviceVerificationFlow::cancelVerification()
{
        this->deleteLater();
}
//! Completes the verification flow
void
DeviceVerificationFlow::acceptDevice()
{
        emit deviceVerified();
        this->deleteLater();
}
