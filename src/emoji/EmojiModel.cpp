// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "EmojiModel.h"

#include <Cache.h>
#include <MatrixClient.h>

#include "CompletionModelRoles.h"

using namespace emoji;

QHash<int, QByteArray>
EmojiModel::roleNames() const
{
        static QHash<int, QByteArray> roles;

        if (roles.isEmpty()) {
                roles = QAbstractListModel::roleNames();
                roles[static_cast<int>(EmojiModel::Roles::Unicode)] = QByteArrayLiteral("unicode");
                roles[static_cast<int>(EmojiModel::Roles::ShortName)] =
                  QByteArrayLiteral("shortName");
                roles[static_cast<int>(EmojiModel::Roles::Category)] =
                  QByteArrayLiteral("category");
                roles[static_cast<int>(EmojiModel::Roles::Emoji)] = QByteArrayLiteral("emoji");
        }

        return roles;
}

int
EmojiModel::rowCount(const QModelIndex &parent) const
{
        return parent == QModelIndex() ? Provider::emoji.count() : 0;
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

EmojiProxyModel::EmojiProxyModel(QObject *parent)
  : QSortFilterProxyModel(parent)
{}

EmojiProxyModel::~EmojiProxyModel() {}

Emoji::Category
EmojiProxyModel::category() const
{
        return category_;
}

void
EmojiProxyModel::setCategory(Emoji::Category cat)
{
        if (category_ == cat) {
                return;
        }

        category_ = cat;
        emit categoryChanged();

        invalidateFilter();
}

QString
EmojiProxyModel::filter() const
{
        return filterRegExp().pattern();
}

void
EmojiProxyModel::setFilter(const QString &filter)
{
        if (filterRegExp().pattern() == filter) {
                return;
        }

        setFilterWildcard(filter);
        emit filterChanged();
}

bool
EmojiProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
        const QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
        const Emoji emoji = index.data(static_cast<int>(EmojiModel::Roles::Emoji)).value<Emoji>();

        // TODO: Add favorites / recently used
        if (category_ != Emoji::Category::Search) {
                return emoji.category == category_;
        }

        return filterRegExp().isEmpty() ? true : filterRegExp().indexIn(emoji.shortName) != -1;
}
