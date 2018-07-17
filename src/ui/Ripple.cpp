#include "Ripple.h"
#include "RippleOverlay.h"

Ripple::Ripple(const QPoint &center, QObject *parent)
  : QParallelAnimationGroup(parent)
  , overlay_(0)
  , radius_anim_(animate("radius"))
  , opacity_anim_(animate("opacity"))
  , radius_(0)
  , opacity_(0)
  , center_(center)
{
        init();
}

Ripple::Ripple(const QPoint &center, RippleOverlay *overlay, QObject *parent)
  : QParallelAnimationGroup(parent)
  , overlay_(overlay)
  , radius_anim_(animate("radius"))
  , opacity_anim_(animate("opacity"))
  , radius_(0)
  , opacity_(0)
  , center_(center)
{
        init();
}

void
Ripple::setRadius(qreal radius)
{
        Q_ASSERT(overlay_);

        if (radius_ == radius)
                return;

        radius_ = radius;
        overlay_->update();
}

void
Ripple::setOpacity(qreal opacity)
{
        Q_ASSERT(overlay_);

        if (opacity_ == opacity)
                return;

        opacity_ = opacity;
        overlay_->update();
}

void
Ripple::setColor(const QColor &color)
{
        if (brush_.color() == color)
                return;

        brush_.setColor(color);

        if (overlay_)
                overlay_->update();
}

void
Ripple::setBrush(const QBrush &brush)
{
        brush_ = brush;

        if (overlay_)
                overlay_->update();
}

void
Ripple::destroy()
{
        Q_ASSERT(overlay_);

        overlay_->removeRipple(this);
}

QPropertyAnimation *
Ripple::animate(const QByteArray &property, const QEasingCurve &easing, int duration)
{
        QPropertyAnimation *animation = new QPropertyAnimation;
        animation->setTargetObject(this);
        animation->setPropertyName(property);
        animation->setEasingCurve(easing);
        animation->setDuration(duration);

        addAnimation(animation);

        return animation;
}

void
Ripple::init()
{
        setOpacityStartValue(0.5);
        setOpacityEndValue(0);
        setRadiusStartValue(0);
        setRadiusEndValue(300);

        brush_.setColor(Qt::black);
        brush_.setStyle(Qt::SolidPattern);

        connect(this, SIGNAL(finished()), this, SLOT(destroy()));
}
