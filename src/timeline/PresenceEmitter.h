// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

#include <vector>

#include <mtx/events.hpp>
#include <mtx/events/presence.hpp>

class PresenceEmitter : public QObject
{
    Q_OBJECT

public:
    PresenceEmitter(QObject *p = nullptr)
      : QObject(p)
    {}

    void sync(const std::vector<mtx::events::Event<mtx::events::presence::Presence>> &presences);

    Q_INVOKABLE QString userPresence(QString id) const;
    Q_INVOKABLE QString userStatus(QString id) const;

signals:
    void presenceChanged(QString userid);
};
