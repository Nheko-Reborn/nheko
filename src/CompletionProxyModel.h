#pragma once

// Class for showing a limited amount of completions at a time

#include <QSortFilterProxyModel>

#include "CompletionModelRoles.h"
#include "Utils.h"

class CompletionProxyModel : public QSortFilterProxyModel
{
        Q_OBJECT

public:
        CompletionProxyModel(QAbstractItemModel *model, QObject *parent = nullptr)
          : QSortFilterProxyModel(parent)
        {
                setSourceModel(model);
                sort(0, Qt::AscendingOrder);
                setFilterRole(CompletionModel::SearchRole);

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

        QHash<int, QByteArray> roleNames() const override
        {
                return this->sourceModel()->roleNames();
        }

        int rowCount(const QModelIndex &parent = QModelIndex()) const override
        {
                auto row_count = QSortFilterProxyModel::rowCount(parent);
                return (row_count < 7) ? row_count : 7;
        }

        bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
        {
                if (searchString.size() < 1)
                        return true;

                auto source_index = sourceModel()->index(source_row, 0, source_parent);
                auto role1        = sourceModel()
                               ->data(source_index, CompletionModel::SearchRole)
                               .toString()
                               .toLower();

                if (role1.contains(searchString))
                        return true;
                // auto score =
                //  utils::levenshtein_distance(searchString, role1.toLower().toStdString());
                // if ((size_t)role1.size() >= searchString.size() &&
                //    ((size_t)score) < (size_t)role1.size() - searchString.size() + 2)
                //        return true;

                auto role2 = sourceModel()
                               ->data(source_index, CompletionModel::SearchRole2)
                               .toString()
                               .toLower();
                if (role2.contains(searchString))
                        return true;
                // if (!role2.isEmpty()) {
                //        score =
                //          utils::levenshtein_distance(searchString,
                //          role2.toLower().toStdString());
                //        if ((size_t)role2.size() >= searchString.size() &&
                //            ((size_t)score) < (size_t)role2.size() - searchString.size() + 2)
                //                return true;
                //}

                return false;
        }

        bool lessThan(const QModelIndex &source_left,
                      const QModelIndex &source_right) const override
        {
                if (searchString.size() < 1)
                        return false;

                auto left1 =
                  sourceModel()->data(source_left, CompletionModel::SearchRole).toString();
                auto left2 =
                  sourceModel()->data(source_left, CompletionModel::SearchRole2).toString();
                auto left = left1.toLower().indexOf(searchString);
                // utils::levenshtein_distance(searchString, left1.toLower().toStdString());
                if (!left2.isEmpty()) {
                        // left = std::min(
                        //  utils::levenshtein_distance(searchString,
                        //  left2.toLower().toStdString()), left);
                        left = std::min(left2.toLower().indexOf(searchString), left);
                }

                auto right1 =
                  sourceModel()->data(source_right, CompletionModel::SearchRole).toString();
                auto right2 =
                  sourceModel()->data(source_right, CompletionModel::SearchRole2).toString();
                auto right = right1.toLower().indexOf(searchString);
                // auto right =
                //  utils::levenshtein_distance(searchString, right1.toLower().toStdString());
                if (!right2.isEmpty()) {
                        // right = std::min(
                        //  utils::levenshtein_distance(searchString,
                        //  right2.toLower().toStdString()), right);
                        right = std::min(right2.toLower().indexOf(searchString), right);
                }

                return left < right;
        }

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
};
