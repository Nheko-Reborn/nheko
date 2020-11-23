#pragma once

// Class for showing a limited amount of completions at a time

#include <QAbstractProxyModel>

#include "CompletionModelRoles.h"
#include "Logging.h"
#include "Utils.h"

template<typename Key, typename Value>
struct trie
{
        std::vector<Value> values;
        std::map<Key, trie> next;

        void insert(const QVector<Key> &keys, const Value &v)
        {
                auto t = this;
                for (const auto k : keys) {
                        t = &t->next[k];
                }

                t->values.push_back(v);
        }

        std::vector<Value> valuesAndSubvalues(size_t limit = -1) const
        {
                std::vector<Value> ret;
                if (limit < 200)
                        ret.reserve(limit);

                for (const auto &v : values) {
                        if (ret.size() >= limit)
                                return ret;
                        else
                                ret.push_back(v);
                }

                for (const auto &[k, t] : next) {
                        (void)k;
                        if (ret.size() >= limit)
                                return ret;
                        else {
                                auto temp = t.valuesAndSubvalues(limit - ret.size());
                                ret.insert(ret.end(), temp.begin(), temp.end());
                        }
                }

                return ret;
        }

        std::vector<Value> search(const QVector<Key> &keys, size_t limit) const
        {
                std::vector<Value> ret;
                auto t = this;
                int i  = 0;
                for (; i < (int)keys.size(); i++) {
                        if (auto e = t->next.find(keys[i]); e != t->next.end()) {
                                t = &e->second;
                        } else {
                                t = nullptr;
                                break;
                        }
                }

                if (t) {
                        ret = t->valuesAndSubvalues(limit);
                }

                return ret;
        }
};

class CompletionProxyModel : public QAbstractProxyModel
{
        Q_OBJECT

public:
        CompletionProxyModel(QAbstractItemModel *model, QObject *parent = nullptr)
          : QAbstractProxyModel(parent)
        {
                setSourceModel(model);

                for (int i = 0; i < sourceModel()->rowCount(); i++) {
                        if (i < 7)
                                mapping.push_back(i);

                        auto string1 =
                          sourceModel()
                            ->data(sourceModel()->index(i, 0), CompletionModel::SearchRole)
                            .toString()
                            .toLower();
                        trie_.insert(string1.toUcs4(), i);

                        auto string2 =
                          sourceModel()
                            ->data(sourceModel()->index(i, 0), CompletionModel::SearchRole2)
                            .toString()
                            .toLower();

                        if (!string2.isEmpty())
                                trie_.insert(string2.toUcs4(), i);
                }

                connect(
                  this,
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

        void invalidate()
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
        };

        QHash<int, QByteArray> roleNames() const override
        {
                return this->sourceModel()->roleNames();
        }

        int rowCount(const QModelIndex &parent = QModelIndex()) const override
        {
                (void)parent;
                return (int)mapping.size();
        }

        QModelIndex mapFromSource(const QModelIndex &sourceIndex) const override
        {
                for (int i = 0; i < (int)mapping.size(); i++) {
                        if (mapping[i] == sourceIndex.row()) {
                                return index(i, 0);
                        }
                }
                return QModelIndex();
        }

        QModelIndex mapToSource(const QModelIndex &proxyIndex) const override
        {
                auto row = proxyIndex.row();
                if (row < 0 || row >= (int)mapping.size())
                        return QModelIndex();

                return sourceModel()->index(mapping[row], 0);
        }

        QModelIndex index(int row,
                          int column,
                          const QModelIndex &parent = QModelIndex()) const override
        {
                (void)parent;
                return createIndex(row, column);
        }

        QModelIndex parent(const QModelIndex &) const override { return QModelIndex{}; }
        int columnCount(const QModelIndex &) const override { return sourceModel()->columnCount(); }

public slots:
        QVariant completionAt(int i) const
        {
                if (i >= 0 && i < rowCount())
                        return data(index(i, 0), CompletionModel::CompletionRole);
                else
                        return {};
        }

        void setSearchString(QString s) { emit newSearchString(s); }

signals:
        void newSearchString(QString);

private:
        QString searchString;
        trie<uint, int> trie_;
        std::vector<int> mapping;
};
