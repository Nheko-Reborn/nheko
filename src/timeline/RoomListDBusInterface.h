// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ROOMLISTDBUSINTERFACE_H
#define ROOMLISTDBUSINTERFACE_H

#include <QDBusMessage>
#include <QObject>

#include "NhekoDBusInterface.h"

class RoomlistModel;

class RoomListDBusInterface : public QObject
{
    Q_OBJECT

public:
    RoomListDBusInterface(RoomlistModel *parent);

public slots:
    //! Get the nheko D-Bus API version.
    Q_SCRIPTABLE QString apiVersion() const { return nheko::dbus::apiVersion; }
    //! Call this function to get a list of all joined rooms.
    Q_SCRIPTABLE QVector<nheko::dbus::RoomInfoItem> getRooms(const QDBusMessage &message);
    //! Activates a currently joined room.
    Q_SCRIPTABLE void activateRoom(const QString &alias) const;
    //! Joins a room. It is your responsibility to ask for confirmation (if desired).
    Q_SCRIPTABLE void joinRoom(const QString &alias) const;
    //! Starts or activates a direct chat. It is your responsibility to ask for confirmation (if
    //! desired).
    Q_SCRIPTABLE void startDirectChat(const QString &userId) const;

private:
    void bringWindowToTop() const;

    RoomlistModel *m_parent;
};

#endif // ROOMLISTDBUSINTERFACE_H
