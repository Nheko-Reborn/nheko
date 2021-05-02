// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QString>
#include <QVector>
#include <mtx/responses.hpp>
#include <mtx/responses/common.hpp>

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
        Q_PROPERTY(QString displayName READ displayName NOTIFY displayNameChanged)
        Q_PROPERTY(QString userid READ userid CONSTANT)
        Q_PROPERTY(QString avatarUrl READ avatarUrl NOTIFY avatarUrlChanged)
        Q_PROPERTY(DeviceInfoModel *deviceList READ deviceList CONSTANT)
        Q_PROPERTY(bool isGlobalUserProfile READ isGlobalUserProfile CONSTANT)
        Q_PROPERTY(bool isUserVerified READ getUserStatus NOTIFY userStatusChanged)
        Q_PROPERTY(bool isLoading READ isLoading NOTIFY loadingChanged)
        Q_PROPERTY(
          bool userVerificationEnabled READ userVerificationEnabled NOTIFY userStatusChanged)
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
        bool getUserStatus();
        bool userVerificationEnabled() const;
        bool isSelf() const;
        bool isLoading() const;
        TimelineModel *room() const { return model; }

        Q_INVOKABLE void verify(QString device = "");
        Q_INVOKABLE void unverify(QString device = "");
        Q_INVOKABLE void fetchDeviceList(const QString &userID);
        Q_INVOKABLE void banUser();
        // Q_INVOKABLE void ignoreUser();
        Q_INVOKABLE void kickUser();
        Q_INVOKABLE void startChat();
        Q_INVOKABLE void changeUsername(QString username);
        Q_INVOKABLE void changeAvatar();

signals:
        void userStatusChanged();
        void loadingChanged();
        void displayNameChanged();
        void avatarUrlChanged();
        void displayError(const QString &errorMessage);
        void globalUsernameRetrieved(const QString &globalUser);

public slots:
        void updateAvatarUrl();

protected slots:
        void setGlobalUsername(const QString &globalUser);

private:
        void updateRoomMemberState(mtx::events::state::Member member);
        void getGlobalProfileData();

private:
        QString roomid_, userid_;
        QString globalUsername;
        QString globalAvatarUrl;
        DeviceInfoModel deviceList_;
        bool isUserVerified = false;
        bool hasMasterKey   = false;
        bool isLoading_     = false;
        TimelineViewManager *manager;
        TimelineModel *model;
};
