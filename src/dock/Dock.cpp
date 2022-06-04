// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dock/Dock.h"
#include <qobject.h>
#if defined(NHEKO_DBUS_SYS)
#include <qdbusconnectioninterface.h>
Dock::Dock(QObject *parent)
  : QObject(parent)
  , unityServiceWatcher(new QDBusServiceWatcher(this))
{
    unityServiceWatcher->setConnection(QDBusConnection::sessionBus());
    unityServiceWatcher->setWatchMode(QDBusServiceWatcher::WatchForUnregistration |
                                      QDBusServiceWatcher::WatchForRegistration);
    unityServiceWatcher->addWatchedService(QStringLiteral("com.canonical.Unity"));
    connect(unityServiceWatcher,
            &QDBusServiceWatcher::serviceRegistered,
            this,
            [this](const QString &service) {
                Q_UNUSED(service);
                unityServiceAvailable = true;
            });
    connect(unityServiceWatcher,
            &QDBusServiceWatcher::serviceUnregistered,
            this,
            [this](const QString &service) {
                Q_UNUSED(service);
                unityServiceAvailable = false;
            });
    QDBusPendingCall listNamesCall =
      QDBusConnection::sessionBus().interface()->asyncCall(QStringLiteral("ListNames"));
    QDBusPendingCallWatcher *callWatcher = new QDBusPendingCallWatcher(listNamesCall, this);
    connect(callWatcher,
            &QDBusPendingCallWatcher::finished,
            this,
            [this](QDBusPendingCallWatcher *watcher) {
                QDBusPendingReply<QStringList> reply = *watcher;
                watcher->deleteLater();

                if (reply.isError()) {
                    return;
                }

                const QStringList &services = reply.value();

                unityServiceAvailable = services.contains(QLatin1String("com.canonical.Unity"));
            });
}

void
Dock::setUnreadCount(const int count)
{
    unitySetNotificationCount(count);
}
void
Dock::unitySetNotificationCount(const int count)
{
    if (unityServiceAvailable) {
        const QString launcherId = QLatin1String("nheko.desktop");

        const QVariantMap properties{{QStringLiteral("count-visible"), count > 0},
                                     {QStringLiteral("count"), count}};

        QDBusMessage message =
          QDBusMessage::createSignal(QStringLiteral("/io/github/NhekoReborn/Nheko/UnityLauncher"),
                                     QStringLiteral("com.canonical.Unity.LauncherEntry"),
                                     QStringLiteral("Update"));
        message.setArguments({launcherId, properties});
        QDBusConnection::sessionBus().send(message);
    }
}
#else
void
Dock::Dock(QObject *parent)
  : QObject(parent)
{}
void
Dock::setUnreadCount(const int count)
{}
#endif
