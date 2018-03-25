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

#include "AvatarProvider.h"
#include "MatrixClient.h"

QSharedPointer<MatrixClient> AvatarProvider::client_;

std::map<QString, AvatarData> AvatarProvider::avatars_;

void
AvatarProvider::init(QSharedPointer<MatrixClient> client)
{
        client_ = client;
}

void
AvatarProvider::updateAvatar(const QString &uid, const QImage &img)
{
        auto avatarData = &avatars_[uid];
        avatarData->img = img;
}

void
AvatarProvider::resolve(const QString &userId,
                        QObject *receiver,
                        std::function<void(QImage)> callback)
{
        if (avatars_.find(userId) == avatars_.end())
                return;

        auto img = avatars_[userId].img;

        if (!img.isNull()) {
                callback(img);
                return;
        }

        auto proxy = client_->fetchUserAvatar(avatars_[userId].url);

        if (proxy.isNull())
                return;

        connect(proxy.data(),
                &DownloadMediaProxy::avatarDownloaded,
                receiver,
                [userId, proxy, callback](const QImage &img) {
                        proxy->deleteLater();
                        updateAvatar(userId, img);
                        callback(img);
                });
}

void
AvatarProvider::setAvatarUrl(const QString &userId, const QUrl &url)
{
        AvatarData data;
        data.url = url;

        avatars_.emplace(userId, data);
}
