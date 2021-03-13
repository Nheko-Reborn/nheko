// SPDX-FileCopyrightText: 2017 Konstantinos Sideris <siderisk@auth.gr>
// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QVBoxLayout>

#include "OverlayWidget.h"

class OverlayModal : public OverlayWidget
{
public:
        OverlayModal(QWidget *parent);

        void setColor(QColor color) { color_ = color; }
        void setDismissible(bool state) { isDismissible_ = state; }

        void setContentAlignment(QFlags<Qt::AlignmentFlag> flag) { layout_->setAlignment(flag); }
        void setWidget(QWidget *widget);

protected:
        void paintEvent(QPaintEvent *event) override;
        void keyPressEvent(QKeyEvent *event) override;
        void mousePressEvent(QMouseEvent *event) override;

private:
        QWidget *content_;
        QVBoxLayout *layout_;

        QColor color_;

        //! Decides whether or not the modal can be removed by clicking into it.
        bool isDismissible_ = true;
};
