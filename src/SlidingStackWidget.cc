/*
 * nheko Copyright (C) 2017  Konstantinos Sideris <siderisk@auth.gr>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "SlidingStackWidget.h"

SlidingStackWidget::SlidingStackWidget(QWidget *parent)
  : QStackedWidget(parent)
{
	window_ = parent;

	if (parent == Q_NULLPTR) {
		qDebug() << "Using nullptr for parent";
		window_ = this;
	}

	current_position_ = QPoint(0, 0);
	speed_ = 400;
	now_ = 0;
	next_ = 0;
	active_ = false;
	animation_type_ = QEasingCurve::InOutCirc;
}

SlidingStackWidget::~SlidingStackWidget()
{
}

void
SlidingStackWidget::slideInNext()
{
	int now = currentIndex();

	if (now < count() - 1)
		slideInIndex(now + 1);
}

void
SlidingStackWidget::slideInPrevious()
{
	int now = currentIndex();

	if (now > 0)
		slideInIndex(now - 1);
}

void
SlidingStackWidget::slideInIndex(int index, AnimationDirection direction)
{
	// Take into consideration possible index overflow/undeflow.
	if (index > count() - 1) {
		direction = AnimationDirection::RIGHT_TO_LEFT;
		index = index % count();
	} else if (index < 0) {
		direction = AnimationDirection::LEFT_TO_RIGHT;
		index = (index + count()) % count();
	}

	slideInWidget(widget(index), direction);
}

void
SlidingStackWidget::slideInWidget(QWidget *next_widget, AnimationDirection direction)
{
	// If an animation is currenlty executing we should wait for it to finish before
	// another transition can start.
	if (active_)
		return;

	active_ = true;

	int now = currentIndex();
	int next = indexOf(next_widget);

	if (now == next) {
		active_ = false;
		return;
	}

	int offset_x = frameRect().width();

	next_widget->setGeometry(0, 0, offset_x, 0);

	if (direction == AnimationDirection::LEFT_TO_RIGHT) {
		offset_x = -offset_x;
	}

	QPoint pnext = next_widget->pos();
	QPoint pnow = widget(now)->pos();
	current_position_ = pnow;

	// Reposition the next widget outside of the display area.
	next_widget->move(pnext.x() - offset_x, pnext.y());

	// Make the widget visible.
	next_widget->show();
	next_widget->raise();

	// Animate both the next and now widget.
	QPropertyAnimation *animation_now = new QPropertyAnimation(widget(now), "pos", this);

	animation_now->setDuration(speed_);
	animation_now->setEasingCurve(animation_type_);
	animation_now->setStartValue(QPoint(pnow.x(), pnow.y()));
	animation_now->setEndValue(QPoint(pnow.x() + offset_x, pnow.y()));

	QPropertyAnimation *animation_next = new QPropertyAnimation(next_widget, "pos", this);

	animation_next->setDuration(speed_);
	animation_next->setEasingCurve(animation_type_);
	animation_next->setStartValue(QPoint(pnext.x() - offset_x, pnext.y()));
	animation_next->setEndValue(QPoint(pnext.x(), pnext.y()));

	QParallelAnimationGroup *animation_group = new QParallelAnimationGroup(this);

	animation_group->addAnimation(animation_now);
	animation_group->addAnimation(animation_next);

	connect(animation_group, SIGNAL(finished()), this, SLOT(onAnimationFinished()));

	next_ = next;
	now_ = now;
	animation_group->start();
}

void
SlidingStackWidget::onAnimationFinished()
{
	setCurrentIndex(next_);

	// The old widget is no longer necessary so we can hide it and
	// move it back to its original position.
	widget(now_)->hide();
	widget(now_)->move(current_position_);

	active_ = false;
	emit animationFinished();
}

int
SlidingStackWidget::getWidgetIndex(QWidget *widget)
{
	return indexOf(widget);
}
