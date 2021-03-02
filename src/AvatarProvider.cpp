// SPDX-FileCopyrightText: 2017 Konstantinos Sideris <siderisk@auth.gr>
// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QBuffer>
#include <QPixmapCache>
#include <memory>
#include <unordered_map>

#include "AvatarProvider.h"
#include "Cache.h"
#include "Logging.h"
#include "MatrixClient.h"
#include "Utils.h"

static QPixmapCache avatar_cache;

namespace AvatarProvider {
void
resolve(const QString &avatarUrl, int size, QObject *receiver, AvatarCallback callback)
{
        const auto cacheKey = QString("%1_size_%2").arg(avatarUrl).arg(size);

        QPixmap pixmap;
        if (avatarUrl.isEmpty()) {
                callback(pixmap);
                return;
        }

        if (avatar_cache.find(cacheKey, &pixmap)) {
                callback(pixmap);
                return;
        }

        auto data = cache::image(cacheKey);
        if (!data.isNull()) {
                pixmap = QPixmap::fromImage(utils::readImage(data));
                avatar_cache.insert(cacheKey, pixmap);
                callback(pixmap);
                return;
        }

        auto proxy = std::make_shared<AvatarProxy>();
        QObject::connect(proxy.get(),
                         &AvatarProxy::avatarDownloaded,
                         receiver,
                         [callback, cacheKey](QByteArray data) {
                                 QPixmap pm = QPixmap::fromImage(utils::readImage(data));
                                 avatar_cache.insert(cacheKey, pm);
                                 callback(pm);
                         });

        mtx::http::ThumbOpts opts;
        opts.width   = size;
        opts.height  = size;
        opts.mxc_url = avatarUrl.toStdString();

        http::client()->get_thumbnail(
          opts,
          [opts, cacheKey, proxy = std::move(proxy)](const std::string &res,
                                                     mtx::http::RequestErr err) {
                  if (err) {
                          nhlog::net()->warn("failed to download avatar: {} - ({} {})",
                                             opts.mxc_url,
                                             mtx::errors::to_string(err->matrix_error.errcode),
                                             err->matrix_error.error);
                  } else {
                          cache::saveImage(cacheKey.toStdString(), res);
                  }

                  emit proxy->avatarDownloaded(QByteArray(res.data(), (int)res.size()));
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
