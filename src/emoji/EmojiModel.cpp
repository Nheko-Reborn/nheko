// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "EmojiModel.h"

#include <Cache.h>
#include <MatrixClient.h>

#include "CompletionModelRoles.h"

using namespace emoji;

int
EmojiModel::categoryToIndex(int category)
{
    auto dist = std::distance(
      Provider::emoji.begin(),
      std::lower_bound(Provider::emoji.begin(),
                       Provider::emoji.end(),
                       static_cast<Emoji::Category>(category),
                       [](const struct Emoji &e, Emoji::Category c) { return e.category < c; }));

    return static_cast<int>(dist);
}

QHash<int, QByteArray>
EmojiModel::roleNames() const
{
    static QHash<int, QByteArray> roles;

    if (roles.isEmpty()) {
        roles                                                   = QAbstractListModel::roleNames();
        roles[static_cast<int>(EmojiModel::Roles::Unicode)]     = QByteArrayLiteral("unicode");
        roles[static_cast<int>(EmojiModel::Roles::ShortName)]   = QByteArrayLiteral("shortName");
        roles[static_cast<int>(EmojiModel::Roles::UnicodeName)] = QByteArrayLiteral("unicodeName");
        roles[static_cast<int>(EmojiModel::Roles::Category)]    = QByteArrayLiteral("category");
        roles[static_cast<int>(EmojiModel::Roles::Emoji)]       = QByteArrayLiteral("emoji");
    }

    return roles;
}

int
EmojiModel::rowCount(const QModelIndex &parent) const
{
    return parent == QModelIndex() ? Provider::emoji.size() : 0;
}

QVariant
EmojiModel::data(const QModelIndex &index, int role) const
{
    if (hasIndex(index.row(), index.column(), index.parent())) {
        switch (role) {
        case Qt::DisplayRole:
        case CompletionModel::CompletionRole:
        case static_cast<int>(EmojiModel::Roles::Unicode):
            return Provider::emoji[index.row()].unicode;

        case Qt::ToolTipRole:
            return Provider::emoji[index.row()].shortName + ", " +
                   Provider::emoji[index.row()].unicodeName;
        case CompletionModel::SearchRole2:
        case static_cast<int>(EmojiModel::Roles::UnicodeName):
            return Provider::emoji[index.row()].unicodeName;
        case CompletionModel::SearchRole:
        case static_cast<int>(EmojiModel::Roles::ShortName):
            return Provider::emoji[index.row()].shortName;
        case static_cast<int>(EmojiModel::Roles::Category):
            return QVariant::fromValue(Provider::emoji[index.row()].category);

        case static_cast<int>(EmojiModel::Roles::Emoji):
            return QVariant::fromValue(Provider::emoji[index.row()]);
        }
    }

    return {};
}
