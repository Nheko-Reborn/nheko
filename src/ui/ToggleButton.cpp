// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QColor>
#include <QCoreApplication>
#include <QEvent>
#include <QPainter>

#include "ToggleButton.h"

void
Toggle::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
}

Toggle::Toggle(QWidget *parent)
  : QAbstractButton{parent}
{
    init();

    connect(this, &QAbstractButton::toggled, this, &Toggle::setState);
}

void
Toggle::setState(bool isEnabled)
{
    setChecked(isEnabled);
    thumb_->setShift(isEnabled ? Position::Left : Position::Right);
    setupProperties();
}

void
Toggle::init()
{
    track_ = new ToggleTrack(this);
    thumb_ = new ToggleThumb(this);

    setCursor(QCursor(Qt::PointingHandCursor));
    setCheckable(true);
    setChecked(false);

    setState(false);
    setupProperties();

    QCoreApplication::processEvents();
}

void
Toggle::setupProperties()
{
    if (isEnabled()) {
        Position position = thumb_->shift();

        thumb_->setThumbColor(trackColor());

        if (position == Position::Left)
            track_->setTrackColor(activeColor());
        else if (position == Position::Right)
            track_->setTrackColor(inactiveColor());
    }

    update();
}

void
Toggle::setDisabledColor(const QColor &color)
{
    disabledColor_ = color;
    setupProperties();
    emit disabledColorChanged();
}

void
Toggle::setActiveColor(const QColor &color)
{
    activeColor_ = color;
    setupProperties();
    emit activeColorChanged();
}

void
Toggle::setInactiveColor(const QColor &color)
{
    inactiveColor_ = color;
    setupProperties();
    emit inactiveColorChanged();
}

void
Toggle::setTrackColor(const QColor &color)
{
    trackColor_ = color;
    setupProperties();
    emit trackColorChanged();
}

ToggleThumb::ToggleThumb(Toggle *parent)
  : QWidget{parent}
  , toggle_{parent}
  , position_{Position::Right}
  , offset_{0}
{
    parent->installEventFilter(this);
}

void
ToggleThumb::setShift(Position position)
{
    if (position_ != position) {
        position_ = position;
        updateOffset();
    }
}

bool
ToggleThumb::eventFilter(QObject *obj, QEvent *event)
{
    const QEvent::Type type = event->type();

    if (QEvent::Resize == type || QEvent::Move == type) {
        setGeometry(toggle_->rect().adjusted(8, 8, -8, -8));
        updateOffset();
    }

    return QWidget::eventFilter(obj, event);
}

void
ToggleThumb::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QBrush brush;
    brush.setStyle(Qt::SolidPattern);
    brush.setColor(toggle_->isEnabled() ? thumbColor_ : Qt::white);

    painter.setBrush(brush);
    painter.setPen(Qt::NoPen);

    int s;
    QRectF r;

    s = height() - 10;
    r = QRectF(5 + offset_, 5, s, s);

    painter.drawEllipse(r);

    if (!toggle_->isEnabled()) {
        brush.setColor(toggle_->disabledColor());
        painter.setBrush(brush);
        painter.drawEllipse(r);
    }
}

void
ToggleThumb::updateOffset()
{
    const QSize s(size());
    offset_ = position_ == Position::Left ? static_cast<qreal>(s.width() - s.height()) : 0;
    update();
}

ToggleTrack::ToggleTrack(Toggle *parent)
  : QWidget{parent}
  , toggle_{parent}
{
    Q_ASSERT(parent);

    parent->installEventFilter(this);
}

void
ToggleTrack::setTrackColor(const QColor &color)
{
    trackColor_ = color;
    emit trackColorChanged();
    update();
}

bool
ToggleTrack::eventFilter(QObject *obj, QEvent *event)
{
    const QEvent::Type type = event->type();

    if (QEvent::Resize == type || QEvent::Move == type) {
        setGeometry(toggle_->rect());
    }

    return QWidget::eventFilter(obj, event);
}

void
ToggleTrack::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QBrush brush;
    if (toggle_->isEnabled()) {
        brush.setColor(trackColor_);
        painter.setOpacity(0.8);
    } else {
        brush.setColor(toggle_->disabledColor());
        painter.setOpacity(0.6);
    }

    brush.setStyle(Qt::SolidPattern);
    painter.setBrush(brush);
    painter.setPen(Qt::NoPen);

    const int h = height() / 2;
    const QRect r(0, h / 2, width(), h);
    painter.drawRoundedRect(r.adjusted(14, 4, -14, -4), h / 2 - 4, h / 2 - 4);
}
