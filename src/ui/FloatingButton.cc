#include <QPainterPath>

#include "FloatingButton.h"

FloatingButton::FloatingButton(const QIcon &icon, QWidget *parent)
  : RaisedButton(parent)
{
        setFixedSize(DIAMETER, DIAMETER);
        setGeometry(buttonGeometry());

        if (parentWidget())
                parentWidget()->installEventFilter(this);

        setFixedRippleRadius(50);
        setIcon(icon);
        raise();
}

QRect
FloatingButton::buttonGeometry() const
{
        QWidget *parent = parentWidget();

        if (!parent)
                return QRect();

        return QRect(parent->width() - (OFFSET_X + DIAMETER),
                     parent->height() - (OFFSET_Y + DIAMETER),
                     DIAMETER,
                     DIAMETER);
}

bool
FloatingButton::event(QEvent *event)
{
        if (!parent())
                return RaisedButton::event(event);

        switch (event->type()) {
        case QEvent::ParentChange: {
                parent()->installEventFilter(this);
                setGeometry(buttonGeometry());
                break;
        }
        case QEvent::ParentAboutToChange: {
                parent()->installEventFilter(this);
                break;
        }
        default:
                break;
        }

        return RaisedButton::event(event);
}

bool
FloatingButton::eventFilter(QObject *obj, QEvent *event)
{
        const QEvent::Type type = event->type();

        if (QEvent::Move == type || QEvent::Resize == type)
                setGeometry(buttonGeometry());

        return RaisedButton::eventFilter(obj, event);
}

void
FloatingButton::paintEvent(QPaintEvent *event)
{
        Q_UNUSED(event);

        QRect square = QRect(0, 0, DIAMETER, DIAMETER);
        square.moveCenter(rect().center());

        QPainter p(this);
        p.setRenderHints(QPainter::Antialiasing);

        QBrush brush;
        brush.setStyle(Qt::SolidPattern);
        brush.setColor(backgroundColor());

        p.setBrush(brush);
        p.setPen(Qt::NoPen);
        p.drawEllipse(square);

        QRect iconGeometry(0, 0, ICON_SIZE, ICON_SIZE);
        iconGeometry.moveCenter(square.center());

        QPixmap pixmap = icon().pixmap(QSize(ICON_SIZE, ICON_SIZE));
        QPainter icon(&pixmap);
        icon.setCompositionMode(QPainter::CompositionMode_SourceIn);
        icon.fillRect(pixmap.rect(), foregroundColor());

        p.drawPixmap(iconGeometry, pixmap);
}
