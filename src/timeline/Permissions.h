// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

#include <mtx/events/power_levels.hpp>

class TimelineModel;

class Permissions : public QObject
{
        Q_OBJECT

public:
        Permissions(TimelineModel *parent);

        Q_INVOKABLE bool canInvite();
        Q_INVOKABLE bool canBan();
        Q_INVOKABLE bool canKick();

        Q_INVOKABLE bool canRedact();
        Q_INVOKABLE bool canChange(int eventType);
        Q_INVOKABLE bool canSend(int eventType);

        void invalidate();

private:
        TimelineModel *room;
        mtx::events::state::PowerLevels pl;
};
