// SPDX-FileCopyrightText: Nheko Contributors
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
inline const QVersionNumber dbusApiVersion{1, 0, 1};

//! Compare the installed Nheko API to the version that your client app targets to see if they
//! are compatible.
bool
apiVersionIsCompatible(const QVersionNumber &clientAppVersion);

class RoomInfoItem final : public QObject
{
    Q_OBJECT

public:
    RoomInfoItem(const QString &roomId         = QString{},
                 const QString &alias          = QString{},
                 const QString &title          = QString{},
                 const QString &avatarUrl      = QString{},
                 const int unreadNotifications = 0,
                 QObject *parent               = nullptr);

    RoomInfoItem(const RoomInfoItem &other);

    const QString &roomId() const { return roomId_; }
    const QString &alias() const { return alias_; }
    const QString &roomName() const { return roomName_; }
    const QString &avatarUrl() const { return avatarUrl_; }
    int unreadNotifications() const { return unreadNotifications_; }

    RoomInfoItem &operator=(const RoomInfoItem &other);
    friend QDBusArgument &operator<<(QDBusArgument &arg, const nheko::dbus::RoomInfoItem &item);
    friend const QDBusArgument &
    operator>>(const QDBusArgument &arg, nheko::dbus::RoomInfoItem &item);

private:
    QString roomId_;
    QString alias_;
    QString roomName_;
    QString avatarUrl_;
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
//! Fetch an image using a matrix URI
QImage
image(const QString &uri);
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
//! Get the user's status message.
QString
statusMessage();
//! Sets the user's status message (if supported by the homeserver).
void
setStatusMessage(const QString &message);

QDBusArgument &
operator<<(QDBusArgument &arg, const RoomInfoItem &item);
const QDBusArgument &
operator>>(const QDBusArgument &arg, RoomInfoItem &item);
} // nheko::dbus

QDBusArgument &
operator<<(QDBusArgument &arg, const QImage &image);
const QDBusArgument &
operator>>(const QDBusArgument &arg, QImage &);

#define NHEKO_DBUS_SERVICE_NAME "im.nheko.Nheko"

#endif // NHEKODBUSAPI_H
