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

#ifndef EMOJI_ITEM_DELEGATE_H
#define EMOJI_ITEM_DELEGATE_H

#include <QModelIndex>
#include <QStandardItemModel>
#include <QStyledItemDelegate>

#include "EmojiProvider.h"

class EmojiItemDelegate : public QStyledItemDelegate
{
	Q_OBJECT

public:
	explicit EmojiItemDelegate(QObject *parent = nullptr);
	~EmojiItemDelegate();

	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
	Emoji *data_;
};

#endif  // EMOJI_ITEM_DELEGATE_H
