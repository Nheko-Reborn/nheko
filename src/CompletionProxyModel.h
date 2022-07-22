// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Class for showing a limited amount of completions at a time

#include <QAbstractProxyModel>

enum class ElementRank
{
    first,
    second
};

template<typename Key, typename Value>
struct trie
{
    std::vector<Value> values;
    std::map<Key, trie> next;

    template<ElementRank r>
    void insert(const QVector<Key> &keys, const Value &v)
    {
        auto t = this;
        for (const auto k : keys) {
            t = &t->next[k];
        }

        if constexpr (r == ElementRank::first) {
            t->values.insert(t->values.begin(), v);
        } else if constexpr (r == ElementRank::second) {
            t->values.push_back(v);
        }
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
                              size_t result_count_limit,
                              size_t max_edit_distance_ = 2) const
    {
        std::vector<Value> ret;
        if (!result_count_limit)
            return ret;

        if (keys.isEmpty())
            return valuesAndSubvalues(result_count_limit);

        auto append = [&ret, result_count_limit](std::vector<Value> &&in) {
            for (auto &&v : in) {
                if (ret.size() >= result_count_limit)
                    return;

                if (std::find(ret.begin(), ret.end(), v) == ret.end()) {
                    ret.push_back(std::move(v));
                }
            }
        };

        auto limit = [&ret, result_count_limit] {
            return std::min(result_count_limit, (result_count_limit - ret.size()) * 2);
        };

        // Try first exact matches, then with maximum errors
        for (size_t max_edit_distance = 0;
             max_edit_distance <= max_edit_distance_ && ret.size() < result_count_limit;
             max_edit_distance += 1) {
            if (max_edit_distance && ret.size() < result_count_limit) {
                max_edit_distance -= 1;

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
                        append(t->search(keys.mid(2), limit(), max_edit_distance));
                    }
                }

                // insert case
                for (const auto &[k, t] : this->next) {
                    if (k == keys[0])
                        continue;
                    if (ret.size() >= limit())
                        break;

                    // insert
                    append(t.search(keys, limit(), max_edit_distance));
                }

                // delete character case
                append(this->search(keys.mid(1), limit(), max_edit_distance));

                // substitute case
                for (const auto &[k, t] : this->next) {
                    if (k == keys[0])
                        continue;
                    if (ret.size() >= limit())
                        break;

                    // substitute
                    append(t.search(keys.mid(1), limit(), max_edit_distance));
                }

                max_edit_distance += 1;
            }

            if (auto e = this->next.find(keys[0]); e != this->next.end()) {
                append(e->second.search(keys.mid(1), limit(), max_edit_distance));
            }
        }

        return ret;
    }
};

class CompletionProxyModel : public QAbstractProxyModel
{
    Q_OBJECT
    Q_PROPERTY(QString searchString READ searchString WRITE setSearchString NOTIFY newSearchString)
public:
    CompletionProxyModel(QAbstractItemModel *model,
                         int max_mistakes       = 2,
                         size_t max_completions = 30,
                         QObject *parent        = nullptr);

    void invalidate();

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &) const override;

    QModelIndex mapFromSource(const QModelIndex &sourceIndex) const override;
    QModelIndex mapToSource(const QModelIndex &proxyIndex) const override;

    QModelIndex
    index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &) const override;

public slots:
    QVariant completionAt(int i) const;

    void setSearchString(QString s);
    QString searchString() const { return searchString_; }

signals:
    void newSearchString(QString);

private:
    QString searchString_;
    trie<uint, int> trie_;
    std::vector<int> mapping;
    int maxMistakes_;
    size_t max_completions_;
};
