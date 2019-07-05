#include <QDebug>
#include <QPainter>
#include <QPoint>
#include <QShowEvent>

#include "Config.h"
#include "TypingDisplay.h"
#include "ui/Painter.h"

constexpr int LEFT_PADDING = 24;
constexpr int RECT_PADDING = 2;

TypingDisplay::TypingDisplay(QWidget *parent)
  : OverlayWidget(parent)
  , offset_{conf::textInput::height}
{
        setFixedHeight(QFontMetrics(font()).height() + RECT_PADDING);
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

        QFont f;
        f.setPointSizeF(f.pointSizeF() * 0.9);

        p.setFont(f);
        p.setPen(QPen(textColor()));

        QRect region = rect();
        region.translate(LEFT_PADDING, 0);

        QFontMetrics fm(f);
        text_ = fm.elidedText(text_, Qt::ElideRight, (double)(width() * 0.75));

        QPainterPath path;
        path.addRoundedRect(QRectF(0, 0, fm.horizontalAdvance(text_) + 2 * LEFT_PADDING, height()), 3, 3);

        p.fillPath(path, backgroundColor());
        p.drawText(region, Qt::AlignVCenter, text_);
}
