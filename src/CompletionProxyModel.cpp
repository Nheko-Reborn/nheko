// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "CompletionProxyModel.h"

#include <QRegularExpression>

#include "CompletionModelRoles.h"
#include "Logging.h"
#include "Utils.h"

CompletionProxyModel::CompletionProxyModel(QAbstractItemModel *model,
                                           int max_mistakes,
                                           size_t max_completions,
                                           QObject *parent)
  : QAbstractProxyModel(parent)
  , maxMistakes_(max_mistakes)
  , max_completions_(max_completions)
{
        setSourceModel(model);
        QRegularExpression splitPoints("\\s+|-");

        // insert all the full texts
        for (int i = 0; i < sourceModel()->rowCount(); i++) {
                if (static_cast<size_t>(i) < max_completions_)
                        mapping.push_back(i);

                auto string1 = sourceModel()
                                 ->data(sourceModel()->index(i, 0), CompletionModel::SearchRole)
                                 .toString()
                                 .toLower();
                if (!string1.isEmpty())
                        trie_.insert(string1.toUcs4(), i);

                auto string2 = sourceModel()
                                 ->data(sourceModel()->index(i, 0), CompletionModel::SearchRole2)
                                 .toString()
                                 .toLower();
                if (!string2.isEmpty())
                        trie_.insert(string2.toUcs4(), i);
        }

        // insert the partial matches
        for (int i = 0; i < sourceModel()->rowCount(); i++) {
                auto string1 = sourceModel()
                                 ->data(sourceModel()->index(i, 0), CompletionModel::SearchRole)
                                 .toString()
                                 .toLower();

                for (const auto &e : string1.split(splitPoints)) {
                        if (!e.isEmpty()) // NOTE(Nico): Use Qt::SkipEmptyParts in Qt 5.14
                                trie_.insert(e.toUcs4(), i);
                }

                auto string2 = sourceModel()
                                 ->data(sourceModel()->index(i, 0), CompletionModel::SearchRole2)
                                 .toString()
                                 .toLower();

                if (!string2.isEmpty()) {
                        for (const auto &e : string2.split(splitPoints)) {
                                if (!e.isEmpty()) // NOTE(Nico): Use Qt::SkipEmptyParts in Qt 5.14
                                        trie_.insert(e.toUcs4(), i);
                        }
                }
        }

        connect(
          this,
          &CompletionProxyModel::newSearchString,
          this,
          [this](QString s) {
                  s.remove(":");
                  s.remove("@");
                  searchString_ = s.toLower();
                  invalidate();
          },
          Qt::QueuedConnection);
}

void
CompletionProxyModel::invalidate()
{
        auto key = searchString_.toUcs4();
        beginResetModel();
        if (!key.empty()) // return default model data, if no search string
                mapping = trie_.search(key, max_completions_, maxMistakes_);
        endResetModel();
}

QHash<int, QByteArray>
CompletionProxyModel::roleNames() const
{
        return this->sourceModel()->roleNames();
}

int
CompletionProxyModel::rowCount(const QModelIndex &) const
{
        if (searchString_.isEmpty())
                return std::min(static_cast<int>(std::min<size_t>(max_completions_,
                                                                  std::numeric_limits<int>::max())),
                                sourceModel()->rowCount());
        else
                return (int)mapping.size();
}

QModelIndex
CompletionProxyModel::mapFromSource(const QModelIndex &sourceIndex) const
{
        // return default model data, if no search string
        if (searchString_.isEmpty()) {
                return index(sourceIndex.row(), 0);
        }

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

        // return default model data, if no search string
        if (searchString_.isEmpty()) {
                return index(row, 0);
        }

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
