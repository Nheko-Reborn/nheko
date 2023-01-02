// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-FileCopyrightText: 2023 Nheko Contributors
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