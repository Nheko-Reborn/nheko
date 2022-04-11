// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ROOMLISTDBUSINTERFACE_H
#define ROOMLISTDBUSINTERFACE_H

#include <QDBusMessage>
#include <QMutex>
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

private:
    void bringWindowToTop() const;

    RoomlistModel *m_parent;

    // this is a QSharedPointer so that copy ops are less expensive (see below)
    QSharedPointer<QVector<nheko::dbus::RoomInfoItem>> m_model{
      new QVector<nheko::dbus::RoomInfoItem>};
    QMutex m_modelAccess;
};

#endif // ROOMLISTDBUSINTERFACE_H
