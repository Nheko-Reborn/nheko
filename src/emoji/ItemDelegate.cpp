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

#include <QPainter>
#include <QSettings>

#include "emoji/ItemDelegate.h"

using namespace emoji;

ItemDelegate::ItemDelegate(QObject *parent)
  : QStyledItemDelegate(parent)
{
        data_ = new Emoji;
}

ItemDelegate::~ItemDelegate() { delete data_; }

void
ItemDelegate::paint(QPainter *painter,
                    const QStyleOptionViewItem &option,
                    const QModelIndex &index) const
{
        Q_UNUSED(index);

        painter->save();

        QStyleOptionViewItem viewOption(option);

        auto emoji = index.data(Qt::UserRole).toString();

        QSettings settings;

        QFont font;
        QString userFontFamily = settings.value("user/emoji_font_family", "emoji").toString();
        if (!userFontFamily.isEmpty()) {
                font.setFamily(userFontFamily);
        } else {
                font.setFamily("emoji");
        }

        font.setPixelSize(36);
        painter->setFont(font);
        if (option.state & QStyle::State_MouseOver) {
                painter->setBackgroundMode(Qt::OpaqueMode);
                QColor hoverColor = parent()->property("hoverBackgroundColor").value<QColor>();
                painter->setBackground(hoverColor);
        }
        painter->drawText(viewOption.rect, Qt::AlignCenter, emoji);

        painter->restore();
}
