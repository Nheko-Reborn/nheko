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
        LoadingIndicator(QWidget *parent = 0);

        void paintEvent(QPaintEvent *e);

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
