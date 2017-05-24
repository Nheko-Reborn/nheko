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

#include <QGraphicsOpacityEffect>
#include <QPainter>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>

class ScrollBar : public QScrollBar
{
	Q_OBJECT
public:
	ScrollBar(QScrollArea *area, QWidget *parent = nullptr);

	void fadeIn();
	void fadeOut();

protected:
	void paintEvent(QPaintEvent *e) override;
	void sliderChange(SliderChange change) override;

private:
	int roundRadius_ = 4;
	int handleWidth_ = 7;
	int minHandleHeight_ = 20;
	bool isActive;

	const int AnimationDuration = 300;
	const int Padding = 4;

	QGraphicsOpacityEffect *eff;
	QTimer hideTimer_;

	QScrollArea *area_;
	QRect handle_;
};
