// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "SingleImagePackModel.h"

#include "Cache_p.h"
#include "MatrixClient.h"

SingleImagePackModel::SingleImagePackModel(ImagePackInfo pack_, QObject *parent)
  : QAbstractListModel(parent)
  , roomid_(std::move(pack_.source_room))
  , statekey_(std::move(pack_.state_key))
  , pack(std::move(pack_.pack))
{
        if (!pack.pack)
                pack.pack = mtx::events::msc2545::ImagePack::PackDescription{};

        for (const auto &e : pack.images)
                shortcodes.push_back(e.first);
}

int
SingleImagePackModel::rowCount(const QModelIndex &) const
{
        return (int)shortcodes.size();
}

QHash<int, QByteArray>
SingleImagePackModel::roleNames() const
{
        return {
          {Roles::Url, "url"},
          {Roles::ShortCode, "shortCode"},
          {Roles::Body, "body"},
          {Roles::IsEmote, "isEmote"},
          {Roles::IsSticker, "isSticker"},
        };
}

QVariant
SingleImagePackModel::data(const QModelIndex &index, int role) const
{
        if (hasIndex(index.row(), index.column(), index.parent())) {
                const auto &img = pack.images.at(shortcodes.at(index.row()));
                switch (role) {
                case Url:
                        return QString::fromStdString(img.url);
                case ShortCode:
                        return QString::fromStdString(shortcodes.at(index.row()));
                case Body:
                        return QString::fromStdString(img.body);
                case IsEmote:
                        return img.overrides_usage() ? img.is_emoji() : pack.pack->is_emoji();
                case IsSticker:
                        return img.overrides_usage() ? img.is_sticker() : pack.pack->is_sticker();
                default:
                        return {};
                }
        }
        return {};
}

bool
SingleImagePackModel::isGloballyEnabled() const
{
        if (auto roomPacks =
              cache::client()->getAccountData(mtx::events::EventType::ImagePackRooms)) {
                if (auto tmp = std::get_if<
                      mtx::events::EphemeralEvent<mtx::events::msc2545::ImagePackRooms>>(
                      &*roomPacks)) {
                        if (tmp->content.rooms.count(roomid_) &&
                            tmp->content.rooms.at(roomid_).count(statekey_))
                                return true;
                }
        }
        return false;
}
void
SingleImagePackModel::setGloballyEnabled(bool enabled)
{
        mtx::events::msc2545::ImagePackRooms content{};
        if (auto roomPacks =
              cache::client()->getAccountData(mtx::events::EventType::ImagePackRooms)) {
                if (auto tmp = std::get_if<
                      mtx::events::EphemeralEvent<mtx::events::msc2545::ImagePackRooms>>(
                      &*roomPacks)) {
                        content = tmp->content;
                }
        }

        if (enabled)
                content.rooms[roomid_][statekey_] = {};
        else
                content.rooms[roomid_].erase(statekey_);

        http::client()->put_account_data(content, [this](mtx::http::RequestErr) {
                // emit this->globallyEnabledChanged();
        });
}
