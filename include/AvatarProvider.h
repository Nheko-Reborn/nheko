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

#include <QImage>
#include <QSharedPointer>
#include <QUrl>

class MatrixClient;
class TimelineItem;

struct AvatarData
{
        QImage img;
        QUrl url;
};

class AvatarProvider : public QObject
{
        Q_OBJECT

public:
        static void init(QSharedPointer<MatrixClient> client);
        static void resolve(const QString &userId, TimelineItem *item);
        static void setAvatarUrl(const QString &userId, const QUrl &url);

        static void clear();

private:
        static void updateAvatar(const QString &uid, const QImage &img);

        static QSharedPointer<MatrixClient> client_;

        using UserID = QString;
        static QMap<UserID, AvatarData> avatars_;
        static QMap<UserID, QList<TimelineItem *>> toBeResolved_;
};
