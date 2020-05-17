#pragma once

#include <QMap>
#include <QObject>
#include <QString>

struct DeviceInfo
{
        QString device_id;
        QString display_name;
};

class UserProfile : public QObject
{
        Q_OBJECT
        Q_PROPERTY(QMap deviceList READ getDeviceList NOTIFY DeviceListUpdated)

public:
        explicit UserProfile(QObject *parent = 0);
        QMap<QString, QString> getDeviceList();

        Q_INVOKABLE void fetchDeviceList(const QString &userID);

signals:
        void DeviceListUpdated();

private:
        QMap<QString, QString> deviceList;
};