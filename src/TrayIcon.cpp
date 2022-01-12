// SPDX-FileCopyrightText: 2017 Konstantinos Sideris <siderisk@auth.gr>
// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
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

#if defined(Q_OS_MAC)
#include <QtMacExtras>
#endif

MsgCountComposedIcon::MsgCountComposedIcon(const QString &filename)
  : QIconEngine()
{
    icon_ = QIcon(filename);
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
    f.setPointSizeF(8);
    f.setWeight(QFont::Thin);

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
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  const
#endif
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
{
#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
    setIcon(QIcon(filename));
#else
    icon_ = new MsgCountComposedIcon(filename);
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
// Use the native badge counter in MacOS.
#if defined(Q_OS_MAC)
// currently, to avoid writing obj-c code, ignore deprecated warnings on the badge functions
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    auto labelText = count == 0 ? "" : QString::number(count);

    if (labelText == QtMac::badgeLabelText())
        return;

    QtMac::setBadgeLabelText(labelText);
#pragma clang diagnostic pop
#elif defined(Q_OS_WIN)
// FIXME: Find a way to use Windows apis for the badge counter (if any).
#else
    if (count == icon_->msgCount)
        return;

    // Custom drawing on Linux.
    MsgCountComposedIcon *tmp = static_cast<MsgCountComposedIcon *>(icon_->clone());
    tmp->msgCount             = count;

    setIcon(QIcon(tmp));

    icon_ = tmp;
#endif
}
