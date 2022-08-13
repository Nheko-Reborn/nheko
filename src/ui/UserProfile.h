// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QString>
#include <QVector>
#include <mtx/responses.hpp>
#include <mtx/responses/common.hpp>

#include "CacheCryptoStructs.h"
#include "timeline/TimelineModel.h"

namespace verification {
Q_NAMESPACE

enum Status
{
    SELF,
    VERIFIED,
    UNVERIFIED,
    BLOCKED,
    NOT_APPLICABLE
};
Q_ENUM_NS(Status)
}

class DeviceVerificationFlow;
class TimelineViewManager;

class UserProfileFetchProxy : public QObject
{
    Q_OBJECT

public:
    UserProfileFetchProxy(QObject *p = nullptr)
      : QObject(p)
    {}

signals:
    void profileFetched(mtx::responses::Profile);
};

class DeviceInfo
{
public:
    DeviceInfo(const QString deviceID,
               const QString displayName,
               verification::Status verification_status_,
               const QString lastIp_,
               const size_t lastTs_)
      : device_id(deviceID)
      , display_name(displayName)
      , verification_status(verification_status_)
      , lastIp(lastIp_)
      , lastTs(lastTs_)
    {}
    DeviceInfo(const QString deviceID,
               const QString displayName,
               verification::Status verification_status_)
      : device_id(deviceID)
      , display_name(displayName)
      , verification_status(verification_status_)
      , lastTs(0)
    {}
    DeviceInfo()
      : verification_status(verification::UNVERIFIED)
    {}

    QString device_id;
    QString display_name;

    verification::Status verification_status;
    QString lastIp;
    qlonglong lastTs;
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
        LastIp,
        LastTs,
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
    Q_PROPERTY(QString displayName READ displayName NOTIFY displayNameChanged)
    Q_PROPERTY(QString userid READ userid CONSTANT)
    Q_PROPERTY(QString avatarUrl READ avatarUrl NOTIFY avatarUrlChanged)
    Q_PROPERTY(DeviceInfoModel *deviceList READ deviceList NOTIFY devicesChanged)
    Q_PROPERTY(bool isGlobalUserProfile READ isGlobalUserProfile CONSTANT)
    Q_PROPERTY(int userVerified READ getUserStatus NOTIFY userStatusChanged)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY loadingChanged)
    Q_PROPERTY(bool userVerificationEnabled READ userVerificationEnabled NOTIFY userStatusChanged)
    Q_PROPERTY(bool isSelf READ isSelf CONSTANT)
    Q_PROPERTY(TimelineModel *room READ room CONSTANT)
public:
    UserProfile(QString roomid,
                QString userid,
                TimelineViewManager *manager_,
                TimelineModel *parent = nullptr);

    DeviceInfoModel *deviceList();

    QString userid();
    QString displayName();
    QString avatarUrl();
    bool isGlobalUserProfile() const;
    crypto::Trust getUserStatus();
    bool userVerificationEnabled() const;
    bool isSelf() const;
    bool isLoading() const;
    TimelineModel *room() const { return model; }

    Q_INVOKABLE void verify(QString device = QLatin1String(""));
    Q_INVOKABLE void unverify(QString device = QLatin1String(""));
    Q_INVOKABLE void fetchDeviceList(const QString &userID);
    Q_INVOKABLE void refreshDevices();
    Q_INVOKABLE void banUser();
    Q_INVOKABLE void signOutDevice(const QString &deviceID);
    // Q_INVOKABLE void ignoreUser();
    Q_INVOKABLE void kickUser();
    Q_INVOKABLE void startChat();
    Q_INVOKABLE void startChat(bool encryptionEnabled);
    Q_INVOKABLE void changeUsername(QString username);
    Q_INVOKABLE void changeDeviceName(QString deviceID, QString deviceName);
    Q_INVOKABLE void changeAvatar();
    Q_INVOKABLE void openGlobalProfile();

signals:
    void userStatusChanged();
    void loadingChanged();
    void displayNameChanged();
    void avatarUrlChanged();
    void displayError(const QString &errorMessage);
    void globalUsernameRetrieved(const QString &globalUser);
    void devicesChanged();

    // internal
    void verificationStatiChanged();

public slots:
    void updateAvatarUrl();

private slots:
    void setGlobalUsername(const QString &globalUser);
    void updateVerificationStatus();

private:
    void updateRoomMemberState(mtx::events::state::Member member);
    void getGlobalProfileData();

private:
    QString roomid_, userid_;
    QString globalUsername;
    QString globalAvatarUrl;
    DeviceInfoModel deviceList_;
    crypto::Trust isUserVerified = crypto::Trust::Unverified;
    bool hasMasterKey            = false;
    bool isLoading_              = false;
    TimelineViewManager *manager;
    TimelineModel *model;
};
