#pragma once

#include <QObject>
#include <QString>
#include <QVector>

#include "MatrixClient.h"

class DeviceInfo;

class UserProfile : public QObject
{
        Q_OBJECT
        Q_PROPERTY(QString userId READ getUserId WRITE setUserId NOTIFY userIdChanged)
        Q_PROPERTY(std::vector<DeviceInfo> deviceList READ getDeviceList NOTIFY deviceListUpdated)
public:
        // constructor
        explicit UserProfile(QObject *parent = 0);
        // getters
        std::vector<DeviceInfo> getDeviceList();
        QString getUserId();
        // setters
        void setUserId(const QString &userId);

        enum Status
        {
                VERIFIED,
                UNVERIFIED,
                BLOCKED
        };
        Q_ENUM(Status)

        void fetchDeviceList(const QString &userID);
        Q_INVOKABLE void banUser();
        // Q_INVOKABLE void ignoreUser();
        Q_INVOKABLE void kickUser();
        Q_INVOKABLE void startChat();

signals:
        void userIdChanged();
        void deviceListUpdated();
        void updateDeviceList();
        void appendDeviceList(const QString device_id,
                              const QString device_naem,
                              const UserProfile::Status verification_status);

private:
        std::vector<DeviceInfo> deviceList;
        QString userId;
        std::optional<std::string> cross_verified;

        void callback_fn(const mtx::responses::QueryKeys &res,
                         mtx::http::RequestErr err,
                         std::string user_id,
                         std::optional<std::vector<std::string>> cross_verified);
};

class DeviceInfo
{
public:
        DeviceInfo(const QString deviceID,
                   const QString displayName,
                   UserProfile::Status verification_status_)
          : device_id(deviceID)
          , display_name(displayName)
          , verification_status(verification_status_)
        {}

        QString device_id;
        QString display_name;
        UserProfile::Status verification_status;
};