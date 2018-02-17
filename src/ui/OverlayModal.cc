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

OverlayModal::OverlayModal(QWidget *parent, QWidget *content)
  : OverlayWidget(parent)
  , content_{content}
  , duration_{500}
  , color_{QColor(55, 55, 55)}
{
        auto layout = new QVBoxLayout();
        layout->addWidget(content);
        layout->setAlignment(Qt::AlignCenter);

        setLayout(layout);

        opacity_ = new QGraphicsOpacityEffect(this);
        setGraphicsEffect(opacity_);

        opacity_->setOpacity(1);
        animation_ = new QPropertyAnimation(opacity_, "opacity", this);
        animation_->setStartValue(1);
        animation_->setEndValue(0);
        animation_->setDuration(duration_);
        animation_->setEasingCurve(QEasingCurve::Linear);

        connect(animation_, &QPropertyAnimation::finished, [this]() {
                if (animation_->direction() == QAbstractAnimation::Forward)
                        this->close();
        });

        content->setFocus();
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
                fadeOut();
}

void
OverlayModal::fadeIn()
{
        animation_->setDirection(QAbstractAnimation::Backward);
        animation_->start();
        show();
}

void
OverlayModal::fadeOut()
{
        animation_->setDirection(QAbstractAnimation::Forward);
        animation_->start();
}

void
OverlayModal::keyPressEvent(QKeyEvent *event)
{
        if (event->key() == Qt::Key_Escape) {
                event->accept();
                fadeOut();
        }
}
