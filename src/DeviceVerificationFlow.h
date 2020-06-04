#pragma once

#include <QObject>

class QTimer;

class DeviceVerificationFlow : public QObject
{
        Q_OBJECT
        // Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")

public:
        enum Method
        {
                Decimal,
                Emoji
        };
        Q_ENUM(Method)

        DeviceVerificationFlow(QObject *parent = nullptr);

public slots:
        //! sends a verification request
        void sendVerificationRequest();
        //! accepts a verification
        void acceptVerificationRequest();
        //! starts the verification flow
        void startVerificationRequest();
        //! cancels a verification flow
        void cancelVerification();
        //! Completes the verification flow
        void acceptDevice();

signals:
        void verificationRequestAccepted(Method method);
        void deviceVerified();
        void timedout();
        void verificationCanceled();

private:
        //! generates a unique transaction id
        std::string generate_txn_id();

        QTimer *timeout = nullptr;
        std::string transaction_id;
};
