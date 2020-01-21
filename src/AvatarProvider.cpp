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
#include <QPixmapCache>
#include <memory>
#include <unordered_map>

#include "AvatarProvider.h"
#include "Cache.h"
#include "Logging.h"
#include "MatrixClient.h"

static QPixmapCache avatar_cache;

namespace AvatarProvider {
void
resolve(const QString &avatarUrl, int size, QObject *receiver, AvatarCallback callback)
{
        const auto cacheKey = QString("%1_size_%2").arg(avatarUrl).arg(size);

        if (avatarUrl.isEmpty())
                return;

        QPixmap pixmap;
        if (avatar_cache.find(cacheKey, &pixmap)) {
                callback(pixmap);
                return;
        }

        auto data = cache::image(avatarUrl);
        if (!data.isNull()) {
                pixmap.loadFromData(data);
                avatar_cache.insert(cacheKey, pixmap);
                callback(pixmap);
                return;
        }

        auto proxy = std::make_shared<AvatarProxy>();
        QObject::connect(proxy.get(),
                         &AvatarProxy::avatarDownloaded,
                         receiver,
                         [callback, cacheKey](const QByteArray &data) {
                                 QPixmap pm;
                                 pm.loadFromData(data);
                                 avatar_cache.insert(cacheKey, pm);
                                 callback(pm);
                         });

        mtx::http::ThumbOpts opts;
        opts.width   = size;
        opts.height  = size;
        opts.mxc_url = avatarUrl.toStdString();

        http::client()->get_thumbnail(
          opts,
          [opts, proxy = std::move(proxy)](const std::string &res, mtx::http::RequestErr err) {
                  if (err) {
                          nhlog::net()->warn("failed to download avatar: {} - ({} {})",
                                             opts.mxc_url,
                                             mtx::errors::to_string(err->matrix_error.errcode),
                                             err->matrix_error.error);
                          return;
                  }

                  cache::saveImage(opts.mxc_url, res);

                  emit proxy->avatarDownloaded(QByteArray(res.data(), res.size()));
          });
}

void
resolve(const QString &room_id,
        const QString &user_id,
        int size,
        QObject *receiver,
        AvatarCallback callback)
{
        const auto avatarUrl = cache::avatarUrl(room_id, user_id);

        resolve(avatarUrl, size, receiver, callback);
}
}
