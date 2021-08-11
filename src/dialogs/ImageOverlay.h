// SPDX-FileCopyrightText: 2017 Konstantinos Sideris <siderisk@auth.gr>
// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QDialog>
#include <QMouseEvent>
#include <QPixmap>
#include <QShortcut>

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
        void saving();

private:
        QPixmap originalImage_;
        QPixmap image_;

        QRect content_;
        QRect close_button_;
        QRect save_button_;
        QShortcut *close_shortcut_;
};
} // dialogs
