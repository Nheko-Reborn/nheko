#pragma once

#include <QColor>

class QPainter;

class DropShadow
{
public:
        static void draw(QPainter &painter,
                         qint16 margin,
                         qreal radius,
                         QColor start,
                         QColor end,
                         qreal startPosition,
                         qreal endPosition0,
                         qreal endPosition1,
                         qreal width,
                         qreal height);
};
