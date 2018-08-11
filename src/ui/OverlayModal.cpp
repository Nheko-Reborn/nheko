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
#include <QVBoxLayout>

#include "OverlayModal.h"

OverlayModal::OverlayModal(QWidget *parent)
  : OverlayWidget(parent)
  , color_{QColor(30, 30, 30, 170)}
{
        layout_ = new QVBoxLayout(this);
        layout_->setSpacing(0);
        layout_->setContentsMargins(10, 40, 10, 20);
        setContentAlignment(Qt::AlignCenter);
}

void
OverlayModal::setWidget(QWidget *widget)
{
        // Delete the previous widget
        if (layout_->count() > 0) {
                QLayoutItem *item;
                while ((item = layout_->takeAt(0)) != nullptr) {
                        delete item->widget();
                        delete item;
                }
        }

        layout_->addWidget(widget);
        content_ = widget;
        content_->setFocus();
}

void
OverlayModal::paintEvent(QPaintEvent *event)
{
        Q_UNUSED(event);

        QPainter painter(this);
        painter.fillRect(rect(), color_);
}

void
OverlayModal::mousePressEvent(QMouseEvent *e)
{
        if (isDismissible_ && content_ && !content_->geometry().contains(e->pos()))
                hide();
}

void
OverlayModal::keyPressEvent(QKeyEvent *event)
{
        if (event->key() == Qt::Key_Escape) {
                event->accept();
                hide();
        }
}
