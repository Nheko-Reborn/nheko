// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "CompletionProxyModel.h"

#include <QRegularExpression>

#include "CompletionModelRoles.h"
#include "Logging.h"
#include "Utils.h"

CompletionProxyModel::CompletionProxyModel(QAbstractItemModel *model, QObject *parent)
  : QAbstractProxyModel(parent)
{
        setSourceModel(model);
        QRegularExpression splitPoints("\\s+|-");

        for (int i = 0; i < sourceModel()->rowCount(); i++) {
                if (i < 7)
                        mapping.push_back(i);

                auto string1 = sourceModel()
                                 ->data(sourceModel()->index(i, 0), CompletionModel::SearchRole)
                                 .toString()
                                 .toLower();
                trie_.insert(string1.toUcs4(), i);

                for (const auto &e : string1.split(splitPoints)) {
                        if (!e.isEmpty()) // NOTE(Nico): Use Qt::SkipEmptyParts in Qt 5.14
                                trie_.insert(e.toUcs4(), i);
                }

                auto string2 = sourceModel()
                                 ->data(sourceModel()->index(i, 0), CompletionModel::SearchRole2)
                                 .toString()
                                 .toLower();

                if (!string2.isEmpty()) {
                        trie_.insert(string2.toUcs4(), i);
                        for (const auto &e : string2.split(splitPoints)) {
                                if (!e.isEmpty()) // NOTE(Nico): Use Qt::SkipEmptyParts in Qt 5.14
                                        trie_.insert(e.toUcs4(), i);
                        }
                }
        }

        connect(this,
                &CompletionProxyModel::newSearchString,
                this,
                [this](QString s) {
                        s.remove(":");
                        s.remove("@");
                        searchString = s.toLower();
                        invalidate();
                },
                Qt::QueuedConnection);
}

void
CompletionProxyModel::invalidate()
{
        auto key = searchString.toUcs4();
        beginResetModel();
        mapping = trie_.search(key, 7);
        endResetModel();

        std::string temp;
        for (auto v : mapping) {
                temp += std::to_string(v) + ", ";
        }
        nhlog::ui()->debug("mapping: {}", temp);
}

QHash<int, QByteArray>
CompletionProxyModel::roleNames() const
{
        return this->sourceModel()->roleNames();
}

int
CompletionProxyModel::rowCount(const QModelIndex &) const
{
        return (int)mapping.size();
}

QModelIndex
CompletionProxyModel::mapFromSource(const QModelIndex &sourceIndex) const
{
        for (int i = 0; i < (int)mapping.size(); i++) {
                if (mapping[i] == sourceIndex.row()) {
                        return index(i, 0);
                }
        }
        return QModelIndex();
}

QModelIndex
CompletionProxyModel::mapToSource(const QModelIndex &proxyIndex) const
{
        auto row = proxyIndex.row();
        if (row < 0 || row >= (int)mapping.size())
                return QModelIndex();

        return sourceModel()->index(mapping[row], 0);
}

QModelIndex
CompletionProxyModel::index(int row, int column, const QModelIndex &) const
{
        return createIndex(row, column);
}

QModelIndex
CompletionProxyModel::parent(const QModelIndex &) const
{
        return QModelIndex{};
}
int
CompletionProxyModel::columnCount(const QModelIndex &) const
{
        return sourceModel()->columnCount();
}

QVariant
CompletionProxyModel::completionAt(int i) const
{
        if (i >= 0 && i < rowCount())
                return data(index(i, 0), CompletionModel::CompletionRole);
        else
                return {};
}

void
CompletionProxyModel::setSearchString(QString s)
{
        emit newSearchString(s);
}
