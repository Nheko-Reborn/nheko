// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QPainter>

#include "SnackBar.h"

constexpr int STARTING_OFFSET         = 1;
constexpr int BOX_PADDING             = 10;
constexpr double MIN_WIDTH            = 400.0;
constexpr double MIN_WIDTH_PERCENTAGE = 0.3;

SnackBar::SnackBar(QWidget *parent)
  : OverlayWidget(parent)
  , offset_anim(this, "offset", this)
{
    QFont font;
    font.setPointSizeF(font.pointSizeF() * 1.2);
    font.setWeight(QFont::Weight::Thin);
    setFont(font);

    boxHeight_ = QFontMetrics(font).height() * 2;
    offset_    = STARTING_OFFSET;
    position_  = SnackBarPosition::Top;

    hideTimer_.setSingleShot(true);

    offset_anim.setStartValue(1.0);
    offset_anim.setEndValue(0.0);
    offset_anim.setDuration(100);
    offset_anim.setEasingCurve(QEasingCurve::OutCubic);

    connect(this, &SnackBar::offsetChanged, this, [this]() mutable { repaint(); });
    connect(
      &offset_anim, &QPropertyAnimation::finished, this, [this]() { hideTimer_.start(10000); });

    connect(&hideTimer_, SIGNAL(timeout()), this, SLOT(hideMessage()));

    hide();
}

void
SnackBar::start()
{
    if (messages_.empty())
        return;

    show();
    raise();

    offset_anim.start();
}

void
SnackBar::hideMessage()
{
    stopTimers();
    hide();

    if (!messages_.empty())
        // Moving on to the next message.
        messages_.pop_front();

    // Resetting the starting position of the widget.
    offset_ = STARTING_OFFSET;

    if (!messages_.empty())
        start();
}

void
SnackBar::stopTimers()
{
    hideTimer_.stop();
}

void
SnackBar::showMessage(const QString &msg)
{
    messages_.push_back(msg);

    // There is already an active message.
    if (isVisible())
        return;

    start();
}

void
SnackBar::mousePressEvent(QMouseEvent *)
{
    hideMessage();
}

void
SnackBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    if (messages_.empty())
        return;

    auto message_ = messages_.front();

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QBrush brush;
    brush.setStyle(Qt::SolidPattern);
    brush.setColor(bgColor_);
    p.setBrush(brush);

    QRect r(0, 0, std::max(MIN_WIDTH, width() * MIN_WIDTH_PERCENTAGE), boxHeight_);

    p.setPen(Qt::white);
    QRect br = p.boundingRect(r, Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap, message_);

    p.setPen(Qt::NoPen);
    r = br.united(r).adjusted(-BOX_PADDING, -BOX_PADDING, BOX_PADDING, BOX_PADDING);

    const qreal s = 1 - offset_;

    if (position_ == SnackBarPosition::Bottom)
        p.translate((width() - (r.width() - 2 * BOX_PADDING)) / 2,
                    height() - BOX_PADDING - s * (r.height()));
    else
        p.translate((width() - (r.width() - 2 * BOX_PADDING)) / 2,
                    s * (r.height()) - 2 * BOX_PADDING);

    br.moveCenter(r.center());
    p.drawRoundedRect(r.adjusted(0, 0, 0, 4), 4, 4);
    p.setPen(textColor_);
    p.drawText(br, Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap, message_);
}
