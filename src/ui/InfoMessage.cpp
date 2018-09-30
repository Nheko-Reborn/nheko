#include "Config.h"
#include "InfoMessage.h"

#include <QDateTime>
#include <QPainter>
#include <QPen>

constexpr int VPadding = 6;
constexpr int HPadding = 12;
constexpr int HMargin  = 20;

InfoMessage::InfoMessage(QWidget *parent)
  : QWidget{parent}
{
        initFont();
}

InfoMessage::InfoMessage(QString msg, QWidget *parent)
  : QWidget{parent}
  , msg_{msg}
{
        initFont();

        QFontMetrics fm{font()};
        width_  = fm.width(msg_) + HPadding * 2;
        height_ = fm.ascent() + 2 * VPadding;

        setFixedHeight(height_ + 2 * HMargin);
}

void
InfoMessage::paintEvent(QPaintEvent *)
{
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setFont(font());

        // Center the box horizontally & vertically.
        auto textRegion = QRectF(width() / 2 - width_ / 2, HMargin, width_, height_);

        QPainterPath ppath;
        ppath.addRoundedRect(textRegion, height_ / 2, height_ / 2);

        p.setPen(Qt::NoPen);
        p.fillPath(ppath, boxColor());
        p.drawPath(ppath);

        p.setPen(QPen(textColor()));
        p.drawText(textRegion, Qt::AlignCenter, msg_);
}

DateSeparator::DateSeparator(QDateTime datetime, QWidget *parent)
  : InfoMessage{parent}
{
        auto now = QDateTime::currentDateTime();

        QString fmt;

        if (now.date().year() != datetime.date().year())
                fmt = QString("ddd d MMMM yy");
        else
                fmt = QString("ddd d MMMM");

        msg_ = datetime.toString(fmt);

        QFontMetrics fm{font()};
        width_  = fm.width(msg_) + HPadding * 2;
        height_ = fm.ascent() + 2 * VPadding;

        setFixedHeight(height_ + 2 * HMargin);
}
