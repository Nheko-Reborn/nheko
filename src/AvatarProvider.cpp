// SPDX-FileCopyrightText: 2017 Konstantinos Sideris <siderisk@auth.gr>
// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QBuffer>
#include <QPixmapCache>
#include <QPointer>
#include <memory>
#include <unordered_map>

#include "AvatarProvider.h"
#include "Cache.h"
#include "Logging.h"
#include "MatrixClient.h"
#include "MxcImageProvider.h"
#include "Utils.h"

static QPixmapCache avatar_cache;

namespace AvatarProvider {
void
resolve(QString avatarUrl, int size, QObject *receiver, AvatarCallback callback)
{
    const auto cacheKey = QStringLiteral("%1_size_%2").arg(avatarUrl).arg(size);

    QPixmap pixmap;
    if (avatarUrl.isEmpty()) {
        callback(pixmap);
        return;
    }

    if (avatar_cache.find(cacheKey, &pixmap)) {
        callback(pixmap);
        return;
    }

    MxcImageProvider::download(avatarUrl.remove(QStringLiteral("mxc://")),
                               QSize(size, size),
                               [callback, cacheKey, recv = QPointer<QObject>(receiver)](
                                 QString, QSize, QImage img, QString) {
                                   if (!recv)
                                       return;

                                   auto proxy = std::make_shared<AvatarProxy>();
                                   QObject::connect(proxy.get(),
                                                    &AvatarProxy::avatarDownloaded,
                                                    recv,
                                                    [callback, cacheKey](QPixmap pm) {
                                                        if (!pm.isNull())
                                                            avatar_cache.insert(cacheKey, pm);
                                                        callback(pm);
                                                    });

                                   if (img.isNull()) {
                                       emit proxy->avatarDownloaded(QPixmap{});
                                       return;
                                   }

                                   auto pm = QPixmap::fromImage(std::move(img));
                                   emit proxy->avatarDownloaded(pm);
                               });
}

void
resolve(const QString &room_id,
        const QString &user_id,
        int size,
        QObject *receiver,
        AvatarCallback callback)
{
    auto avatarUrl = cache::avatarUrl(room_id, user_id);

    resolve(std::move(avatarUrl), size, receiver, callback);
}
}
