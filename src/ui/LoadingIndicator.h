// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QColor>
#include <QWidget>

class QPainter;
class QTimer;
class QPaintEvent;
class LoadingIndicator : public QWidget
{
        Q_OBJECT
        Q_PROPERTY(QColor color READ color WRITE setColor)

public:
        LoadingIndicator(QWidget *parent = nullptr);

        void paintEvent(QPaintEvent *e) override;

        void start();
        void stop();

        QColor color() { return color_; }
        void setColor(QColor color) { color_ = color; }

        int interval() { return interval_; }
        void setInterval(int interval) { interval_ = interval; }

private slots:
        void onTimeout();

private:
        int interval_;
        int angle_;

        QColor color_;
        QTimer *timer_;
};
