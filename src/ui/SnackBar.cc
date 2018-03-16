#include <QDebug>
#include <QPainter>

#include <tweeny.h>

#include "SnackBar.h"

constexpr int STARTING_OFFSET = 1;

SnackBar::SnackBar(QWidget *parent)
  : OverlayWidget(parent)
{
        bgOpacity_  = 0.9;
        duration_   = 6000;
        boxWidth_   = 400;
        boxHeight_  = 40;
        boxPadding_ = 10;
        textColor_  = QColor("white");
        bgColor_    = QColor("#333");
        offset_     = STARTING_OFFSET;
        position_   = SnackBarPosition::Top;

        QFont font("Open Sans");
        font.setPixelSize(14);
        font.setWeight(50);
        setFont(font);

        showTimer_ = QSharedPointer<QTimer>(new QTimer);
        hideTimer_ = QSharedPointer<QTimer>(new QTimer);
        hideTimer_->setSingleShot(true);

        auto offset_anim = tweeny::from(1.0f).to(0.0f).during(4000).via(tweeny::easing::elasticOut);
        connect(showTimer_.data(), &QTimer::timeout, this, [this, offset_anim]() mutable {
                if (offset_anim.progress() < 1.0f) {
                        offset_ = offset_anim.step(0.02f);
                        update();
                } else {
                        showTimer_->stop();
                        hideTimer_->start(duration_);
                        offset_anim.seek(0.0f);
                }
        });

        connect(hideTimer_.data(), SIGNAL(timeout()), this, SLOT(hideMessage()));
}

void
SnackBar::start()
{
        show();
        raise();

        showTimer_->start(10);
}

void
SnackBar::hideMessage()
{
        stopTimers();
        hide();

        // Moving on to the next message.
        messages_.removeFirst();

        // Reseting the starting position of the widget.
        offset_ = STARTING_OFFSET;

        if (!messages_.isEmpty())
                start();
}

void
SnackBar::stopTimers()
{
        showTimer_->stop();
        hideTimer_->stop();
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

        if (messages_.isEmpty())
                return;

        auto message_ = messages_.first();

        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        QBrush brush;
        brush.setStyle(Qt::SolidPattern);
        brush.setColor(bgColor_);
        p.setBrush(brush);
        p.setOpacity(bgOpacity_);

        QRect r(0, 0, boxWidth_, boxHeight_);

        p.setPen(Qt::white);
        QRect br = p.boundingRect(r, Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap, message_);

        p.setPen(Qt::NoPen);
        r = br.united(r).adjusted(-boxPadding_, -boxPadding_, boxPadding_, boxPadding_);

        const qreal s = 1 - offset_;

        if (position_ == SnackBarPosition::Bottom)
                p.translate((width() - (r.width() - 2 * boxPadding_)) / 2,
                            height() - boxPadding_ - s * (r.height()));
        else
                p.translate((width() - (r.width() - 2 * boxPadding_)) / 2,
                            s * (r.height()) - 2 * boxPadding_);

        br.moveCenter(r.center());
        p.drawRoundedRect(r.adjusted(0, 0, 0, 3), 3, 3);
        p.setPen(textColor_);
        p.drawText(br, Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap, message_);
}
