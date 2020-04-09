#pragma once

#include <QHash>
#include <QImage>
#include <QObject>
#include <QString>

#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
#include <QtDBus/QDBusArgument>
#include <QtDBus/QDBusInterface>
#endif

struct roomEventId
{
        QString roomId;
        QString eventId;
};

inline bool
operator<(const roomEventId &a, const roomEventId &b)
{
        if (a.roomId == b.roomId)
                return a.eventId < b.eventId;
        else
                return a.roomId < b.roomId;
}

inline bool
operator==(const roomEventId &a, const roomEventId &b)
{
        return a.roomId == b.roomId && a.eventId == b.eventId;
}

inline uint
qHash(const roomEventId &v, uint seed)
{
        return qHash(v.roomId, seed) ^ qHash(v.eventId, seed);
}

class NotificationsManager : public QObject
{
        Q_OBJECT
public:
        NotificationsManager(QObject *parent = nullptr);

        void postNotification(const QString &roomId,
                              const QString &eventId,
                              const QString &roomName,
                              const QString &senderName,
                              const QString &text,
                              const QImage &icon);

signals:
        void notificationClicked(const QString roomId, const QString eventId);

public slots:
        void removeNotification(const QString &roomId, const QString &eventId);

#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
public:
        void closeNotifications(QString roomId);

private:
        QDBusInterface dbus;
        uint showNotification(const QString summary, const QString text, const QImage image);
        void closeNotification(uint id);

        // notification ID to (room ID, event ID)
        QMap<uint, roomEventId> notificationIds;
        QHash<roomEventId, uint> eventToNotificationId;
#endif

        // these slots are platform specific (D-Bus only)
        // but Qt slot declarations can not be inside an ifdef!
private slots:
        void actionInvoked(uint id, QString action);
        void notificationClosed(uint id, uint reason);
};

#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
QDBusArgument &
operator<<(QDBusArgument &arg, const QImage &image);
const QDBusArgument &
operator>>(const QDBusArgument &arg, QImage &);
#endif
