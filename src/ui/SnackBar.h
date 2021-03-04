// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QCoreApplication>
#include <QPaintEvent>
#include <QTimer>
#include <deque>

#include "OverlayWidget.h"

enum class SnackBarPosition
{
        Bottom,
        Top,
};

class SnackBar : public OverlayWidget
{
        Q_OBJECT

        Q_PROPERTY(QColor bgColor READ backgroundColor WRITE setBackgroundColor)
        Q_PROPERTY(QColor textColor READ textColor WRITE setTextColor)

public:
        explicit SnackBar(QWidget *parent);

        QColor backgroundColor() const { return bgColor_; }
        void setBackgroundColor(const QColor &color)
        {
                bgColor_ = color;
                update();
        }

        QColor textColor() const { return textColor_; }
        void setTextColor(const QColor &color)
        {
                textColor_ = color;
                update();
        }
        void setPosition(SnackBarPosition pos)
        {
                position_ = pos;
                update();
        }

public slots:
        void showMessage(const QString &msg);

protected:
        void paintEvent(QPaintEvent *event) override;
        void mousePressEvent(QMouseEvent *event) override;

private slots:
        void hideMessage();

private:
        void stopTimers();
        void start();

        QColor bgColor_;
        QColor textColor_;

        qreal bgOpacity_;
        qreal offset_;

        std::deque<QString> messages_;

        QTimer showTimer_;
        QTimer hideTimer_;

        double boxHeight_;

        SnackBarPosition position_;
};
