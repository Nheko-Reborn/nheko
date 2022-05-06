// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include <QSet>
#include <QSortFilterProxyModel>
#include <QVector>

#include "Provider.h"

namespace emoji {

/*
 * Provides access to the emojis in Provider.h to QML
 */
class EmojiModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles
    {
        Unicode = Qt::UserRole, // unicode of emoji
        Category,               // category of emoji
        ShortName,              // shortext of the emoji
        UnicodeName,            // true unicode name of the emoji
        Emoji,                  // Contains everything from the Emoji
    };

    using QAbstractListModel::QAbstractListModel;

    Q_INVOKABLE int categoryToIndex(int category);

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
};
}
