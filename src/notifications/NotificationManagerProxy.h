// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QString>

class NotificationManagerProxy final : public QObject
{
    Q_OBJECT
public:
    NotificationManagerProxy(QObject *parent = nullptr)
      : QObject(parent)
    {
    }

signals:
    void notificationReplied(const QString &room, const QString &event, const QString &reply);
};