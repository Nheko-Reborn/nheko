// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dock/Dock.h"

#include <QApplication>

#include "Logging.h"

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
                nhlog::ui()->info("Unity service available: {}", unityServiceAvailable);
            });
    connect(unityServiceWatcher,
            &QDBusServiceWatcher::serviceUnregistered,
            this,
            [this](const QString &service) {
                Q_UNUSED(service);
                unityServiceAvailable = false;
                nhlog::ui()->info("Unity service available: {}", unityServiceAvailable);
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
                    nhlog::ui()->error("Failed to list dbus names");
                    return;
                }

                const QStringList &services = reply.value();

                unityServiceAvailable = services.contains(QLatin1String("com.canonical.Unity"));
                nhlog::ui()->info("Unity service available: {}", unityServiceAvailable);
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
        const QString launcherId =
          QLatin1String("application://%1.desktop").arg(qApp->desktopFileName());

        const QVariantMap properties{{QStringLiteral("count-visible"), count > 0},
                                     {QStringLiteral("count"), count}};

        QDBusMessage message =
          QDBusMessage::createSignal(QStringLiteral("/im/nheko/Nheko/UnityLauncher"),
                                     QStringLiteral("com.canonical.Unity.LauncherEntry"),
                                     QStringLiteral("Update"));
        message.setArguments({launcherId, properties});
        QDBusConnection::sessionBus().send(message);
    }
}
#else
Dock::Dock(QObject *parent)
  : QObject(parent)
{}
void
Dock::setUnreadCount(const int)
{}
#endif
