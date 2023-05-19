// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "GridImagePackModel.h"

#include "Cache_p.h"
#include "CompletionModelRoles.h"

#include <algorithm>

Q_DECLARE_METATYPE(StickerImage)

GridImagePackModel::GridImagePackModel(const std::string &roomId, bool stickers, QObject *parent)
  : QAbstractListModel(parent)
  , room_id(roomId)
{
    [[maybe_unused]] static auto id = qRegisterMetaType<StickerImage>();

    auto originalPacks = cache::client()->getImagePacks(room_id, stickers);

    for (auto &pack : originalPacks) {
        PackDesc newPack{};
        newPack.packname =
          pack.pack.pack ? QString::fromStdString(pack.pack.pack->display_name) : QString();
        newPack.room_id   = pack.source_room;
        newPack.state_key = pack.state_key;

        newPack.images.resize(pack.pack.images.size());
        std::ranges::transform(std::move(pack.pack.images), newPack.images.begin(), [](auto &&img) {
            return std::pair(std::move(img.second), QString::fromStdString(img.first));
        });

        size_t packRowCount =
          (newPack.images.size() / columns) + (newPack.images.size() % columns ? 1 : 0);
        newPack.firstRow = rowToPack.size();
        for (size_t i = 0; i < packRowCount; i++)
            rowToPack.push_back(packs.size());
        packs.push_back(std::move(newPack));
    }
}

int
GridImagePackModel::rowCount(const QModelIndex &) const
{
    return (int)rowToPack.size();
}

QHash<int, QByteArray>
GridImagePackModel::roleNames() const
{
    return {
      {Roles::PackName, "packname"},
      {Roles::Row, "row"},
    };
}

QVariant
GridImagePackModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < rowCount() && index.row() >= 0) {
        const auto &pack = packs[rowToPack[index.row()]];
        switch (role) {
        case Roles::PackName:
            return pack.packname;
        case Roles::Row: {
            std::size_t offset = static_cast<std::size_t>(index.row()) - pack.firstRow;
            QList<StickerImage> imgs;
            auto endOffset = std::min((offset + 1) * 3, pack.images.size());
            for (std::size_t img = offset * 3; img < endOffset; img++) {
                const auto &data = pack.images.at(img);
                imgs.push_back({.url         = QString::fromStdString(data.first.url),
                                .shortcode   = data.second,
                                .body        = QString::fromStdString(data.first.body),
                                .descriptor_ = std::vector{
                                  pack.room_id,
                                  pack.state_key,
                                  data.second.toStdString(),
                                }});
            }
            return QVariant::fromValue(imgs);
        }
        default:
            return {};
        }
    }
    return {};
}
