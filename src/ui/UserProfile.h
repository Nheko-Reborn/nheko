#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QString>
#include <QVector>

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

class DeviceVerificationFlow;
class TimelineModel;
class TimelineViewManager;

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
        int rowCount(const QModelIndex &parent = QModelIndex()) const override
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

        friend class UserProfile;
};

class UserProfile : public QObject
{
        Q_OBJECT
        Q_PROPERTY(QString displayName READ displayName CONSTANT)
        Q_PROPERTY(QString userid READ userid CONSTANT)
        Q_PROPERTY(QString avatarUrl READ avatarUrl CONSTANT)
        Q_PROPERTY(DeviceInfoModel *deviceList READ deviceList CONSTANT)
        Q_PROPERTY(bool isUserVerified READ getUserStatus NOTIFY userStatusChanged)
public:
        UserProfile(QString roomid,
                    QString userid,
                    TimelineViewManager *manager_,
                    TimelineModel *parent = nullptr);

        DeviceInfoModel *deviceList();

        QString userid();
        QString displayName();
        QString avatarUrl();
        bool getUserStatus();

        Q_INVOKABLE void verify(QString device = "");
        Q_INVOKABLE void unverify(QString device = "");
        Q_INVOKABLE void fetchDeviceList(const QString &userID);
        Q_INVOKABLE void banUser();
        // Q_INVOKABLE void ignoreUser();
        Q_INVOKABLE void kickUser();
        Q_INVOKABLE void startChat();

signals:
        void userStatusChanged();

private:
        QString roomid_, userid_;
        DeviceInfoModel deviceList_;
        bool isUserVerified = false;
        TimelineViewManager *manager;
        TimelineModel *model;
};
