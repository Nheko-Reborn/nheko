#pragma once

#include <QObject>
#include <QProgressBar>

#include "Theme.h"

class CircularProgressDelegate;

class CircularProgress : public QProgressBar
{
	Q_OBJECT

	Q_PROPERTY(qreal lineWidth WRITE setLineWidth READ lineWidth)
	Q_PROPERTY(qreal size WRITE setSize READ size)
	Q_PROPERTY(QColor color WRITE setColor READ color)

public:
	explicit CircularProgress(QWidget *parent = nullptr);
	~CircularProgress();

	void setProgressType(ui::ProgressType type);
	void setLineWidth(qreal width);
	void setSize(int size);
	void setColor(const QColor &color);

	ui::ProgressType progressType() const;
	qreal lineWidth() const;
	int size() const;
	QColor color() const;

	QSize sizeHint() const override;

protected:
	void paintEvent(QPaintEvent *event) override;

private:
	CircularProgressDelegate *delegate_;

	ui::ProgressType progress_type_;

	QColor color_;

	// Circle width.
	qreal width_;

	// Circle radius.
	int size_;

	// Animation duration.
	int duration_;
};

class CircularProgressDelegate : public QObject
{
	Q_OBJECT

	Q_PROPERTY(qreal dashOffset WRITE setDashOffset READ dashOffset)
	Q_PROPERTY(qreal dashLength WRITE setDashLength READ dashLength)
	Q_PROPERTY(int angle WRITE setAngle READ angle)

public:
	explicit CircularProgressDelegate(CircularProgress *parent);
	~CircularProgressDelegate();

	inline void setDashOffset(qreal offset);
	inline void setDashLength(qreal length);
	inline void setAngle(int angle);

	inline qreal dashOffset() const;
	inline qreal dashLength() const;
	inline int angle() const;

private:
	CircularProgress *const progress_;

	qreal dash_offset_;
	qreal dash_length_;

	int angle_;
};

inline void
CircularProgressDelegate::setDashOffset(qreal offset)
{
	dash_offset_ = offset;
	progress_->update();
}

inline void
CircularProgressDelegate::setDashLength(qreal length)
{
	dash_length_ = length;
	progress_->update();
}

inline void
CircularProgressDelegate::setAngle(int angle)
{
	angle_ = angle;
	progress_->update();
}

inline qreal
CircularProgressDelegate::dashOffset() const
{
	return dash_offset_;
}

inline qreal
CircularProgressDelegate::dashLength() const
{
	return dash_length_;
}

inline int
CircularProgressDelegate::angle() const
{
	return angle_;
}
