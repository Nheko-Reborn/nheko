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

private slots:
    void prepareModel();

private:
    void bringWindowToTop() const;

    RoomlistModel *m_parent;

    // this is a QSharedPointer so that copy ops are less expensive (see below)
    QSharedPointer<QVector<nheko::dbus::RoomInfoItem>> m_model{
      new QVector<nheko::dbus::RoomInfoItem>};
    // Use this to lock the entire model for access. This prevents potentially interesting race
    // conditions when copying from the staging area.
    QMutex m_modelAccess;

    // use this to store data while generating a new model; then copy it to m_model
    QSharedPointer<QVector<nheko::dbus::RoomInfoItem>> m_stagingModel{
      new QVector<nheko::dbus::RoomInfoItem>};
    // this locks the entire staging model during a refresh
    QMutex m_modifyStagingDataMutex;
    // use this to guard while adding individual items to the staging model
    QMutex m_addItemsToStagingData;
};

#endif // ROOMLISTDBUSINTERFACE_H
