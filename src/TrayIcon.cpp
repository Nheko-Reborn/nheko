// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QAction>
#include <QApplication>
#include <QList>
#include <QMenu>
#include <QPainter>
#include <QTimer>
#include <QWindow>

#include "TrayIcon.h"

MsgCountComposedIcon::MsgCountComposedIcon(const QIcon &icon)
  : QIconEngine()
  , icon_{icon}
{
}

void
MsgCountComposedIcon::paint(QPainter *painter,
                            const QRect &rect,
                            QIcon::Mode mode,
                            QIcon::State state)
{
    painter->setRenderHint(QPainter::TextAntialiasing);
    painter->setRenderHint(QPainter::SmoothPixmapTransform);
    painter->setRenderHint(QPainter::Antialiasing);

    icon_.paint(painter, rect, Qt::AlignCenter, mode, state);

    if (msgCount <= 0)
        return;

    QColor backgroundColor("red");
    QColor textColor("white");

    QBrush brush;
    brush.setStyle(Qt::SolidPattern);
    brush.setColor(backgroundColor);

    QFont f;
    f.setPixelSize(int(BubbleDiameter * 0.6));
    f.setWeight(QFont::Bold);

    painter->setBrush(brush);
    painter->setPen(Qt::NoPen);
    painter->setFont(f);

    QRectF bubble(rect.width() - BubbleDiameter,
                  rect.height() - BubbleDiameter,
                  BubbleDiameter,
                  BubbleDiameter);
    painter->drawEllipse(bubble);
    painter->setPen(QPen(textColor));
    painter->setBrush(Qt::NoBrush);
    painter->drawText(bubble, Qt::AlignCenter, QString::number(msgCount));
}

QIconEngine *
MsgCountComposedIcon::clone() const
{
    return new MsgCountComposedIcon(*this);
}

QList<QSize>
MsgCountComposedIcon::availableSizes(QIcon::Mode mode, QIcon::State state)
{
    Q_UNUSED(mode);
    Q_UNUSED(state);
    QList<QSize> sizes;
    sizes.append(QSize(24, 24));
    sizes.append(QSize(32, 32));
    sizes.append(QSize(48, 48));
    sizes.append(QSize(64, 64));
    sizes.append(QSize(128, 128));
    sizes.append(QSize(256, 256));
    return sizes;
}

QPixmap
MsgCountComposedIcon::pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state)
{
    QImage img(size, QImage::Format_ARGB32);
    img.fill(qRgba(0, 0, 0, 0));
    QPixmap result = QPixmap::fromImage(img, Qt::NoFormatConversion);
    {
        QPainter painter(&result);
        paint(&painter, QRect(QPoint(0, 0), size), mode, state);
    }
    return result;
}

TrayIcon::TrayIcon(const QString &filename, QWindow *parent)
  : QSystemTrayIcon(parent)
  , icon(filename)
{
#if defined(Q_OS_MACOS) || defined(Q_OS_WIN)
    setIcon(icon);
#else
    auto icon_ = new MsgCountComposedIcon(icon);
    setIcon(QIcon(icon_));
#endif

    QMenu *menu = new QMenu();
    setContextMenu(menu);

    viewAction_ = new QAction(tr("Show"), this);
    quitAction_ = new QAction(tr("Quit"), this);

    connect(viewAction_, &QAction::triggered, parent, &QWindow::show);
    connect(quitAction_, &QAction::triggered, this, QApplication::quit);

    menu->addAction(viewAction_);
    menu->addAction(quitAction_);
}

void
TrayIcon::setUnreadCount(int count)
{
    qGuiApp->setBadgeNumber(count);

#if !defined(Q_OS_MACOS) && !defined(Q_OS_WIN)
    if (count != previousCount) {
        auto i      = new MsgCountComposedIcon(icon);
        i->msgCount = count;
        setIcon(QIcon(i));
        previousCount = count;
    }
#endif

    QString toolTip = QLatin1String("nheko");
    if (count > 0) {
        toolTip.append(tr("\n%n unread message(s)", "", count));
    }

    setToolTip(toolTip);
}

#include "moc_TrayIcon.cpp"
