#include "RoomSettings.h"

#include <mtx/responses/common.hpp>
#include <mtx/responses/media.hpp>

#include "Cache.h"
#include "Logging.h"

RoomSettings::RoomSettings(QString roomid, QObject *parent)
  : roomid_{std::move(roomid)}
  , QObject(parent)
{
        retrieveRoomInfo();
}

QString
RoomSettings::roomName() const
{
        return QString(info_.name.c_str());
}

void
RoomSettings::retrieveRoomInfo()
{
        try {
                usesEncryption_ = cache::isRoomEncrypted(roomid_.toStdString());
                info_           = cache::singleRoomInfo(roomid_.toStdString());
                //setAvatar();
        } catch (const lmdb::error &) {
                nhlog::db()->warn("failed to retrieve room info from cache: {}",
                                  roomid_.toStdString());
        }
}