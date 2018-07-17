#include "OverlayWidget.h"
#include <QEvent>

OverlayWidget::OverlayWidget(QWidget *parent)
  : QWidget(parent)
{
        if (parent) {
                parent->installEventFilter(this);
                setGeometry(overlayGeometry());
                raise();
        }
}

bool
OverlayWidget::event(QEvent *event)
{
        if (!parent())
                return QWidget::event(event);

        switch (event->type()) {
        case QEvent::ParentChange: {
                parent()->installEventFilter(this);
                setGeometry(overlayGeometry());
                break;
        }
        case QEvent::ParentAboutToChange: {
                parent()->removeEventFilter(this);
                break;
        }
        default:
                break;
        }

        return QWidget::event(event);
}

bool
OverlayWidget::eventFilter(QObject *obj, QEvent *event)
{
        switch (event->type()) {
        case QEvent::Move:
        case QEvent::Resize:
                setGeometry(overlayGeometry());
                break;
        default:
                break;
        }

        return QWidget::eventFilter(obj, event);
}

QRect
OverlayWidget::overlayGeometry() const
{
        QWidget *widget = parentWidget();

        if (!widget)
                return QRect();

        return widget->rect();
}

void
OverlayWidget::paintEvent(QPaintEvent *event)
{
        Q_UNUSED(event);

        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
