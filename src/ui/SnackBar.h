// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QCoreApplication>
#include <QPaintEvent>
#include <QPropertyAnimation>
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

    Q_PROPERTY(
      QColor bgColor READ backgroundColor WRITE setBackgroundColor NOTIFY backgroundColorChanged)
    Q_PROPERTY(QColor textColor READ textColor WRITE setTextColor NOTIFY textColorChanged)
    Q_PROPERTY(double offset READ offset WRITE setOffset NOTIFY offsetChanged)

public:
    explicit SnackBar(QWidget *parent);

    QColor backgroundColor() const { return bgColor_; }
    void setBackgroundColor(const QColor &color)
    {
        bgColor_ = color;
        update();
        emit backgroundColorChanged();
    }

    QColor textColor() const { return textColor_; }
    void setTextColor(const QColor &color)
    {
        textColor_ = color;
        update();
        emit textColorChanged();
    }
    void setPosition(SnackBarPosition pos)
    {
        position_ = pos;
        update();
    }

    double offset() { return offset_; }
    void setOffset(double offset)
    {
        if (offset != offset_) {
            offset_ = offset;
            emit offsetChanged();
        }
    }

public slots:
    void showMessage(const QString &msg);

signals:
    void offsetChanged();
    void backgroundColorChanged();
    void textColorChanged();

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

    QTimer hideTimer_;

    double boxHeight_;

    QPropertyAnimation offset_anim;

    SnackBarPosition position_;
};
