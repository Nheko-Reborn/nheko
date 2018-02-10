#pragma once

#include <QPainterPath>

#include "OverlayWidget.h"

class Ripple;

class RippleOverlay : public OverlayWidget
{
        Q_OBJECT

public:
        explicit RippleOverlay(QWidget *parent = 0);

        void addRipple(Ripple *ripple);
        void addRipple(const QPoint &position, qreal radius = 300);

        void removeRipple(Ripple *ripple);

        inline void setClipping(bool enable);
        inline bool hasClipping() const;

        inline void setClipPath(const QPainterPath &path);

protected:
        void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;

private:
        Q_DISABLE_COPY(RippleOverlay)

        void paintRipple(QPainter *painter, Ripple *ripple);

        QList<Ripple *> ripples_;
        QPainterPath clip_path_;
        bool use_clip_;
};

inline void
RippleOverlay::setClipping(bool enable)
{
        use_clip_ = enable;
        update();
}

inline bool
RippleOverlay::hasClipping() const
{
        return use_clip_;
}

inline void
RippleOverlay::setClipPath(const QPainterPath &path)
{
        clip_path_ = path;
        update();
}
