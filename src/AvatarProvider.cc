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
#include <memory>

#include "AvatarProvider.h"
#include "Cache.h"
#include "Logging.hpp"
#include "MatrixClient.h"

namespace AvatarProvider {

void
resolve(const QString &room_id, const QString &user_id, QObject *receiver, AvatarCallback callback)
{
        const auto key       = QString("%1 %2").arg(room_id).arg(user_id);
        const auto avatarUrl = Cache::avatarUrl(room_id, user_id);

        if (!Cache::AvatarUrls.contains(key) || !cache::client())
                return;

        if (avatarUrl.isEmpty())
                return;

        auto data = cache::client()->image(avatarUrl);
        if (!data.isNull()) {
                callback(QImage::fromData(data));
                return;
        }

        auto proxy = std::make_shared<AvatarProxy>();
        QObject::connect(proxy.get(),
                         &AvatarProxy::avatarDownloaded,
                         receiver,
                         [callback](const QByteArray &data) { callback(QImage::fromData(data)); });

        mtx::http::ThumbOpts opts;
        opts.mxc_url = avatarUrl.toStdString();

        http::v2::client()->get_thumbnail(
          opts,
          [opts, proxy = std::move(proxy)](const std::string &res, mtx::http::RequestErr err) {
                  if (err) {
                          nhlog::net()->warn("failed to download avatar: {} - ({} {})",
                                             opts.mxc_url,
                                             mtx::errors::to_string(err->matrix_error.errcode),
                                             err->matrix_error.error);
                          return;
                  }

                  cache::client()->saveImage(opts.mxc_url, res);

                  auto data = QByteArray(res.data(), res.size());
                  emit proxy->avatarDownloaded(data);
          });
}
}
