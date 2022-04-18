// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef NHEKODBUSAPI_H
#define NHEKODBUSAPI_H

#include <QDBusArgument>
#include <QIcon>
#include <QObject>
#include <QVersionNumber>

namespace nheko::dbus {

//! Registers all necessary classes with D-Bus. Call this before using any nheko D-Bus classes.
void
init();

//! The nheko D-Bus API version provided by this file. The API version number follows semantic
//! versioning as defined by https://semver.org.
const QVersionNumber dbusApiVersion{0, 0, 1};

//! Compare the installed Nheko API to the version that your client app targets to see if they
//! are compatible.
bool
apiVersionIsCompatible(const QVersionNumber &clientAppVersion);

class RoomInfoItem : public QObject
{
    Q_OBJECT

public:
    RoomInfoItem(const QString &roomId         = QString{},
                 const QString &alias          = QString{},
                 const QString &title          = QString{},
                 const QImage &image           = QImage{},
                 const int unreadNotifications = 0,
                 QObject *parent               = nullptr);

    RoomInfoItem(const RoomInfoItem &other);

    const QString &roomId() const { return roomId_; }
    const QString &alias() const { return alias_; }
    const QString &roomName() const { return roomName_; }
    const QImage &image() const { return image_; }
    int unreadNotifications() const { return unreadNotifications_; }

    RoomInfoItem &operator=(const RoomInfoItem &other);
    friend QDBusArgument &operator<<(QDBusArgument &arg, const nheko::dbus::RoomInfoItem &item);
    friend const QDBusArgument &
    operator>>(const QDBusArgument &arg, nheko::dbus::RoomInfoItem &item);

private:
    QString roomId_;
    QString alias_;
    QString roomName_;
    QImage image_;
    int unreadNotifications_;
};

//! Get the nheko D-Bus API version.
QString
apiVersion();
//! Get the nheko version.
QString
nhekoVersion();
//! Call this function to get a list of all joined rooms.
QVector<RoomInfoItem>
rooms();
//! Activates a currently joined room.
void
activateRoom(const QString &alias);
//! Joins a room. It is your responsibility to ask for confirmation (if desired).
void
joinRoom(const QString &alias);
//! Starts or activates a direct chat. It is your responsibility to ask for confirmation (if
//! desired).
void
directChat(const QString &userId);

QDBusArgument &
operator<<(QDBusArgument &arg, const RoomInfoItem &item);
const QDBusArgument &
operator>>(const QDBusArgument &arg, RoomInfoItem &item);
} // nheko::dbus
Q_DECLARE_METATYPE(nheko::dbus::RoomInfoItem)

QDBusArgument &
operator<<(QDBusArgument &arg, const QImage &image);
const QDBusArgument &
operator>>(const QDBusArgument &arg, QImage &);

#define NHEKO_DBUS_SERVICE_NAME "im.nheko.Nheko"

#endif // NHEKODBUSAPI_H
