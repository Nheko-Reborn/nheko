#pragma once

// Class for showing a limited amount of completions at a time

#include <QSortFilterProxyModel>

#include "CompletionModelRoles.h"

class CompletionProxyModel : public QSortFilterProxyModel
{
        Q_OBJECT

public:
        CompletionProxyModel(QAbstractItemModel *model, QObject *parent = nullptr)
          : QSortFilterProxyModel(parent)
        {
                setSourceModel(model);
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

public slots:
        QVariant completionAt(int i) const
        {
                if (i >= 0 && i < rowCount())
                        return data(index(i, 0), CompletionModel::CompletionRole);
                else
                        return {};
        }
};
