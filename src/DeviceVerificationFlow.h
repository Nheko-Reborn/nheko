#pragma once

#include "Olm.h"

#include "MatrixClient.h"
#include "mtx/responses/crypto.hpp"
#include <QObject>

class QTimer;

using sas_ptr = std::unique_ptr<mtx::crypto::SAS>;

class TimelineModel;

class DeviceVerificationFlow : public QObject
{
        Q_OBJECT
        // Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")
        Q_PROPERTY(QString tranId READ getTransactionId WRITE setTransactionId)
        Q_PROPERTY(bool sender READ getSender WRITE setSender)
        Q_PROPERTY(QString userId READ getUserId WRITE setUserId)
        Q_PROPERTY(QString deviceId READ getDeviceId WRITE setDeviceId)
        Q_PROPERTY(Method method READ getMethod WRITE setMethod)
        Q_PROPERTY(Type type READ getType WRITE setType)
        Q_PROPERTY(std::vector<int> sasList READ getSasList CONSTANT)

public:
        enum Type
        {
                ToDevice,
                RoomMsg
        };
        Q_ENUM(Type)

        enum Method
        {
                Decimal,
                Emoji
        };
        Q_ENUM(Method)

        enum Error
        {
                UnknownMethod,
                MismatchedCommitment,
                MismatchedSAS,
                KeyMismatch,
                Timeout,
                User
        };
        Q_ENUM(Error)

        DeviceVerificationFlow(
          QObject *parent              = nullptr,
          DeviceVerificationFlow::Type = DeviceVerificationFlow::Type::ToDevice,
          TimelineModel *model         = nullptr);
        // getters
        QString getTransactionId();
        QString getUserId();
        QString getDeviceId();
        Method getMethod();
        Type getType();
        std::vector<int> getSasList();
        bool getSender();
        // setters
        void setTransactionId(QString transaction_id_);
        void setUserId(QString userID);
        void setDeviceId(QString deviceID);
        void setMethod(Method method_);
        void setType(Type type_);
        void setSender(bool sender_);
        void setEventId(std::string event_id);

        void callback_fn(const mtx::responses::QueryKeys &res,
                         mtx::http::RequestErr err,
                         std::string user_id);

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
        void cancelVerification(DeviceVerificationFlow::Error error_code);
        //! sends the verification key
        void sendVerificationKey();
        //! sends the mac of the keys
        void sendVerificationMac();
        //! Completes the verification flow
        void acceptDevice();
        //! unverifies a device
        void unverify();

signals:
        void verificationRequestAccepted(Method method);
        void deviceVerified();
        void timedout();
        void verificationCanceled();
        void refreshProfile();
        void deleteFlow();

private:
        // general
        QString userId;
        QString deviceId;
        Method method;
        Type type;
        bool sender;
        QTimer *timeout = nullptr;
        sas_ptr sas;
        bool isMacVerified = false;
        std::string mac_method;
        std::string commitment;
        mtx::identifiers::User toClient;
        std::vector<int> sasList;
        std::map<std::string, std::string> device_keys;
        // for to_device messages
        std::string transaction_id;
        // for room messages
        std::optional<std::string> room_id;
        std::optional<std::string> event_id;
        TimelineModel *model_;
        mtx::common::RelatesTo relation;
};
