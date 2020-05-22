#pragma once

#include <QObject>
#include <QString>

#include "MatrixClient.h"

class DeviceInfo
{
public:
        explicit DeviceInfo(QString device_id,QString display_name){
                this->device_id = device_id;
                this->display_name = display_name;
        }
        ~DeviceInfo() = default;
        DeviceInfo(const DeviceInfo &device){
                this->device_id = device.device_id;
                this->display_name = device.display_name;
        }

        QString device_id;
        QString display_name;
};
Q_DECLARE_METATYPE(DeviceInfo);

class UserProfile : public QObject
{
        Q_OBJECT
        Q_PROPERTY(QVector<DeviceInfo> deviceList READ getDeviceList NOTIFY deviceListUpdated)
        Q_PROPERTY(QString userId READ getUserId WRITE setUserId)

public:
        // constructor
        explicit UserProfile(QObject *parent = 0);
        // getters
        QVector<DeviceInfo> getDeviceList();
        QString getUserId();
        // setters
        void setUserId(const QString &userId);

        Q_INVOKABLE void fetchDeviceList(const QString &userID);

signals:
        void deviceListUpdated();

private:
        QVector<DeviceInfo> deviceList;
        QString userId;
};