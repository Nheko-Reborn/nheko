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

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPaintEvent>

#include "OverlayWidget.h"

class OverlayModal : public OverlayWidget
{
public:
        OverlayModal(QWidget *parent, QWidget *content);

        void setColor(QColor color) { color_ = color; }
        void setDismissible(bool state) { isDismissible_ = state; }

protected:
        void paintEvent(QPaintEvent *event) override;
        void keyPressEvent(QKeyEvent *event) override;
        void mousePressEvent(QMouseEvent *event) override;

private:
        QWidget *content_;
        QColor color_;

        //! Decides whether or not the modal can be removed by clicking into it.
        bool isDismissible_ = true;
};
