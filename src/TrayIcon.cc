/*
 * nheko Copyright (C) 2017  Konstantinos Sideris <siderisk@auth.gr>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QApplication>
#include <QTimer>

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

        painter->setBrush(brush);
        painter->setPen(Qt::NoPen);
        painter->setFont(QFont("Open Sans", 8, QFont::Black));

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

TrayIcon::TrayIcon(const QString &filename, QWidget *parent)
  : QSystemTrayIcon(parent)
{
#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
        setIcon(QIcon(filename));
#else
        icon_ = new MsgCountComposedIcon(filename);
        setIcon(QIcon(icon_));
#endif

        QMenu *menu = new QMenu(parent);
        viewAction_ = new QAction(tr("Show"), parent);
        quitAction_ = new QAction(tr("Quit"), parent);

        connect(viewAction_, SIGNAL(triggered()), parent, SLOT(show()));
        connect(quitAction_, &QAction::triggered, this, QApplication::quit);

        menu->addAction(viewAction_);
        menu->addAction(quitAction_);

        setContextMenu(menu);

        // We wait a little for the icon to load.
        QTimer::singleShot(500, this, [=]() { show(); });
}

void
TrayIcon::setUnreadCount(int count)
{
// Use the native badge counter in MacOS.
#if defined(Q_OS_MAC)
        if (count == 0)
                QtMac::setBadgeLabelText("");
        else
                QtMac::setBadgeLabelText(QString::number(count));
#elif defined(Q_OS_WIN)
// FIXME: Find a way to use Windows apis for the badge counter (if any).
#else
        // Custom drawing on Linux.
        // FIXME: It doesn't seem to work on KDE.
        MsgCountComposedIcon *tmp = static_cast<MsgCountComposedIcon *>(icon_->clone());
        tmp->msgCount             = count;

        setIcon(QIcon(tmp));

        icon_ = tmp;
#endif
}
