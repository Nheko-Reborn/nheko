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

#include <QBuffer>
#include <QtConcurrent>

#include "AvatarProvider.h"
#include "Cache.h"
#include "MatrixClient.h"

QSharedPointer<Cache> AvatarProvider::cache_;

void
AvatarProvider::resolve(const QString &room_id,
                        const QString &user_id,
                        QObject *receiver,
                        std::function<void(QImage)> callback)
{
        const auto key       = QString("%1 %2").arg(room_id).arg(user_id);
        const auto avatarUrl = Cache::avatarUrl(room_id, user_id);

        if (!Cache::AvatarUrls.contains(key) || cache_.isNull())
                return;

        if (avatarUrl.isEmpty())
                return;

        auto data = cache_->image(avatarUrl);
        if (!data.isNull()) {
                callback(QImage::fromData(data));
                return;
        }

        auto proxy = http::client()->fetchUserAvatar(avatarUrl);

        if (proxy.isNull())
                return;

        connect(proxy.data(),
                &DownloadMediaProxy::avatarDownloaded,
                receiver,
                [user_id, proxy, callback, avatarUrl](const QImage &img) {
                        proxy->deleteLater();
                        QtConcurrent::run([img, avatarUrl]() {
                                QByteArray data;
                                QBuffer buffer(&data);
                                buffer.open(QIODevice::WriteOnly);
                                img.save(&buffer, "PNG");

                                cache_->saveImage(avatarUrl, data);
                        });
                        callback(img);
                });
}
