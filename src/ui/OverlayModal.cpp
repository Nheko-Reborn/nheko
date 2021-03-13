// SPDX-FileCopyrightText: 2017 Konstantinos Sideris <siderisk@auth.gr>
// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

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
