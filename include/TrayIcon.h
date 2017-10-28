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

#pragma once

#include <QAction>
#include <QIcon>
#include <QIconEngine>
#include <QPainter>
#include <QRect>
#include <QSystemTrayIcon>

class MsgCountComposedIcon : public QIconEngine
{
public:
        MsgCountComposedIcon(const QString &filename);

        virtual void paint(QPainter *p, const QRect &rect, QIcon::Mode mode, QIcon::State state);
        virtual QIconEngine *clone() const;
        virtual QList<QSize> availableSizes(QIcon::Mode mode, QIcon::State state) const;
        virtual QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state);

        int msgCount = 0;

private:
        const int BubbleDiameter = 17;

        QIcon icon_;
};

class TrayIcon : public QSystemTrayIcon
{
        Q_OBJECT
public:
        TrayIcon(const QString &filename, QWidget *parent);

public slots:
        void setUnreadCount(int count);

private:
        QAction *viewAction_;
        QAction *quitAction_;

        MsgCountComposedIcon *icon_;
};
