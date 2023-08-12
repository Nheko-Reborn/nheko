// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QHash>
#include <QObject>
#include <QQmlEngine>
#include <QSharedPointer>

#include <mtx/events.hpp>
#include <mtx/events/encrypted.hpp>

class DeviceVerificationFlow;
class TimelineModel;
class TimelineViewManager;
class RoomlistModel;

class VerificationManager final : public QObject
{
    Q_OBJECT

    QML_ELEMENT
    QML_SINGLETON

public:
    VerificationManager(TimelineViewManager *o);

    static VerificationManager *create(QQmlEngine *qmlEngine, QJSEngine *)
    {
        // The instance has to exist before it is used. We cannot replace it.
        Q_ASSERT(instance_);

        // The engine has to have the same thread affinity as the singleton.
        Q_ASSERT(qmlEngine->thread() == instance_->thread());

        // There can only be one engine accessing the singleton.
        static QJSEngine *s_engine = nullptr;
        if (s_engine)
            Q_ASSERT(qmlEngine == s_engine);
        else
            s_engine = qmlEngine;

        QJSEngine::setObjectOwnership(instance_, QJSEngine::CppOwnership);
        return instance_;
    }

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

    inline static VerificationManager *instance_ = nullptr;
};
