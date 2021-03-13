// SPDX-FileCopyrightText: 2021 Nheko Contributors
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
                Emoji,                  // Contains everything from the Emoji
        };

        using QAbstractListModel::QAbstractListModel;

        QHash<int, QByteArray> roleNames() const override;
        int rowCount(const QModelIndex &parent = QModelIndex()) const override;
        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
};

class EmojiProxyModel : public QSortFilterProxyModel
{
        Q_OBJECT

        Q_PROPERTY(
          emoji::Emoji::Category category READ category WRITE setCategory NOTIFY categoryChanged)
        Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)

public:
        explicit EmojiProxyModel(QObject *parent = nullptr);
        ~EmojiProxyModel() override;

        Emoji::Category category() const;
        void setCategory(Emoji::Category cat);

        QString filter() const;
        void setFilter(const QString &filter);

signals:
        void categoryChanged();
        void filterChanged();

protected:
        bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
        Emoji::Category category_ = Emoji::Category::Search;
        emoji::Provider emoji_provider_;
};
}
