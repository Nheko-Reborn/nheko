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
        //! accepts a verification and starts the verification flow
        void acceptVerificationRequest();
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
        QTimer *timeout = nullptr;
};
