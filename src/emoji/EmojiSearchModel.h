#pragma once

#include "EmojiModel.h"

#include <CompletionModelRoles.h>
#include <QDebug>
#include <QEvent>
#include <QSortFilterProxyModel>

namespace emoji {

// Map emoji data to searchable data
class EmojiSearchModel : public QSortFilterProxyModel
{
public:
        EmojiSearchModel(QObject *parent = nullptr)
          : QSortFilterProxyModel(parent)
        {
                setSourceModel(new EmojiModel(this));
        }
        QVariant data(const QModelIndex &index, int role = Qt::UserRole + 1) const override
        {
                switch (role) {
                case Qt::DisplayRole: {
                        auto emoji = QSortFilterProxyModel::data(index, role).toString();
                        return emoji + " :" +
                               toShortcode(data(index, EmojiModel::ShortName).toString()) + ":";
                }
                case CompletionModel::CompletionRole:
                        return QSortFilterProxyModel::data(index, EmojiModel::Unicode);
                case CompletionModel::SearchRole: {
                        return toShortcode(
                          QSortFilterProxyModel::data(index, EmojiModel::ShortName).toString());
                }
                default:
                        return QSortFilterProxyModel::data(index, role);
                }
        }

private:
        QString toShortcode(QString shortname) const
        {
                return shortname.replace(" ", "-").replace(":", "-").replace("--", "-").toLower();
        }
};
}
