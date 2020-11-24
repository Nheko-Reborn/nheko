#pragma once

// Class for showing a limited amount of completions at a time

#include <QAbstractProxyModel>
#include <QRegularExpression>

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
                                for (auto &&v : temp) {
                                        if (ret.size() >= limit)
                                                return ret;

                                        if (std::find(ret.begin(), ret.end(), v) == ret.end()) {
                                                ret.push_back(std::move(v));
                                        }
                                }
                        }
                }

                return ret;
        }

        std::vector<Value> search(const QVector<Key> &keys, //< TODO(Nico): replace this with a span
                                  size_t limit,
                                  size_t max_distance = 2) const
        {
                std::vector<Value> ret;
                if (!limit)
                        return ret;

                if (keys.isEmpty())
                        return valuesAndSubvalues(limit);

                auto append = [&ret, limit](std::vector<Value> &&in) {
                        for (auto &&v : in) {
                                if (ret.size() >= limit)
                                        return;

                                if (std::find(ret.begin(), ret.end(), v) == ret.end()) {
                                        ret.push_back(std::move(v));
                                }
                        }
                };

                if (auto e = this->next.find(keys[0]); e != this->next.end()) {
                        append(e->second.search(keys.mid(1), limit, max_distance));
                }

                if (max_distance && ret.size() < limit) {
                        max_distance -= 1;

                        // swap chars case
                        if (keys.size() >= 2) {
                                auto t = this;
                                for (int i = 1; i >= 0; i--) {
                                        if (auto e = t->next.find(keys[i]); e != t->next.end()) {
                                                t = &e->second;
                                        } else {
                                                t = nullptr;
                                                break;
                                        }
                                }

                                if (t) {
                                        append(t->search(
                                          keys.mid(2), (limit - ret.size()) * 2, max_distance));
                                }
                        }

                        // delete character case
                        append(this->search(keys.mid(1), (limit - ret.size()) * 2, max_distance));

                        // substitute and insert cases
                        for (const auto &[k, t] : this->next) {
                                if (k == keys[0] || ret.size() >= limit)
                                        break;

                                // substitute
                                append(t.search(keys.mid(1), limit - ret.size(), max_distance));

                                if (ret.size() >= limit)
                                        break;

                                // insert
                                append(t.search(keys, limit - ret.size(), max_distance));
                        }
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
                QRegularExpression splitPoints("\\s+|-");

                for (int i = 0; i < sourceModel()->rowCount(); i++) {
                        if (i < 7)
                                mapping.push_back(i);

                        auto string1 =
                          sourceModel()
                            ->data(sourceModel()->index(i, 0), CompletionModel::SearchRole)
                            .toString()
                            .toLower();
                        trie_.insert(string1.toUcs4(), i);

                        for (const auto &e : string1.split(splitPoints, Qt::SkipEmptyParts)) {
                                trie_.insert(e.toUcs4(), i);
                        }

                        auto string2 =
                          sourceModel()
                            ->data(sourceModel()->index(i, 0), CompletionModel::SearchRole2)
                            .toString()
                            .toLower();

                        if (!string2.isEmpty()) {
                                trie_.insert(string2.toUcs4(), i);
                                for (const auto &e :
                                     string2.split(splitPoints, Qt::SkipEmptyParts)) {
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
