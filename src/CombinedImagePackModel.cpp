// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "CombinedImagePackModel.h"

#include "Cache_p.h"
#include "CompletionModelRoles.h"

CombinedImagePackModel::CombinedImagePackModel(const std::string &roomId,
                                               bool stickers,
                                               QObject *parent)
  : QAbstractListModel(parent)
  , room_id(roomId)
{
    auto packs = cache::client()->getImagePacks(room_id, stickers);

    for (const auto &pack : packs) {
        QString packname =
          pack.pack.pack ? QString::fromStdString(pack.pack.pack->display_name) : QString();

        for (const auto &img : pack.pack.images) {
            ImageDesc i{};
            i.shortcode = QString::fromStdString(img.first);
            i.packname  = packname;
            i.image     = img.second;
            images.push_back(std::move(i));
        }
    }
}

int
CombinedImagePackModel::rowCount(const QModelIndex &) const
{
    return (int)images.size();
}

QHash<int, QByteArray>
CombinedImagePackModel::roleNames() const
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
CombinedImagePackModel::data(const QModelIndex &index, int role) const
{
    if (hasIndex(index.row(), index.column(), index.parent())) {
        switch (role) {
        case CompletionModel::CompletionRole:
            return QStringLiteral(
                     "<img data-mx-emoticon height=\"32\" src=\"%1\" alt=\"%2\" title=\"%2\">")
              .arg(QString::fromStdString(images[index.row()].image.url).toHtmlEscaped(),
                   !images[index.row()].image.body.empty()
                     ? QString::fromStdString(images[index.row()].image.body)
                     : images[index.row()].shortcode);
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
