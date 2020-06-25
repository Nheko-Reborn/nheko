#pragma once

#include "Olm.h"

#include "mtx/responses/crypto.hpp"
#include <MatrixClient.h>
#include <QObject>

class QTimer;

using sas_ptr = std::unique_ptr<mtx::crypto::SAS>;

class DeviceVerificationFlow : public QObject
{
        Q_OBJECT
        // Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")
        Q_PROPERTY(QString tranId READ getTransactionId WRITE setTransactionId)
        Q_PROPERTY(bool sender READ getSender WRITE setSender)
        Q_PROPERTY(QString userId READ getUserId WRITE setUserId)
        Q_PROPERTY(QString deviceId READ getDeviceId WRITE setDeviceId)
        Q_PROPERTY(Method method READ getMethod WRITE setMethod)
        Q_PROPERTY(std::vector<int> sasList READ getSasList)

public:
        enum Method
        {
                Decimal,
                Emoji
        };
        Q_ENUM(Method)

        DeviceVerificationFlow(QObject *parent = nullptr);
        QString getTransactionId();
        QString getUserId();
        QString getDeviceId();
        Method getMethod();
        std::vector<int> getSasList();
        void setTransactionId(QString transaction_id_);
        bool getSender();
        void setUserId(QString userID);
        void setDeviceId(QString deviceID);
        void setMethod(Method method_);
        void setSender(bool sender_);

        nlohmann::json canonical_json;

public slots:
        //! sends a verification request
        void sendVerificationRequest();
        //! accepts a verification request
        void sendVerificationReady();
        //! completes the verification flow();
        void sendVerificationDone();
        //! accepts a verification
        void acceptVerificationRequest();
        //! starts the verification flow
        void startVerificationRequest();
        //! cancels a verification flow
        void cancelVerification();
        //! sends the verification key
        void sendVerificationKey();
        //! sends the mac of the keys
        void sendVerificationMac();
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
        bool sender;

        QTimer *timeout = nullptr;
        sas_ptr sas;
        bool isMacVerified = false;
        std::string mac_method;
        std::string transaction_id;
        std::string commitment;
        mtx::identifiers::User toClient;
        std::vector<int> sasList;
        std::map<std::string, std::string> device_keys;
};
