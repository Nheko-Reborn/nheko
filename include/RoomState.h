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

#include <QJsonDocument>
#include <QPixmap>
#include <QUrl>

#include "AliasesEventContent.h"
#include "AvatarEventContent.h"
#include "CanonicalAliasEventContent.h"
#include "CreateEventContent.h"
#include "HistoryVisibilityEventContent.h"
#include "JoinRulesEventContent.h"
#include "MemberEventContent.h"
#include "NameEventContent.h"
#include "PowerLevelsEventContent.h"
#include "TopicEventContent.h"

#include "Event.h"
#include "RoomEvent.h"
#include "StateEvent.h"

namespace events = matrix::events;

class RoomState
{
public:
        // Calculate room data that are not immediatly accessible. Like room name and
        // avatar.
        //
        // e.g If the room is 1-on-1 name and avatar should be extracted from a user.
        void resolveName();
        void resolveAvatar();
        void parse(const QJsonObject &object);

        QUrl getAvatar() const { return avatar_; };
        QString getName() const { return name_; };
        QString getTopic() const { return topic.content().topic().simplified(); };

        void removeLeaveMemberships();
        void update(const RoomState &state);
        void updateFromEvents(const QJsonArray &events);

        QJsonObject serialize() const;

        // The latest state events.
        events::StateEvent<events::AliasesEventContent> aliases;
        events::StateEvent<events::AvatarEventContent> avatar;
        events::StateEvent<events::CanonicalAliasEventContent> canonical_alias;
        events::StateEvent<events::CreateEventContent> create;
        events::StateEvent<events::HistoryVisibilityEventContent> history_visibility;
        events::StateEvent<events::JoinRulesEventContent> join_rules;
        events::StateEvent<events::NameEventContent> name;
        events::StateEvent<events::PowerLevelsEventContent> power_levels;
        events::StateEvent<events::TopicEventContent> topic;

        // Contains the m.room.member events for all the joined users.
        QMap<QString, events::StateEvent<events::MemberEventContent>> memberships;

private:
        QUrl avatar_;
        QString name_;

        // It defines the user whose avatar is used for the room. If the room has an
        // avatar event this should be empty.
        QString userAvatar_;
};
