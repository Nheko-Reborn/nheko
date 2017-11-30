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
#include <QScrollBar>
#include <QStyleOption>

#include "Config.h"

#include "emoji/Category.h"

using namespace emoji;

Category::Category(QString category, QList<Emoji> emoji, QWidget *parent)
  : QWidget(parent)
{
        mainLayout_ = new QVBoxLayout(this);
        mainLayout_->setMargin(0);
        mainLayout_->setSpacing(0);

        emojiListView_ = new QListView();
        itemModel_     = new QStandardItemModel(this);

        delegate_ = new ItemDelegate(this);
        data_     = new Emoji;

        emojiListView_->setItemDelegate(delegate_);
        emojiListView_->setModel(itemModel_);
        emojiListView_->setViewMode(QListView::IconMode);
        emojiListView_->setFlow(QListView::LeftToRight);
        emojiListView_->setResizeMode(QListView::Adjust);
        emojiListView_->verticalScrollBar()->setEnabled(false);
        emojiListView_->horizontalScrollBar()->setEnabled(false);

        const int cols = 7;
        const int rows = emoji.size() / 7;

        // TODO: Be precise here. Take the parent into consideration.
        emojiListView_->setFixedSize(cols * 50 + 20, rows * 50 + 20);
        emojiListView_->setGridSize(QSize(50, 50));
        emojiListView_->setDragEnabled(false);
        emojiListView_->setEditTriggers(QAbstractItemView::NoEditTriggers);

        for (const auto &e : emoji) {
                data_->unicode = e.unicode;

                auto item = new QStandardItem;
                item->setSizeHint(QSize(24, 24));

                QVariant unicode(data_->unicode);
                item->setData(unicode.toString(), Qt::UserRole);

                itemModel_->appendRow(item);
        }

        QFont font("Open Sans SemiBold");
        font.setPixelSize(conf::fontSize);

        category_ = new QLabel(category, this);
        category_->setFont(font);
        category_->setStyleSheet("margin: 20px 0 20px 8px;");

        mainLayout_->addWidget(category_);
        mainLayout_->addWidget(emojiListView_);

        connect(emojiListView_, &QListView::clicked, this, &Category::clickIndex);
}

void
Category::paintEvent(QPaintEvent *)
{
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

Category::~Category() {}
