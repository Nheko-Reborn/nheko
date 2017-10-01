/*
 * nheko Copyright (C) 2017  Konstantinos Sideris <siderisk@auth.gr>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <QDir>
#include <lmdb++.h>

#include "RoomState.h"

class Cache
{
public:
        Cache(const QString &userId);

        void setState(const QString &nextBatchToken, const QMap<QString, RoomState> &states);
        bool isInitialized() const;

        QString nextBatchToken() const;
        QMap<QString, RoomState> states();

        inline void deleteData();
        inline void unmount();
        inline QString memberDbName(const QString &roomid);

        void removeRoom(const QString &roomid);

private:
        void setNextBatchToken(lmdb::txn &txn, const QString &token);
        void insertRoomState(lmdb::txn &txn, const QString &roomid, const RoomState &state);

        lmdb::env env_;
        lmdb::dbi stateDb_;
        lmdb::dbi roomDb_;

        bool isMounted_;

        QString userId_;
        QString cacheDirectory_;
};

inline void
Cache::unmount()
{
        isMounted_ = false;
}

inline QString
Cache::memberDbName(const QString &roomid)
{
        return QString("m.%1").arg(roomid);
}

inline void
Cache::deleteData()
{
        if (!cacheDirectory_.isEmpty())
                QDir(cacheDirectory_).removeRecursively();
}
