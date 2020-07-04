#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QString>
#include <QVector>

#include "MatrixClient.h"

namespace verification {
Q_NAMESPACE

enum Status
{
        VERIFIED,
        UNVERIFIED,
        BLOCKED
};
Q_ENUM_NS(Status)
}

class DeviceInfo
{
public:
        DeviceInfo(const QString deviceID,
                   const QString displayName,
                   verification::Status verification_status_)
          : device_id(deviceID)
          , display_name(displayName)
          , verification_status(verification_status_)
        {}
        DeviceInfo()
          : verification_status(verification::UNVERIFIED)
        {}

        QString device_id;
        QString display_name;

        verification::Status verification_status;
};

class DeviceInfoModel : public QAbstractListModel
{
        Q_OBJECT
public:
        enum Roles
        {
                DeviceId,
                DeviceName,
                VerificationStatus,
        };

        explicit DeviceInfoModel(QObject *parent = nullptr)
        {
                (void)parent;
                connect(this, &DeviceInfoModel::queueReset, this, &DeviceInfoModel::reset);
        };
        QHash<int, QByteArray> roleNames() const override;
        int rowCount(const QModelIndex &parent = QModelIndex()) const
        {
                (void)parent;
                return (int)deviceList_.size();
        }
        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

signals:
        void queueReset(const std::vector<DeviceInfo> &deviceList);
public slots:
        void reset(const std::vector<DeviceInfo> &deviceList);

private:
        std::vector<DeviceInfo> deviceList_;
};

class UserProfile : public QObject
{
        Q_OBJECT
        Q_PROPERTY(QString displayName READ displayName CONSTANT)
        Q_PROPERTY(QString userid READ userid CONSTANT)
        Q_PROPERTY(QString avatarUrl READ avatarUrl CONSTANT)
        Q_PROPERTY(DeviceInfoModel *deviceList READ deviceList CONSTANT)
public:
        UserProfile(QString roomid, QString userid, QObject *parent = 0);

        DeviceInfoModel *deviceList();

        QString userid();
        QString displayName();
        QString avatarUrl();

        void fetchDeviceList(const QString &userID);
        Q_INVOKABLE void banUser();
        // Q_INVOKABLE void ignoreUser();
        Q_INVOKABLE void kickUser();
        Q_INVOKABLE void startChat();

private:
        QString roomid_, userid_;
        std::optional<std::string> cross_verified;
        DeviceInfoModel deviceList_;

        void callback_fn(const mtx::responses::QueryKeys &res,
                         mtx::http::RequestErr err,
                         std::string user_id,
                         std::optional<std::vector<std::string>> cross_verified);
};
