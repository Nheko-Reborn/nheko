// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef NHEKODBUSINTERFACE_H
#define NHEKODBUSINTERFACE_H

#include <QDBusArgument>
#include <QIcon>
#include <QObject>

namespace nheko::dbus {

//! Registers all necessary classes with D-Bus. Call this before using any nheko D-Bus classes.
void init();

//! The current nheko D-Bus API version.
const auto apiVersion{QStringLiteral("0.0.1")};

class RoomInfoItem : public QObject
{
    Q_OBJECT

public:
    RoomInfoItem(const QString &mxid  = QString{},
                 const QString &alias = QString{},
                 const QString &title = QString{},
                 const QImage &image  = QImage{},
                 QObject *parent      = nullptr);

    RoomInfoItem(const RoomInfoItem &other);

    const QString &roomId() const { return roomId_; }
    const QString &alias() const { return alias_; }
    const QString &roomName() const { return roomName_; }
    const QImage &image() const { return image_; }

    RoomInfoItem &operator=(const RoomInfoItem &other);
    friend QDBusArgument &operator<<(QDBusArgument &arg, const nheko::dbus::RoomInfoItem &item);
    friend const QDBusArgument &operator>>(const QDBusArgument &arg, nheko::dbus::RoomInfoItem &item);

private:
    QString roomId_;
    QString alias_;
    QString roomName_;
    QImage image_;
};

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

#define NHEKO_DBUS_SERVICE_NAME "io.github.Nheko-Reborn.nheko"

#endif // NHEKODBUSINTERFACE_H
