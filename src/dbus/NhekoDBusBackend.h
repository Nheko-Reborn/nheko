// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef NHEKODBUSBACKEND_H
#define NHEKODBUSBACKEND_H

#include <QDBusMessage>
#include <QObject>

#include "NhekoDBusApi.h"
#include "config/nheko.h"

class RoomlistModel;

class NhekoDBusBackend : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "im.nheko.Nheko")

public:
    NhekoDBusBackend(RoomlistModel *parent);

public slots:
    //! Get the nheko D-Bus API version.
    Q_SCRIPTABLE QString apiVersion() const { return nheko::dbus::dbusApiVersion.toString(); }
    //! Get the nheko version.
    Q_SCRIPTABLE QString nhekoVersion() const { return nheko::version; }
    //! Call this function to get a list of all joined rooms.
    Q_SCRIPTABLE QVector<nheko::dbus::RoomInfoItem> rooms(const QDBusMessage &message);
    //! Activates a currently joined room.
    Q_SCRIPTABLE void activateRoom(const QString &alias) const;
    //! Joins a room. It is your responsibility to ask for confirmation (if desired).
    Q_SCRIPTABLE void joinRoom(const QString &alias) const;
    //! Starts or activates a direct chat. It is your responsibility to ask for confirmation (if
    //! desired).
    Q_SCRIPTABLE void directChat(const QString &userId) const;

private:
    void bringWindowToTop() const;

    RoomlistModel *m_parent;
};

#endif // NHEKODBUSBACKEND_H
