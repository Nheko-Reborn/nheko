#include <QPainter>

#include "Badge.h"

Badge::Badge(QWidget *parent)
    : OverlayWidget(parent)
{
	init();
}

Badge::Badge(const QIcon &icon, QWidget *parent)
    : OverlayWidget(parent)
{
	init();
	setIcon(icon);
}

Badge::Badge(const QString &text, QWidget *parent)
    : OverlayWidget(parent)
{
	init();
	setText(text);
}

Badge::~Badge()
{
}

void Badge::init()
{
	x_ = 0;
	y_ = 0;
	padding_ = 10;

	setAttribute(Qt::WA_TransparentForMouseEvents);

	QFont _font(font());
	_font.setPointSizeF(10);
	_font.setStyleName("Bold");

	setFont(_font);
	setText("");
}

QString Badge::text() const
{
	return text_;
}

QIcon Badge::icon() const
{
	return icon_;
}

QSize Badge::sizeHint() const
{
	const int d = getDiameter();
	return QSize(d + 4, d + 4);
}

qreal Badge::relativeYPosition() const
{
	return y_;
}

qreal Badge::relativeXPosition() const
{
	return x_;
}

QPointF Badge::relativePosition() const
{
	return QPointF(x_, y_);
}

QColor Badge::backgroundColor() const
{
	if (!background_color_.isValid())
		return QColor("black");

	return background_color_;
}

QColor Badge::textColor() const
{
	if (!text_color_.isValid())
		return QColor("white");

	return text_color_;
}

void Badge::setTextColor(const QColor &color)
{
	text_color_ = color;
}

void Badge::setBackgroundColor(const QColor &color)
{
	background_color_ = color;
}

void Badge::setRelativePosition(const QPointF &pos)
{
	setRelativePosition(pos.x(), pos.y());
}

void Badge::setRelativePosition(qreal x, qreal y)
{
	x_ = x;
	y_ = y;
	update();
}

void Badge::setRelativeXPosition(qreal x)
{
	x_ = x;
	update();
}

void Badge::setRelativeYPosition(qreal y)
{
	y_ = y;
	update();
}

void Badge::setIcon(const QIcon &icon)
{
	icon_ = icon;
	update();
}

void Badge::setText(const QString &text)
{
	text_ = text;

	if (!icon_.isNull())
		icon_ = QIcon();

	size_ = fontMetrics().size(Qt::TextShowMnemonic, text);

	update();
}

void Badge::paintEvent(QPaintEvent *)
{
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.translate(x_, y_);

	QBrush brush;
	brush.setStyle(Qt::SolidPattern);
	brush.setColor(isEnabled() ? backgroundColor() : QColor("#cccccc"));

	painter.setBrush(brush);
	painter.setPen(Qt::NoPen);

	const int d = getDiameter();

	QRectF r(0, 0, d, d);
	r.translate(QPointF((width() - d), (height() - d)) / 2);

	if (icon_.isNull()) {
		painter.drawEllipse(r);
		painter.setPen(textColor());
		painter.setBrush(Qt::NoBrush);
		painter.drawText(r.translated(0, -0.5), Qt::AlignCenter, text_);
	} else {
		painter.drawEllipse(r);
		QRectF q(0, 0, 16, 16);
		q.moveCenter(r.center());
		QPixmap pixmap = icon().pixmap(16, 16);
		QPainter icon(&pixmap);
		icon.setCompositionMode(QPainter::CompositionMode_SourceIn);
		icon.fillRect(pixmap.rect(), textColor());
		painter.drawPixmap(q.toRect(), pixmap);
	}
}

int Badge::getDiameter() const
{
	if (icon_.isNull()) {
		return qMax(size_.width(), size_.height()) + padding_;
	}
	// FIXME: Move this to Theme.h as the default
	return 24;
}
