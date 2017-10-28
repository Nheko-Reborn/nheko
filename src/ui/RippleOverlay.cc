#include <QPainter>

#include "Ripple.h"
#include "RippleOverlay.h"

RippleOverlay::RippleOverlay(QWidget *parent)
  : OverlayWidget(parent)
  , use_clip_(false)
{
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_NoSystemBackground);
}

RippleOverlay::~RippleOverlay() {}

void
RippleOverlay::addRipple(Ripple *ripple)
{
        ripple->setOverlay(this);
        ripples_.push_back(ripple);
        ripple->start();
}

void
RippleOverlay::addRipple(const QPoint &position, qreal radius)
{
        Ripple *ripple = new Ripple(position);
        ripple->setRadiusEndValue(radius);
        addRipple(ripple);
}

void
RippleOverlay::removeRipple(Ripple *ripple)
{
        if (ripples_.removeOne(ripple))
                delete ripple;
}

void
RippleOverlay::paintEvent(QPaintEvent *event)
{
        Q_UNUSED(event)

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(Qt::NoPen);

        if (use_clip_)
                painter.setClipPath(clip_path_);

        for (auto it = ripples_.constBegin(); it != ripples_.constEnd(); ++it)
                paintRipple(&painter, *it);
}

void
RippleOverlay::paintRipple(QPainter *painter, Ripple *ripple)
{
        const qreal radius   = ripple->radius();
        const QPointF center = ripple->center();

        painter->setOpacity(ripple->opacity());
        painter->setBrush(ripple->brush());
        painter->drawEllipse(center, radius, radius);
}
