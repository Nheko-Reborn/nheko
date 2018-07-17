#pragma once

#include <QColor>
#include <QIcon>
#include <QWidget>
#include <QtGlobal>

#include "OverlayWidget.h"

class Badge : public OverlayWidget
{
        Q_OBJECT

        Q_PROPERTY(QColor textColor WRITE setTextColor READ textColor)
        Q_PROPERTY(QColor backgroundColor WRITE setBackgroundColor READ backgroundColor)
        Q_PROPERTY(QPointF relativePosition WRITE setRelativePosition READ relativePosition)

public:
        explicit Badge(QWidget *parent = 0);
        explicit Badge(const QIcon &icon, QWidget *parent = 0);
        explicit Badge(const QString &text, QWidget *parent = 0);

        void setBackgroundColor(const QColor &color);
        void setTextColor(const QColor &color);
        void setIcon(const QIcon &icon);
        void setRelativePosition(const QPointF &pos);
        void setRelativePosition(qreal x, qreal y);
        void setRelativeXPosition(qreal x);
        void setRelativeYPosition(qreal y);
        void setText(const QString &text);
        void setDiameter(int diameter);

        QIcon icon() const;
        QString text() const;
        QColor backgroundColor() const;
        QColor textColor() const;
        QPointF relativePosition() const;
        QSize sizeHint() const override;
        qreal relativeXPosition() const;
        qreal relativeYPosition() const;

        int diameter() const;

protected:
        void paintEvent(QPaintEvent *event) override;

private:
        void init();

        QColor background_color_;
        QColor text_color_;

        QIcon icon_;
        QSize size_;
        QString text_;

        int padding_;
        int diameter_;

        qreal x_;
        qreal y_;
};
