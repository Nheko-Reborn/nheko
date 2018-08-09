#include <QDebug>
#include <QPainter>
#include <QPoint>
#include <QShowEvent>

#include "Config.h"
#include "TypingDisplay.h"
#include "ui/Painter.h"

constexpr int LEFT_PADDING = 24;

TypingDisplay::TypingDisplay(QWidget *parent)
  : OverlayWidget(parent)
  , offset_{conf::textInput::height}
{
        QFont f;
        f.setPixelSize(conf::typingNotificationFontSize);
        setFont(f);

        setFixedHeight(QFontMetrics(font()).height() + 2);
        setAttribute(Qt::WA_TransparentForMouseEvents);
}

void
TypingDisplay::setOffset(int margin)
{
        offset_ = margin;
        move(0, parentWidget()->height() - offset_ - height());
}

void
TypingDisplay::setUsers(const QStringList &uid)
{
        move(0, parentWidget()->height() - offset_ - height());

        text_.clear();

        if (uid.isEmpty()) {
                hide();
                update();

                return;
        }

        text_ = uid.join(", ");

        if (uid.size() == 1)
                text_ += tr(" is typing");
        else if (uid.size() > 1)
                text_ += tr(" are typing");

        show();
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
