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
std::map<QString, std::vector<std::function<void(QImage)>>> AvatarProvider::toBeResolved_;

void
AvatarProvider::init(QSharedPointer<MatrixClient> client)
{
        client_ = client;
}

void
AvatarProvider::updateAvatar(const QString &uid, const QImage &img)
{
        if (toBeResolved_.find(uid) != toBeResolved_.end()) {
                auto callbacks = toBeResolved_[uid];

                // Update all the timeline items with the resolved avatar.
                for (const auto &callback : callbacks)
                        callback(img);

                toBeResolved_.erase(uid);
        }

        auto avatarData = &avatars_[uid];
        avatarData->img = img;
}

void
AvatarProvider::resolve(const QString &userId, std::function<void(QImage)> callback)
{
        if (avatars_.find(userId) == avatars_.end())
                return;

        auto img = avatars_[userId].img;

        if (!img.isNull()) {
                callback(img);
                return;
        }

        // Add the current timeline item to the waiting list for this avatar.
        if (toBeResolved_.find(userId) == toBeResolved_.end()) {
                client_->fetchUserAvatar(avatars_[userId].url,
                                         [userId](QImage image) { updateAvatar(userId, image); },
                                         [userId](QString error) {
                                                 qWarning()
                                                   << error << ": failed to retrieve user avatar"
                                                   << userId;
                                         });

                std::vector<std::function<void(QImage)>> items;
                items.emplace_back(callback);

                toBeResolved_.emplace(userId, items);
        } else {
                toBeResolved_[userId].emplace_back(callback);
        }
}

void
AvatarProvider::setAvatarUrl(const QString &userId, const QUrl &url)
{
        AvatarData data;
        data.url = url;

        avatars_.emplace(userId, data);
}

void
AvatarProvider::clear()
{
        avatars_.clear();
        toBeResolved_.clear();
}
