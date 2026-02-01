// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QIcon>
#include <QIconEngine>
#include <QRect>
#include <QSystemTrayIcon>

class QAction;
class QPainter;

class MsgCountComposedIcon final : public QIconEngine
{
public:
    MsgCountComposedIcon(const QIcon &icon);

    void paint(QPainter *p, const QRect &rect, QIcon::Mode mode, QIcon::State state) override;
    QIconEngine *clone() const override;
    QList<QSize> availableSizes(QIcon::Mode mode, QIcon::State state) override;
    QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override;

    int msgCount = 0;

private:
    const int BubbleDiameter = 17;

    QIcon icon_;
};

class TrayIcon final : public QSystemTrayIcon
{
    Q_OBJECT
public:
    TrayIcon(const QString &filename, QWindow *parent);

public slots:
    void setUnreadCount(int count);

private:
    QAction *toggleAction_;
    QAction *quitAction_;

    int previousCount = 0;
    QIcon icon;
};
