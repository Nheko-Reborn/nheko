#pragma once

#include <QObject>
#include <QString>
#include <QVector>

#include "MatrixClient.h"
class DeviceInfo
{
public:
        DeviceInfo(const QString deviceID, const QString displayName)
          : device_id(deviceID)
          , display_name(displayName)
        {}

        DeviceInfo() {}

        QString device_id;
        QString display_name;
};

class UserProfile : public QObject
{
        Q_OBJECT
        Q_PROPERTY(QString userId READ getUserId WRITE setUserId NOTIFY userIdChanged)
        Q_PROPERTY(QVector<DeviceInfo> deviceList READ getDeviceList NOTIFY deviceListUpdated)
public:
        // constructor
        explicit UserProfile(QObject *parent = 0);
        // getters
        QVector<DeviceInfo> getDeviceList();
        QString getUserId();
        // setters
        void setUserId(const QString &userId);

        Q_INVOKABLE void fetchDeviceList(const QString &userID);
        Q_INVOKABLE void updateDeviceList();
        Q_INVOKABLE void banUser();
        // Q_INVOKABLE void ignoreUser();
        Q_INVOKABLE void kickUser();
        Q_INVOKABLE void startChat();

signals:
        void userIdChanged();
        void deviceListUpdated();

private:
        QVector<DeviceInfo> deviceList;
        QString userId;
};