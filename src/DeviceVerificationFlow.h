#pragma once

#include <MatrixClient.h>
#include <QObject>

class QTimer;

class DeviceVerificationFlow : public QObject
{
        Q_OBJECT
        // Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")
        Q_PROPERTY(QString userId READ getUserId WRITE setUserId)
        Q_PROPERTY(QString deviceId READ getDeviceId WRITE setDeviceId)
        Q_PROPERTY(Method method READ getMethod WRITE setMethod)

public:
        enum Method
        {
                Decimal,
                Emoji
        };
        Q_ENUM(Method)

        DeviceVerificationFlow(QObject *parent = nullptr);
        QString getUserId();
        QString getDeviceId();
        Method getMethod();
        void setUserId(QString userID);
        void setDeviceId(QString deviceID);
        void setMethod(Method method_);

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
        QString userId;
        QString deviceId;
        Method method;

        QTimer *timeout = nullptr;
        std::string transaction_id;
        mtx::identifiers::User toClient;
};
