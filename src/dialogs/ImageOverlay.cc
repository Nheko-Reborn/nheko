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

#include <QApplication>
#include <QDesktopWidget>
#include <QPainter>

#include "Utils.h"
#include "dialogs/ImageOverlay.h"

using namespace dialogs;

ImageOverlay::ImageOverlay(QPixmap image, QWidget *parent)
  : QWidget{parent}
  , originalImage_{image}
{
        setMouseTracking(true);
        setParent(0);

        setWindowFlags(windowFlags() | Qt::FramelessWindowHint);

        setAttribute(Qt::WA_NoSystemBackground, true);
        setAttribute(Qt::WA_TranslucentBackground, true);
        setAttribute(Qt::WA_DeleteOnClose, true);
        setWindowState(Qt::WindowFullScreen);

        screen_ = QApplication::desktop()->availableGeometry();

        move(QApplication::desktop()->mapToGlobal(screen_.topLeft()));
        resize(screen_.size());

        connect(this, SIGNAL(closing()), this, SLOT(close()));

        raise();
}

void
ImageOverlay::paintEvent(QPaintEvent *event)
{
        Q_UNUSED(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // Full screen overlay.
        painter.fillRect(QRect(0, 0, screen_.width(), screen_.height()), QColor(55, 55, 55, 170));

        // Left and Right margins
        int outer_margin = screen_.width() * 0.12;
        int buttonSize   = 36;
        int margin       = outer_margin * 0.1;

        int max_width  = screen_.width() - 2 * outer_margin;
        int max_height = screen_.height();

        image_ = utils::scaleDown<QPixmap>(max_width, max_height, originalImage_);

        int diff_x = max_width - image_.width();
        int diff_y = max_height - image_.height();

        content_ = QRect(outer_margin + diff_x / 2, diff_y / 2, image_.width(), image_.height());
        close_button_ =
          QRect(screen_.width() - margin - buttonSize, margin, buttonSize, buttonSize);

        // Draw main content_.
        painter.drawPixmap(content_, image_);

        // Draw top right corner X.
        QPen pen;
        pen.setCapStyle(Qt::RoundCap);
        pen.setWidthF(5);
        pen.setColor("gray");

        auto center = close_button_.center();

        painter.setPen(pen);
        painter.drawLine(center - QPointF(15, 15), center + QPointF(15, 15));
        painter.drawLine(center + QPointF(15, -15), center - QPointF(15, -15));
}

void
ImageOverlay::mousePressEvent(QMouseEvent *event)
{
        if (event->button() != Qt::LeftButton)
                return;

        if (close_button_.contains(event->pos()))
                emit closing();
        else if (!content_.contains(event->pos()))
                emit closing();
}
