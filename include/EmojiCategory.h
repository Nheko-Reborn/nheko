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

#ifndef EMOJI_CATEGORY_H
#define EMOJI_CATEGORY_H

#include <QHBoxLayout>
#include <QLabel>
#include <QListView>
#include <QStandardItemModel>
#include <QVBoxLayout>
#include <QWidget>

#include "EmojiItemDelegate.h"
#include "EmojiProvider.h"

class EmojiCategory : public QWidget
{
	Q_OBJECT

public:
	EmojiCategory(QString category, QList<Emoji> emoji, QWidget *parent = nullptr);
	~EmojiCategory();

signals:
	void emojiSelected(const QString &emoji);

private slots:
	inline void clickIndex(const QModelIndex &);

private:
	QVBoxLayout *mainLayout_;

	QStandardItemModel *itemModel_;
	QListView *emojiListView_;

	Emoji *data_;
	EmojiItemDelegate *delegate_;

	QLabel *category_;
};

inline void EmojiCategory::clickIndex(const QModelIndex &index)
{
	emit emojiSelected(index.data(Qt::UserRole).toString());
}

#endif  // EMOJI_CATEGORY_H
