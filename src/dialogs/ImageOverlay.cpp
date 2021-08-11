// SPDX-FileCopyrightText: 2017 Konstantinos Sideris <siderisk@auth.gr>
// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QApplication>
#include <QDesktopWidget>
#include <QGuiApplication>
#include <QPainter>
#include <QScreen>

#include "dialogs/ImageOverlay.h"

#include "Utils.h"

using namespace dialogs;

ImageOverlay::ImageOverlay(QPixmap image, QWidget *parent)
  : QWidget{parent}
  , originalImage_{image}
{
        setMouseTracking(true);
        setParent(nullptr);

        setWindowFlags(windowFlags() | Qt::FramelessWindowHint);

        setAttribute(Qt::WA_NoSystemBackground, true);
        setAttribute(Qt::WA_TranslucentBackground, true);
        setAttribute(Qt::WA_DeleteOnClose, true);
        setWindowState(Qt::WindowFullScreen);
        close_shortcut_ = new QShortcut(QKeySequence(Qt::Key_Escape), this);

        connect(close_shortcut_, &QShortcut::activated, this, &ImageOverlay::closing);
        connect(this, &ImageOverlay::closing, this, &ImageOverlay::close);

        raise();
}

void
ImageOverlay::paintEvent(QPaintEvent *event)
{
        Q_UNUSED(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // Full screen overlay.
        painter.fillRect(QRect(0, 0, width(), height()), QColor(55, 55, 55, 170));

        // Left and Right margins
        int outer_margin = width() * 0.12;
        int buttonSize   = 36;
        int margin       = outer_margin * 0.1;

        int max_width  = width() - 2 * outer_margin;
        int max_height = height();

        image_ = utils::scaleDown(max_width, max_height, originalImage_);

        int diff_x = max_width - image_.width();
        int diff_y = max_height - image_.height();

        content_ = QRect(outer_margin + diff_x / 2, diff_y / 2, image_.width(), image_.height());
        close_button_ = QRect(width() - margin - buttonSize, margin, buttonSize, buttonSize);
        save_button_ =
          QRect(width() - (2 * margin) - (2 * buttonSize), margin, buttonSize, buttonSize);

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

        // Draw download button
        center = save_button_.center();
        painter.drawLine(center - QPointF(0, 15), center + QPointF(0, 15));
        painter.drawLine(center - QPointF(15, 0), center + QPointF(0, 15));
        painter.drawLine(center + QPointF(0, 15), center + QPointF(15, 0));
}

void
ImageOverlay::mousePressEvent(QMouseEvent *event)
{
        if (event->button() != Qt::LeftButton)
                return;

        if (close_button_.contains(event->pos()))
                emit closing();
        else if (save_button_.contains(event->pos()))
                emit saving();
        else if (!content_.contains(event->pos()))
                emit closing();
}
