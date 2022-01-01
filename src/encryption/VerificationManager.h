// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QHash>
#include <QObject>
#include <QSharedPointer>

#include <mtx/events.hpp>
#include <mtx/events/encrypted.hpp>

class DeviceVerificationFlow;
class TimelineModel;
class TimelineModel;
class TimelineViewManager;
class RoomlistModel;

class VerificationManager : public QObject
{
    Q_OBJECT

public:
    VerificationManager(TimelineViewManager *o = nullptr);

    Q_INVOKABLE void removeVerificationFlow(DeviceVerificationFlow *flow);
    void verifyUser(QString userid);
    void verifyDevice(QString userid, QString deviceid);
    void verifyOneOfDevices(QString userid, std::vector<QString> deviceids);

signals:
    void newDeviceVerificationRequest(DeviceVerificationFlow *flow);

public slots:
    void receivedRoomDeviceVerificationRequest(
      const mtx::events::RoomEvent<mtx::events::msg::KeyVerificationRequest> &message,
      TimelineModel *model);
    void receivedDeviceVerificationRequest(const mtx::events::msg::KeyVerificationRequest &msg,
                                           std::string sender);
    void receivedDeviceVerificationStart(const mtx::events::msg::KeyVerificationStart &msg,
                                         std::string sender);

private:
    QHash<QString, QSharedPointer<DeviceVerificationFlow>> dvList;
    bool isInitialSync_ = false;
    RoomlistModel *rooms_;
};
