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

#pragma once

#include <QDebug>
#include <QEasingCurve>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QStackedWidget>
#include <QWidget>

/*
 * SlidingStackWidget allows smooth side shifting of widgets,
 * in addition to the hard switching from one to another offered
 * by QStackedWidget.
 */

class SlidingStackWidget : public QStackedWidget
{
	Q_OBJECT

public:
	// Defines the animation direction.
	enum class AnimationDirection {
		LEFT_TO_RIGHT,
		RIGHT_TO_LEFT,
		AUTOMATIC
	};

	SlidingStackWidget(QWidget *parent);
	~SlidingStackWidget();

public slots:
	// Move to the next widget.
	void slideInNext();

	// Move to the previous widget.
	void slideInPrevious();

	// Move to a widget by index.
	void slideInIndex(int index, AnimationDirection direction = AnimationDirection::AUTOMATIC);

	int getWidgetIndex(QWidget *widget);
signals:
	// Internal signal to alert the engine for the animation's end.
	void animationFinished();

protected slots:
	// Internal slot to handle the end of the animation.
	void onAnimationFinished();

protected:
	// The method that does the main work for the widget transition.
	void slideInWidget(QWidget *widget, AnimationDirection direction = AnimationDirection::AUTOMATIC);

	// Indicates whether or not the animation is active.
	bool active_;

	// The widget currently displayed.
	QWidget *window_;

	// The speed of the animation in milliseconds.
	int speed_;

	// The animation type.
	QEasingCurve::Type animation_type_;

	// Current widget's index.
	int now_;

	// Reference point.
	QPoint current_position_;

	// Next widget's to show index.
	int next_;
};
