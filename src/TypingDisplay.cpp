#include <QDebug>
#include <QPainter>
#include <QPoint>
#include <QShowEvent>
#include <QtGlobal>

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

        QString temp = text_ +=
          tr("%1 and %2 are typing",
             "Multiple users are typing. First argument is a comma separated list of potentially "
             "multiple users. Second argument is the last user of that list. (If only one user is "
             "typing, %1 is empty. You should still use it in your string though to silence Qt "
             "warnings.)",
             uid.size());

        if (uid.isEmpty()) {
                hide();
                update();

                return;
        }

        QStringList uidWithoutLast = uid;
        uidWithoutLast.pop_back();
        text_ = temp.arg(uidWithoutLast.join(", ")).arg(uid.back());

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
#if QT_VERSION < QT_VERSION_CHECK(5, 11, 0)
        path.addRoundedRect(QRectF(0, 0, fm.width(text_) + 2 * LEFT_PADDING, height()), 3, 3);
#else
        path.addRoundedRect(
          QRectF(0, 0, fm.horizontalAdvance(text_) + 2 * LEFT_PADDING, height()), 3, 3);
#endif
        p.fillPath(path, backgroundColor());
        p.drawText(region, Qt::AlignVCenter, text_);
}
