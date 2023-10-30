// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QPixmap>

#include <functional>

using AvatarCallback = std::function<void(QPixmap)>;

class AvatarProxy final : public QObject
{
    Q_OBJECT

signals:
    void avatarDownloaded(QPixmap pm);
};

namespace AvatarProvider {
void
resolve(QString avatarUrl, int size, QObject *receiver, AvatarCallback cb);
void
resolve(const QString &room_id,
        const QString &user_id,
        int size,
        QObject *receiver,
        AvatarCallback cb);
}
