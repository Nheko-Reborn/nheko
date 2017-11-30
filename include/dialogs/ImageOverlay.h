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

#include <QDialog>
#include <QMouseEvent>
#include <QPixmap>

namespace dialogs {

class ImageOverlay : public QWidget
{
        Q_OBJECT
public:
        ImageOverlay(QPixmap image, QWidget *parent = nullptr);

protected:
        void mousePressEvent(QMouseEvent *event) override;
        void paintEvent(QPaintEvent *event) override;

signals:
        void closing();

private:
        void scaleImage(int width, int height);

        QPixmap originalImage_;
        QPixmap image_;

        QRect content_;
        QRect close_button_;
        QRect screen_;
};
} // dialogs
