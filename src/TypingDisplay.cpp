#include <QPainter>
#include <QPoint>

#include "Config.h"
#include "TypingDisplay.h"
#include "ui/Painter.h"

constexpr int LEFT_PADDING = 24;

TypingDisplay::TypingDisplay(QWidget *parent)
  : QWidget(parent)
{
        QFont f;
        f.setPixelSize(conf::typingNotificationFontSize);

        setFont(f);

        setFixedHeight(QFontMetrics(font()).height() + 2);
}

void
TypingDisplay::setUsers(const QStringList &uid)
{
        if (uid.isEmpty()) {
                text_.clear();
                update();

                return;
        }

        text_ = uid.join(", ");

        if (uid.size() == 1)
                text_ += tr(" is typing");
        else if (uid.size() > 1)
                text_ += tr(" are typing");

        update();
}

void
TypingDisplay::paintEvent(QPaintEvent *)
{
        Painter p(this);
        PainterHighQualityEnabler hq(p);

        p.setFont(font());
        p.setPen(QPen(textColor()));

        QRect region = rect();
        region.translate(LEFT_PADDING, 0);

        QFontMetrics fm(font());
        text_ = fm.elidedText(text_, Qt::ElideRight, width() - 3 * LEFT_PADDING);

        p.drawText(region, Qt::AlignVCenter, text_);
}
