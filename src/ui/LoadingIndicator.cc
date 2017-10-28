#include "LoadingIndicator.h"

#include <QPoint>
#include <QtGlobal>

LoadingIndicator::LoadingIndicator(QWidget *parent)
  : QWidget(parent)
  , interval_(70)
  , angle_(0)
  , color_(Qt::black)
{
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        setFocusPolicy(Qt::NoFocus);

        timer_ = new QTimer();
        connect(timer_, SIGNAL(timeout()), this, SLOT(onTimeout()));
}

LoadingIndicator::~LoadingIndicator()
{
        stop();

        delete timer_;
}

void
LoadingIndicator::paintEvent(QPaintEvent *e)
{
        Q_UNUSED(e)

        if (!timer_->isActive())
                return;

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        int width = qMin(this->width(), this->height());

        int outerRadius = (width - 4) * 0.5f;
        int innerRadius = outerRadius * 0.78f;

        int capsuleRadius = (outerRadius - innerRadius) / 2;

        for (int i = 0; i < 8; ++i) {
                QColor color = color_;

                color.setAlphaF(1.0f - (i / 8.0f));

                painter.setPen(Qt::NoPen);
                painter.setBrush(color);

                qreal radius = capsuleRadius * (1.0f - (i / 16.0f));

                painter.save();

                painter.translate(rect().center());
                painter.rotate(angle_ - i * 45.0f);

                QPointF center = QPointF(-capsuleRadius, -innerRadius);
                painter.drawEllipse(center, radius * 2, radius * 2);

                painter.restore();
        }
}

void
LoadingIndicator::start()
{
        timer_->start(interval_);
        show();
}

void
LoadingIndicator::stop()
{
        timer_->stop();
        hide();
}

void
LoadingIndicator::onTimeout()
{
        angle_ = (angle_ + 45) % 360;
        update();
}
