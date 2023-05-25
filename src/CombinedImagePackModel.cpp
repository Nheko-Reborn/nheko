// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "CombinedImagePackModel.h"

#include "Cache_p.h"
#include "CompletionModelRoles.h"
#include "emoji/Provider.h"

CombinedImagePackModel::CombinedImagePackModel(const std::string &roomId, QObject *parent)
  : QAbstractListModel(parent)
  , room_id(roomId)
{
    auto packs = cache::client()->getImagePacks(room_id, false);

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
    return (int)(emoji::Provider::emoji.size() + images.size());
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
      {Roles::Unicode, "unicode"},
    };
}

QVariant
CombinedImagePackModel::data(const QModelIndex &index, int role) const
{
    using emoji::Provider;
    if (hasIndex(index.row(), index.column(), index.parent())) {
        if (index.row() < (int)emoji::Provider::emoji.size()) {
            switch (role) {
            case CompletionModel::CompletionRole:
            case Roles::Unicode:
                return emoji::Provider::emoji[index.row()].unicode();

            case Qt::ToolTipRole:
                return Provider::emoji[index.row()].shortName() + ", " +
                       Provider::emoji[index.row()].unicodeName();
            case CompletionModel::SearchRole2:
            case Roles::Body:
                return Provider::emoji[index.row()].unicodeName();
            case CompletionModel::SearchRole:
            case Roles::ShortCode:
                return Provider::emoji[index.row()].shortName();
            case Roles::PackName:
                return emoji::categoryToName(Provider::emoji[index.row()].category);
            default:
                return {};
            }
        } else {
            int row = index.row() - static_cast<int>(emoji::Provider::emoji.size());
            switch (role) {
            case CompletionModel::CompletionRole:
                return QStringLiteral(
                         "<img data-mx-emoticon height=\"32\" src=\"%1\" alt=\"%2\" title=\"%2\">")
                  .arg(QString::fromStdString(images[row].image.url).toHtmlEscaped(),
                       !images[row].image.body.empty()
                         ? QString::fromStdString(images[row].image.body)
                         : images[row].shortcode);
            case Roles::Url:
                return QString::fromStdString(images[row].image.url);
            case CompletionModel::SearchRole:
            case Roles::ShortCode:
                return images[row].shortcode;
            case CompletionModel::SearchRole2:
            case Roles::Body:
                return QString::fromStdString(images[row].image.body);
            case Roles::PackName:
                return images[row].packname;
            case Roles::Unicode:
                return QString();
            default:
                return {};
            }
        }
    }
    return {};
}
