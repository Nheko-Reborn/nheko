// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ImagePackModel.h"

#include "Cache_p.h"
#include "CompletionModelRoles.h"

ImagePackModel::ImagePackModel(const std::string &roomId, bool stickers, QObject *parent)
  : QAbstractListModel(parent)
  , room_id(roomId)
{
        auto accountpackV =
          cache::client()->getAccountData(mtx::events::EventType::ImagePackInAccountData);
        auto enabledRoomPacksV =
          cache::client()->getAccountData(mtx::events::EventType::ImagePackRooms);

        std::optional<mtx::events::msc2545::ImagePack> accountPack;
        if (accountpackV) {
                auto tmp =
                  std::get_if<mtx::events::EphemeralEvent<mtx::events::msc2545::ImagePack>>(
                    &*accountpackV);
                if (tmp)
                        accountPack = tmp->content;
        }
        // mtx::events::msc2545::ImagePackRooms *enabledRoomPacks = nullptr;
        // if (enabledRoomPacksV)
        //        enabledRoomPacks =
        //          std::get_if<mtx::events::msc2545::ImagePackRooms>(&*enabledRoomPacksV);

        if (accountPack && (!accountPack->pack || (stickers ? accountPack->pack->is_sticker()
                                                            : accountPack->pack->is_emoji()))) {
                QString packname;
                if (accountPack->pack)
                        packname = QString::fromStdString(accountPack->pack->display_name);

                for (const auto &img : accountPack->images) {
                        if (img.second.overrides_usage() &&
                            (stickers ? !img.second.is_sticker() : !img.second.is_emoji()))
                                continue;

                        ImageDesc i{};
                        i.shortcode = QString::fromStdString(img.first);
                        i.packname  = packname;
                        i.image     = img.second;
                        images.push_back(std::move(i));
                }
        }
}

QHash<int, QByteArray>
ImagePackModel::roleNames() const
{
        return {
          {CompletionModel::CompletionRole, "completionRole"},
          {CompletionModel::SearchRole, "searchRole"},
          {CompletionModel::SearchRole2, "searchRole2"},
          {Roles::Url, "url"},
          {Roles::ShortCode, "shortcode"},
          {Roles::Body, "body"},
          {Roles::PackName, "packname"},
          {Roles::OriginalRow, "originalRow"},
        };
}

QVariant
ImagePackModel::data(const QModelIndex &index, int role) const
{
        if (hasIndex(index.row(), index.column(), index.parent())) {
                switch (role) {
                case CompletionModel::CompletionRole:
                        return QString::fromStdString(images[index.row()].image.url);
                case Roles::Url:
                        return QString::fromStdString(images[index.row()].image.url);
                case CompletionModel::SearchRole:
                case Roles::ShortCode:
                        return images[index.row()].shortcode;
                case CompletionModel::SearchRole2:
                case Roles::Body:
                        return QString::fromStdString(images[index.row()].image.body);
                case Roles::PackName:
                        return images[index.row()].packname;
                case Roles::OriginalRow:
                        return index.row();
                default:
                        return {};
                }
        }
        return {};
}
