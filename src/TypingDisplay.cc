#include <QPainter>
#include <QPoint>

#include "Config.h"
#include "TypingDisplay.h"

TypingDisplay::TypingDisplay(QWidget *parent)
  : QWidget(parent)
  , leftPadding_{ 24 }
{
        QFont font;
        font.setPixelSize(conf::typingNotificationFontSize);

        setFixedHeight(QFontMetrics(font).height() + 2);
}

void
TypingDisplay::setUsers(const QStringList &uid)
{
        if (uid.isEmpty())
                text_.clear();
        else
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
        QPen pen(QColor("#898989"));

        QFont font("Open Sans Bold");
        font.setPixelSize(conf::typingNotificationFontSize);
        font.setItalic(true);

        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setFont(font);
        p.setPen(pen);

        QRect region = rect();
        region.translate(leftPadding_, 0);

        QFontMetrics fm(font);
        text_ = fm.elidedText(text_, Qt::ElideRight, width() - 3 * leftPadding_);

        p.drawText(region, Qt::AlignVCenter, text_);
}
