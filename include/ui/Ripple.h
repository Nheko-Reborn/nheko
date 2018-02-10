#pragma once

#include <QBrush>
#include <QEasingCurve>
#include <QParallelAnimationGroup>
#include <QPoint>
#include <QPropertyAnimation>

class RippleOverlay;

class Ripple : public QParallelAnimationGroup
{
        Q_OBJECT

        Q_PROPERTY(qreal radius WRITE setRadius READ radius)
        Q_PROPERTY(qreal opacity WRITE setOpacity READ opacity)

public:
        explicit Ripple(const QPoint &center, QObject *parent = 0);
        Ripple(const QPoint &center, RippleOverlay *overlay, QObject *parent = 0);

        inline void setOverlay(RippleOverlay *overlay);

        void setRadius(qreal radius);
        void setOpacity(qreal opacity);
        void setColor(const QColor &color);
        void setBrush(const QBrush &brush);

        inline qreal radius() const;
        inline qreal opacity() const;
        inline QColor color() const;
        inline QBrush brush() const;
        inline QPoint center() const;

        inline QPropertyAnimation *radiusAnimation() const;
        inline QPropertyAnimation *opacityAnimation() const;

        inline void setOpacityStartValue(qreal value);
        inline void setOpacityEndValue(qreal value);
        inline void setRadiusStartValue(qreal value);
        inline void setRadiusEndValue(qreal value);
        inline void setDuration(int msecs);

protected slots:
        void destroy();

private:
        Q_DISABLE_COPY(Ripple)

        QPropertyAnimation *animate(const QByteArray &property,
                                    const QEasingCurve &easing = QEasingCurve::OutQuad,
                                    int duration               = 800);

        void init();

        RippleOverlay *overlay_;

        QPropertyAnimation *const radius_anim_;
        QPropertyAnimation *const opacity_anim_;

        qreal radius_;
        qreal opacity_;

        QPoint center_;
        QBrush brush_;
};

inline void
Ripple::setOverlay(RippleOverlay *overlay)
{
        overlay_ = overlay;
}

inline qreal
Ripple::radius() const
{
        return radius_;
}

inline qreal
Ripple::opacity() const
{
        return opacity_;
}

inline QColor
Ripple::color() const
{
        return brush_.color();
}

inline QBrush
Ripple::brush() const
{
        return brush_;
}

inline QPoint
Ripple::center() const
{
        return center_;
}

inline QPropertyAnimation *
Ripple::radiusAnimation() const
{
        return radius_anim_;
}

inline QPropertyAnimation *
Ripple::opacityAnimation() const
{
        return opacity_anim_;
}

inline void
Ripple::setOpacityStartValue(qreal value)
{
        opacity_anim_->setStartValue(value);
}

inline void
Ripple::setOpacityEndValue(qreal value)
{
        opacity_anim_->setEndValue(value);
}

inline void
Ripple::setRadiusStartValue(qreal value)
{
        radius_anim_->setStartValue(value);
}

inline void
Ripple::setRadiusEndValue(qreal value)
{
        radius_anim_->setEndValue(value);
}

inline void
Ripple::setDuration(int msecs)
{
        radius_anim_->setDuration(msecs);
        opacity_anim_->setDuration(msecs);
}
