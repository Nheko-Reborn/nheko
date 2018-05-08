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

#include <QHash>
#include <QImage>
#include <QSharedPointer>
#include <functional>

class TimelineItem;
class Cache;

class AvatarProvider : public QObject
{
        Q_OBJECT

public:
        static void init(QSharedPointer<Cache> cache) { cache_ = cache; }
        //! The callback is called with the downloaded avatar for the given user
        //! or the avatar is downloaded first and then saved for re-use.
        static void resolve(const QString &room_id,
                            const QString &userId,
                            QObject *receiver,
                            std::function<void(QImage)> callback);

private:
        static QSharedPointer<Cache> cache_;
};
