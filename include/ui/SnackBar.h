#pragma once

#include <QCoreApplication>
#include <QPaintEvent>
#include <QSharedPointer>
#include <QTimer>

#include "OverlayWidget.h"

enum class SnackBarPosition
{
        Bottom,
        Top,
};

class SnackBar : public OverlayWidget
{
        Q_OBJECT

public:
        explicit SnackBar(QWidget *parent);

        inline void setBackgroundColor(const QColor &color);
        inline void setTextColor(const QColor &color);
        inline void setPosition(SnackBarPosition pos);

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

        QList<QString> messages_;

        QSharedPointer<QTimer> showTimer_;
        QSharedPointer<QTimer> hideTimer_;

        int duration_;
        int boxWidth_;
        int boxHeight_;
        int boxPadding_;

        SnackBarPosition position_;
};

inline void
SnackBar::setPosition(SnackBarPosition pos)
{
        position_ = pos;
        update();
}

inline void
SnackBar::setBackgroundColor(const QColor &color)
{
        bgColor_ = color;
        update();
}

inline void
SnackBar::setTextColor(const QColor &color)
{
        textColor_ = color;
        update();
}
