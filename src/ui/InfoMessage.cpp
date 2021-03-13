// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "InfoMessage.h"
#include "Config.h"

#include <QDateTime>
#include <QLocale>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QtGlobal>

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
#if QT_VERSION < QT_VERSION_CHECK(5, 11, 0)
        // width deprecated in 5.13
        width_ = fm.width(msg_) + HPadding * 2;
#else
        width_ = fm.horizontalAdvance(msg_) + HPadding * 2;
#endif

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

        QString fmt = QLocale::system().dateFormat(QLocale::LongFormat);

        if (now.date().year() == datetime.date().year()) {
                QRegularExpression rx("[^a-zA-Z]*y+[^a-zA-Z]*");
                fmt = fmt.remove(rx);
        }

        msg_ = datetime.date().toString(fmt);

        QFontMetrics fm{font()};
#if QT_VERSION < QT_VERSION_CHECK(5, 11, 0)
        // width deprecated in 5.13
        width_ = fm.width(msg_) + HPadding * 2;
#else
        width_ = fm.horizontalAdvance(msg_) + HPadding * 2;
#endif
        height_ = fm.ascent() + 2 * VPadding;

        setFixedHeight(height_ + 2 * HMargin);
}
