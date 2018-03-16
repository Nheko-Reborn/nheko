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

#include <QPainter>
#include <QScrollArea>
#include <QScrollBar>

class ScrollBar : public QScrollBar
{
        Q_OBJECT
        Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor)
        Q_PROPERTY(QColor handleColor READ handleColor WRITE setHandleColor)

public:
        ScrollBar(QScrollArea *area, QWidget *parent = nullptr);

        QColor backgroundColor() const { return bgColor_; }
        void setBackgroundColor(QColor &color) { bgColor_ = color; }

        QColor handleColor() const { return handleColor_; }
        void setHandleColor(QColor &color) { handleColor_ = color; }

protected:
        void paintEvent(QPaintEvent *e) override;

private:
        int roundRadius_     = 4;
        int handleWidth_     = 7;
        int minHandleHeight_ = 20;

        const int Padding = 4;

        QScrollArea *area_;
        QRect handle_;

        QColor bgColor_     = QColor(33, 33, 33, 30);
        QColor handleColor_ = QColor(0, 0, 0, 80);
};
