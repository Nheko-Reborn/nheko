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
#include "Cache.h"
#include "MatrixClient.h"

QSharedPointer<MatrixClient> AvatarProvider::client_;
QHash<QString, QImage> AvatarProvider::avatars_;

void
AvatarProvider::resolve(const QString &room_id,
                        const QString &user_id,
                        QObject *receiver,
                        std::function<void(QImage)> callback)
{
        const auto key = QString("%1 %2").arg(room_id).arg(user_id);

        if (!Cache::AvatarUrls.contains(key))
                return;

        if (avatars_.contains(key)) {
                auto img = avatars_[key];

                if (!img.isNull()) {
                        callback(img);
                        return;
                }
        }

        auto proxy = client_->fetchUserAvatar(Cache::avatarUrl(room_id, user_id));

        if (proxy.isNull())
                return;

        connect(proxy.data(),
                &DownloadMediaProxy::avatarDownloaded,
                receiver,
                [user_id, proxy, callback, key](const QImage &img) {
                        proxy->deleteLater();
                        avatars_.insert(key, img);
                        callback(img);
                });
}
