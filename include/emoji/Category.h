/*
 * nheko Copyright (C) 2017  Konstantinos Sideris <siderisk@auth.gr>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <QLabel>
#include <QLayout>
#include <QListView>
#include <QStandardItemModel>

#include "ItemDelegate.h"

namespace emoji {

class Category : public QWidget
{
        Q_OBJECT

public:
        Category(QString category, std::vector<Emoji> emoji, QWidget *parent = nullptr);
        ~Category();

signals:
        void emojiSelected(const QString &emoji);

protected:
        void paintEvent(QPaintEvent *event) override;

private slots:
        void clickIndex(const QModelIndex &index)
        {
                emit emojiSelected(index.data(Qt::UserRole).toString());
        };

private:
        QVBoxLayout *mainLayout_;

        QStandardItemModel *itemModel_;
        QListView *emojiListView_;

        emoji::Emoji *data_;
        emoji::ItemDelegate *delegate_;

        QLabel *category_;
};
} // namespace emoji
