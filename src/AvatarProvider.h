// SPDX-FileCopyrightText: 2017 Konstantinos Sideris <siderisk@auth.gr>
// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QPixmap>
#include <functional>

class AvatarProxy : public QObject
{
        Q_OBJECT

signals:
        void avatarDownloaded(const QByteArray &data);
};

using AvatarCallback = std::function<void(QPixmap)>;

namespace AvatarProvider {
void
resolve(const QString &avatarUrl, int size, QObject *receiver, AvatarCallback cb);
void
resolve(const QString &room_id,
        const QString &user_id,
        int size,
        QObject *receiver,
        AvatarCallback cb);
}
