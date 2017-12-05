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

#include <QDebug>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>

#include "ScrollBar.h"

ScrollBar::ScrollBar(QScrollArea *area, QWidget *parent)
  : QScrollBar(parent)
  , area_{area}
{
        hideTimer_.setSingleShot(true);

        connect(&hideTimer_, &QTimer::timeout, this, &ScrollBar::fadeOut);

        eff = new QGraphicsOpacityEffect(this);
        setGraphicsEffect(eff);
}

void
ScrollBar::fadeOut()
{
        isActive = false;

        QPropertyAnimation *anim = new QPropertyAnimation(eff, "opacity");
        anim->setDuration(AnimationDuration);
        anim->setStartValue(1);
        anim->setEndValue(0);
        anim->setEasingCurve(QEasingCurve::Linear);
        anim->start(QPropertyAnimation::DeleteWhenStopped);
}

void
ScrollBar::fadeIn()
{
        QPropertyAnimation *anim = new QPropertyAnimation(eff, "opacity");
        anim->setDuration(AnimationDuration);
        anim->setStartValue(0);
        anim->setEndValue(1);
        anim->setEasingCurve(QEasingCurve::Linear);
        anim->start(QPropertyAnimation::DeleteWhenStopped);
}

void
ScrollBar::sliderChange(SliderChange change)
{
        if (!isActive)
                fadeIn();

        hideTimer_.stop();
        hideTimer_.start(1500);
        isActive = true;

        QScrollBar::sliderChange(change);
}

void
ScrollBar::paintEvent(QPaintEvent *)
{
        if (!width() && !height()) {
                hide();
                return;
        }

        QPainter p(this);
        p.setRenderHint(QPainter::TextAntialiasing);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);

        p.setPen(Qt::NoPen);

        p.setBrush(backgroundColor());
        QRect backgroundArea(Padding, 0, handleWidth_, height());
        p.drawRoundedRect(backgroundArea, roundRadius_, roundRadius_);

        int areaHeight   = area_->height();
        int widgetHeight = area_->widget()->height();

        double visiblePercentage = (double)areaHeight / (double)widgetHeight;
        int handleHeight = std::max(visiblePercentage * areaHeight, (double)minHandleHeight_);

        if (maximum() == 0) {
                return;
        }

        int handle_y = (value() * (areaHeight - handleHeight - roundRadius_ / 2)) / maximum();

        p.setBrush(handleColor());
        QRect handleArea(Padding, handle_y, handleWidth_, handleHeight);
        p.drawRoundedRect(handleArea, roundRadius_, roundRadius_);
}
