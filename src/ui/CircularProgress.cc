#include <QPainter>
#include <QParallelAnimationGroup>
#include <QPen>
#include <QPropertyAnimation>

#include "CircularProgress.h"
#include "Theme.h"

CircularProgress::CircularProgress(QWidget *parent)
    : QProgressBar{parent}
    , progress_type_{ui::ProgressType::IndeterminateProgress}
    , width_{6.25}
    , size_{64}
    , duration_{3050}
{
	delegate_ = new CircularProgressDelegate(this);

	setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

	auto group = new QParallelAnimationGroup(this);
	group->setLoopCount(-1);

	auto length_animation = new QPropertyAnimation(this);
	length_animation->setPropertyName("dashLength");
	length_animation->setTargetObject(delegate_);
	length_animation->setEasingCurve(QEasingCurve::InOutQuad);
	length_animation->setStartValue(0.1);
	length_animation->setKeyValueAt(0.15, 3);
	length_animation->setKeyValueAt(0.6, 20);
	length_animation->setKeyValueAt(0.7, 20);
	length_animation->setEndValue(20);
	length_animation->setDuration(duration_);

	auto offset_animation = new QPropertyAnimation(this);
	offset_animation->setPropertyName("dashOffset");
	offset_animation->setTargetObject(delegate_);
	offset_animation->setEasingCurve(QEasingCurve::InOutSine);
	offset_animation->setStartValue(0);
	offset_animation->setKeyValueAt(0.15, 0);
	offset_animation->setKeyValueAt(0.6, -7);
	offset_animation->setKeyValueAt(0.7, -7);
	offset_animation->setEndValue(-25);
	offset_animation->setDuration(duration_);

	auto angle_animation = new QPropertyAnimation(this);
	angle_animation->setPropertyName("angle");
	angle_animation->setTargetObject(delegate_);
	angle_animation->setStartValue(0);
	angle_animation->setEndValue(360);
	angle_animation->setDuration(duration_);

	group->addAnimation(length_animation);
	group->addAnimation(offset_animation);
	group->addAnimation(angle_animation);

	group->start();
}

void CircularProgress::setProgressType(ui::ProgressType type)
{
	progress_type_ = type;
	update();
}

void CircularProgress::setLineWidth(qreal width)
{
	width_ = width;
	update();
	updateGeometry();
}

void CircularProgress::setSize(int size)
{
	size_ = size;
	update();
	updateGeometry();
}

ui::ProgressType CircularProgress::progressType() const
{
	return progress_type_;
}

qreal CircularProgress::lineWidth() const
{
	return width_;
}

int CircularProgress::size() const
{
	return size_;
}

void CircularProgress::setColor(const QColor &color)
{
	color_ = color;
}

QColor CircularProgress::color() const
{
	if (!color_.isValid()) {
		return QColor("red");
	}

	return color_;
}

QSize CircularProgress::sizeHint() const
{
	const qreal s = size_ + width_ + 8;
	return QSize(s, s);
}

void CircularProgress::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event);

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	/*
	 * If the progress bar is disabled draw an X instead
	 */
	if (!isEnabled()) {
		QPen pen;
		pen.setCapStyle(Qt::RoundCap);
		pen.setWidthF(lineWidth());
		pen.setColor("gray");

		auto center = rect().center();

		painter.setPen(pen);
		painter.drawLine(center - QPointF(20, 20), center + QPointF(20, 20));
		painter.drawLine(center + QPointF(20, -20), center - QPointF(20, -20));

		return;
	}

	if (progress_type_ == ui::ProgressType::IndeterminateProgress) {
		painter.translate(width() / 2, height() / 2);
		painter.rotate(delegate_->angle());
	}

	QPen pen;
	pen.setCapStyle(Qt::RoundCap);
	pen.setWidthF(width_);
	pen.setColor(color());

	if (ui::ProgressType::IndeterminateProgress == progress_type_) {
		QVector<qreal> pattern;
		pattern << delegate_->dashLength() * size_ / 50 << 30 * size_ / 50;

		pen.setDashOffset(delegate_->dashOffset() * size_ / 50);
		pen.setDashPattern(pattern);

		painter.setPen(pen);

		painter.drawEllipse(QPoint(0, 0), size_ / 2, size_ / 2);
	} else {
		painter.setPen(pen);

		const qreal x = (width() - size_) / 2;
		const qreal y = (height() - size_) / 2;

		const qreal a = 360 * (value() - minimum()) / (maximum() - minimum());

		QPainterPath path;
		path.arcMoveTo(x, y, size_, size_, 0);
		path.arcTo(x, y, size_, size_, 0, a);

		painter.drawPath(path);
	}
}

CircularProgress::~CircularProgress()
{
}

CircularProgressDelegate::CircularProgressDelegate(CircularProgress *parent)
    : QObject(parent)
    , progress_(parent)
    , dash_offset_(0)
    , dash_length_(89)
    , angle_(0)
{
	Q_ASSERT(parent);
}

CircularProgressDelegate::~CircularProgressDelegate()
{
}
