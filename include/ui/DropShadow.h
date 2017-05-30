#pragma once

#include <QColor>
#include <QLinearGradient>
#include <QPainter>

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
			 qreal height)
	{
		painter.setPen(Qt::NoPen);

		QLinearGradient gradient;
		gradient.setColorAt(startPosition, start);
		gradient.setColorAt(endPosition0, end);

		// Right
		QPointF right0(width - margin, height / 2);
		QPointF right1(width, height / 2);
		gradient.setStart(right0);
		gradient.setFinalStop(right1);
		painter.setBrush(QBrush(gradient));
		painter.drawRoundRect(QRectF(QPointF(width - margin * radius, margin), QPointF(width, height - margin)), 0.0, 0.0);

		// Left
		QPointF left0(margin, height / 2);
		QPointF left1(0, height / 2);
		gradient.setStart(left0);
		gradient.setFinalStop(left1);
		painter.setBrush(QBrush(gradient));
		painter.drawRoundRect(QRectF(QPointF(margin * radius, margin), QPointF(0, height - margin)), 0.0, 0.0);

		// Top
		QPointF top0(width / 2, margin);
		QPointF top1(width / 2, 0);
		gradient.setStart(top0);
		gradient.setFinalStop(top1);
		painter.setBrush(QBrush(gradient));
		painter.drawRoundRect(QRectF(QPointF(width - margin, 0), QPointF(margin, margin)), 0.0, 0.0);

		// Bottom
		QPointF bottom0(width / 2, height - margin);
		QPointF bottom1(width / 2, height);
		gradient.setStart(bottom0);
		gradient.setFinalStop(bottom1);
		painter.setBrush(QBrush(gradient));
		painter.drawRoundRect(QRectF(QPointF(margin, height - margin), QPointF(width - margin, height)), 0.0, 0.0);

		// BottomRight
		QPointF bottomright0(width - margin, height - margin);
		QPointF bottomright1(width, height);
		gradient.setStart(bottomright0);
		gradient.setFinalStop(bottomright1);
		gradient.setColorAt(endPosition1, end);
		painter.setBrush(QBrush(gradient));
		painter.drawRoundRect(QRectF(bottomright0, bottomright1), 0.0, 0.0);

		// BottomLeft
		QPointF bottomleft0(margin, height - margin);
		QPointF bottomleft1(0, height);
		gradient.setStart(bottomleft0);
		gradient.setFinalStop(bottomleft1);
		gradient.setColorAt(endPosition1, end);
		painter.setBrush(QBrush(gradient));
		painter.drawRoundRect(QRectF(bottomleft0, bottomleft1), 0.0, 0.0);

		// TopLeft
		QPointF topleft0(margin, margin);
		QPointF topleft1(0, 0);
		gradient.setStart(topleft0);
		gradient.setFinalStop(topleft1);
		gradient.setColorAt(endPosition1, end);
		painter.setBrush(QBrush(gradient));
		painter.drawRoundRect(QRectF(topleft0, topleft1), 0.0, 0.0);

		// TopRight
		QPointF topright0(width - margin, margin);
		QPointF topright1(width, 0);
		gradient.setStart(topright0);
		gradient.setFinalStop(topright1);
		gradient.setColorAt(endPosition1, end);
		painter.setBrush(QBrush(gradient));
		painter.drawRoundRect(QRectF(topright0, topright1), 0.0, 0.0);

		// Widget
		painter.setBrush(QBrush("#FFFFFF"));
		painter.setRenderHint(QPainter::Antialiasing);
		painter.drawRoundRect(QRectF(QPointF(margin, margin), QPointF(width - margin, height - margin)), radius, radius);
	}
};
