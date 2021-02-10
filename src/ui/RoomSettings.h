#pragma once

#include <QObject>
#include <QString>

#include "CacheStructs.h"

class RoomSettings : public QObject
{
		Q_OBJECT
        Q_PROPERTY(QString roomName READ roomName CONSTANT)

public:
        RoomSettings(QString roomid, QObject *parent = nullptr);

        QString roomName() const;

private:
        void retrieveRoomInfo();

private:
        QString roomid_;
        bool usesEncryption_ = false;
        RoomInfo info_;
};