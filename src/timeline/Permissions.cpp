// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Permissions.h"

#include "Cache_p.h"
#include "MatrixClient.h"
#include "TimelineModel.h"

Permissions::Permissions(QString roomId, QObject *parent)
  : QObject(parent)
  , roomId_(std::move(roomId))
{
    invalidate();
}

void
Permissions::invalidate()
{
    pl = cache::client()
           ->getStateEvent<mtx::events::state::PowerLevels>(roomId_.toStdString())
           .value_or(mtx::events::StateEvent<mtx::events::state::PowerLevels>{})
           .content;
}

bool
Permissions::canInvite()
{
    return pl.user_level(http::client()->user_id().to_string()) >= pl.invite;
}

bool
Permissions::canBan()
{
    return pl.user_level(http::client()->user_id().to_string()) >= pl.ban;
}

bool
Permissions::canKick()
{
    return pl.user_level(http::client()->user_id().to_string()) >= pl.kick;
}

bool
Permissions::canRedact()
{
    return pl.user_level(http::client()->user_id().to_string()) >= pl.redact;
}
bool
Permissions::canChange(int eventType)
{
    return pl.user_level(http::client()->user_id().to_string()) >=
           pl.state_level(to_string(
             qml_mtx_events::fromRoomEventType(static_cast<qml_mtx_events::EventType>(eventType))));
}
bool
Permissions::canSend(int eventType)
{
    return pl.user_level(http::client()->user_id().to_string()) >=
           pl.event_level(to_string(
             qml_mtx_events::fromRoomEventType(static_cast<qml_mtx_events::EventType>(eventType))));
}

int
Permissions::defaultLevel()
{
    return pl.users_default;
}
int
Permissions::redactLevel()
{
    return pl.redact;
}
int
Permissions::changeLevel(int eventType)
{
    return pl.state_level(to_string(
      qml_mtx_events::fromRoomEventType(static_cast<qml_mtx_events::EventType>(eventType))));
}
int
Permissions::sendLevel(int eventType)
{
    return pl.event_level(to_string(
      qml_mtx_events::fromRoomEventType(static_cast<qml_mtx_events::EventType>(eventType))));
}

bool
Permissions::canPingRoom()
{
    return pl.user_level(http::client()->user_id().to_string()) >=
           pl.notification_level(mtx::events::state::notification_keys::room);
}
