// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-FileCopyrightText: 2023 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QImage>
#include <QMap>
#include <QObject>
#include <QString>

#include <mtx/responses/notifications.hpp>

#if defined(NHEKO_DBUS_SYS)
#include <QtDBus/QDBusArgument>
#include <QtDBus/QDBusInterface>
#endif

struct roomEventId
{
    QString roomId;
    QString eventId;
};

inline bool
operator==(const roomEventId &a, const roomEventId &b)
{
    return a.roomId == b.roomId && a.eventId == b.eventId;
}

class NotificationsManager final : public QObject
{
    Q_OBJECT
public:
    NotificationsManager(QObject *parent = nullptr);

    void postNotification(const mtx::responses::Notification &notification, const QImage &icon);

    void removeNotification(const QString &roomId, const QString &eventId);

signals:
    void notificationClicked(const QString roomId, const QString eventId);
    void sendNotificationReply(const QString roomId, const QString eventId, const QString body);
    void systemPostNotificationCb(const QString &room_id,
                                  const QString &event_id,
                                  const QString &roomName,
                                  const QString &text,
                                  const QImage &icon);

public slots:
    void removeNotifications(const QString &roomId, const std::vector<QString> &eventId);

#if defined(NHEKO_DBUS_SYS)
public:
    void closeNotifications(QString roomId);
#if defined(Q_OS_LINUX)
    void closeAllNotifications();
#endif

private:
    QDBusInterface dbus;

    void systemPostNotification(const QString &room_id,
                                const QString &event_id,
                                const QString &roomName,
                                const QString &text,
                                const QImage &icon);
    void closeNotification(uint id);

    const bool hasMarkup_;
    const bool hasImages_;
#endif

#if defined(Q_OS_MACOS)
private:
    // Objective-C(++) doesn't like to do lots of regular C++, so the actual notification
    // posting is split out
    void objCxxPostNotification(const QString &room_name,
                                const QString &room_id,
                                const QString &event_id,
                                const QString &subtitle,
                                const QString &informativeText,
                                const QString &bodyImagePath,
                                const QString &respondStr,
                                const QString &sendStr,
                                const QString &placeholder,
                                const bool playSound);

public:
    static void attachToMacNotifCenter();
#endif

#if defined(Q_OS_WINDOWS)
private:
    void
    systemPostNotification(const QString &line1, const QString &line2, const QString &iconPath);
#endif

    // these slots are platform specific (D-Bus only)
    // but Qt slot declarations can not be inside an ifdef!
private slots:
    void actionInvoked(uint id, QString action);
    void notificationClosed(uint id, uint reason);
    void notificationReplied(uint id, QString reply);

private:
    QString getMessageTemplate(const mtx::responses::Notification &notification);

    // notification ID to (room ID, event ID)
    // Only populated on Linux atm
    QMap<uint, roomEventId> notificationIds;
};

#if defined(NHEKO_DBUS_SYS)
QDBusArgument &
operator<<(QDBusArgument &arg, const QImage &image);
const QDBusArgument &
operator>>(const QDBusArgument &arg, QImage &);
#endif
