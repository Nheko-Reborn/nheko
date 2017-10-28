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

#include <QJsonArray>
#include <QSettings>

#include "RoomState.h"

namespace events = matrix::events;

RoomState::RoomState() {}
RoomState::RoomState(const QJsonArray &events) { updateFromEvents(events); }

void
RoomState::resolveName()
{
        name_ = "Empty Room";
        userAvatar_.clear();

        if (!name.content().name().isEmpty()) {
                name_ = name.content().name().simplified();
                return;
        }

        if (!canonical_alias.content().alias().isEmpty()) {
                name_ = canonical_alias.content().alias().simplified();
                return;
        }

        // FIXME: Doesn't follow the spec guidelines.
        if (aliases.content().aliases().size() != 0) {
                name_ = aliases.content().aliases()[0].simplified();
                return;
        }

        QSettings settings;
        auto user_id = settings.value("auth/user_id");

        // TODO: Display names should be sorted alphabetically.
        for (const auto membership : memberships) {
                if (membership.stateKey() == user_id)
                        continue;

                if (membership.content().membershipState() == events::Membership::Join) {
                        userAvatar_ = membership.stateKey();

                        if (membership.content().displayName().isEmpty())
                                name_ = membership.stateKey();
                        else
                                name_ = membership.content().displayName();

                        break;
                }
        }

        // TODO: pluralization
        if (memberships.size() > 2)
                name_ = QString("%1 and %2 others").arg(name_).arg(memberships.size());
}

void
RoomState::resolveAvatar()
{
        if (userAvatar_.isEmpty()) {
                avatar_ = avatar.content().url();
                return;
        }

        if (memberships.contains(userAvatar_)) {
                avatar_ = memberships[userAvatar_].content().avatarUrl();
        } else {
                qWarning() << "Setting room avatar from unknown user id" << userAvatar_;
        }
}

// Should be used only after initial sync.
void
RoomState::removeLeaveMemberships()
{
        for (auto it = memberships.begin(); it != memberships.end();) {
                if (it.value().content().membershipState() == events::Membership::Leave) {
                        it = memberships.erase(it);
                } else {
                        ++it;
                }
        }
}

void
RoomState::update(const RoomState &state)
{
        bool needsNameCalculation   = false;
        bool needsAvatarCalculation = false;

        if (aliases.eventId() != state.aliases.eventId()) {
                aliases = state.aliases;
        }

        if (avatar.eventId() != state.avatar.eventId()) {
                avatar                 = state.avatar;
                needsAvatarCalculation = true;
        }

        if (canonical_alias.eventId() != state.canonical_alias.eventId()) {
                canonical_alias      = state.canonical_alias;
                needsNameCalculation = true;
        }

        if (create.eventId() != state.create.eventId())
                create = state.create;
        if (history_visibility.eventId() != state.history_visibility.eventId())
                history_visibility = state.history_visibility;
        if (join_rules.eventId() != state.join_rules.eventId())
                join_rules = state.join_rules;

        if (name.eventId() != state.name.eventId()) {
                name                 = state.name;
                needsNameCalculation = true;
        }

        if (power_levels.eventId() != state.power_levels.eventId())
                power_levels = state.power_levels;
        if (topic.eventId() != state.topic.eventId())
                topic = state.topic;

        for (auto it = state.memberships.constBegin(); it != state.memberships.constEnd(); ++it) {
                auto membershipState = it.value().content().membershipState();

                if (it.key() == userAvatar_) {
                        needsNameCalculation   = true;
                        needsAvatarCalculation = true;
                }

                if (membershipState == events::Membership::Leave)
                        this->memberships.remove(it.key());
                else
                        this->memberships.insert(it.key(), it.value());
        }

        if (needsNameCalculation)
                resolveName();

        if (needsAvatarCalculation)
                resolveAvatar();
}

QJsonObject
RoomState::serialize() const
{
        QJsonObject obj;

        if (!aliases.eventId().isEmpty())
                obj["aliases"] = aliases.serialize();

        if (!avatar.eventId().isEmpty())
                obj["avatar"] = avatar.serialize();

        if (!canonical_alias.eventId().isEmpty())
                obj["canonical_alias"] = canonical_alias.serialize();

        if (!create.eventId().isEmpty())
                obj["create"] = create.serialize();

        if (!history_visibility.eventId().isEmpty())
                obj["history_visibility"] = history_visibility.serialize();

        if (!join_rules.eventId().isEmpty())
                obj["join_rules"] = join_rules.serialize();

        if (!name.eventId().isEmpty())
                obj["name"] = name.serialize();

        if (!power_levels.eventId().isEmpty())
                obj["power_levels"] = power_levels.serialize();

        if (!topic.eventId().isEmpty())
                obj["topic"] = topic.serialize();

        return obj;
}

void
RoomState::parse(const QJsonObject &object)
{
        // FIXME: Make this less versbose.

        if (object.contains("aliases")) {
                events::StateEvent<events::AliasesEventContent> event;

                try {
                        event.deserialize(object["aliases"]);
                        aliases = event;
                } catch (const DeserializationException &e) {
                        qWarning() << "RoomState::parse - aliases" << e.what();
                }
        }

        if (object.contains("avatar")) {
                events::StateEvent<events::AvatarEventContent> event;

                try {
                        event.deserialize(object["avatar"]);
                        avatar = event;
                } catch (const DeserializationException &e) {
                        qWarning() << "RoomState::parse - avatar" << e.what();
                }
        }

        if (object.contains("canonical_alias")) {
                events::StateEvent<events::CanonicalAliasEventContent> event;

                try {
                        event.deserialize(object["canonical_alias"]);
                        canonical_alias = event;
                } catch (const DeserializationException &e) {
                        qWarning() << "RoomState::parse - canonical_alias" << e.what();
                }
        }

        if (object.contains("create")) {
                events::StateEvent<events::CreateEventContent> event;

                try {
                        event.deserialize(object["create"]);
                        create = event;
                } catch (const DeserializationException &e) {
                        qWarning() << "RoomState::parse - create" << e.what();
                }
        }

        if (object.contains("history_visibility")) {
                events::StateEvent<events::HistoryVisibilityEventContent> event;

                try {
                        event.deserialize(object["history_visibility"]);
                        history_visibility = event;
                } catch (const DeserializationException &e) {
                        qWarning() << "RoomState::parse - history_visibility" << e.what();
                }
        }

        if (object.contains("join_rules")) {
                events::StateEvent<events::JoinRulesEventContent> event;

                try {
                        event.deserialize(object["join_rules"]);
                        join_rules = event;
                } catch (const DeserializationException &e) {
                        qWarning() << "RoomState::parse - join_rules" << e.what();
                }
        }

        if (object.contains("name")) {
                events::StateEvent<events::NameEventContent> event;

                try {
                        event.deserialize(object["name"]);
                        name = event;
                } catch (const DeserializationException &e) {
                        qWarning() << "RoomState::parse - name" << e.what();
                }
        }

        if (object.contains("power_levels")) {
                events::StateEvent<events::PowerLevelsEventContent> event;

                try {
                        event.deserialize(object["power_levels"]);
                        power_levels = event;
                } catch (const DeserializationException &e) {
                        qWarning() << "RoomState::parse - power_levels" << e.what();
                }
        }

        if (object.contains("topic")) {
                events::StateEvent<events::TopicEventContent> event;

                try {
                        event.deserialize(object["topic"]);
                        topic = event;
                } catch (const DeserializationException &e) {
                        qWarning() << "RoomState::parse - topic" << e.what();
                }
        }
}

void
RoomState::updateFromEvents(const QJsonArray &events)
{
        events::EventType ty;

        for (const auto &event : events) {
                try {
                        ty = events::extractEventType(event.toObject());
                } catch (const DeserializationException &e) {
                        qWarning() << e.what() << event;
                        continue;
                }

                if (!events::isStateEvent(ty))
                        continue;

                try {
                        switch (ty) {
                        case events::EventType::RoomAliases: {
                                events::StateEvent<events::AliasesEventContent> aliases;
                                aliases.deserialize(event);
                                this->aliases = aliases;
                                break;
                        }
                        case events::EventType::RoomAvatar: {
                                events::StateEvent<events::AvatarEventContent> avatar;
                                avatar.deserialize(event);
                                this->avatar = avatar;
                                break;
                        }
                        case events::EventType::RoomCanonicalAlias: {
                                events::StateEvent<events::CanonicalAliasEventContent>
                                  canonical_alias;
                                canonical_alias.deserialize(event);
                                this->canonical_alias = canonical_alias;
                                break;
                        }
                        case events::EventType::RoomCreate: {
                                events::StateEvent<events::CreateEventContent> create;
                                create.deserialize(event);
                                this->create = create;
                                break;
                        }
                        case events::EventType::RoomHistoryVisibility: {
                                events::StateEvent<events::HistoryVisibilityEventContent>
                                  history_visibility;
                                history_visibility.deserialize(event);
                                this->history_visibility = history_visibility;
                                break;
                        }
                        case events::EventType::RoomJoinRules: {
                                events::StateEvent<events::JoinRulesEventContent> join_rules;
                                join_rules.deserialize(event);
                                this->join_rules = join_rules;
                                break;
                        }
                        case events::EventType::RoomName: {
                                events::StateEvent<events::NameEventContent> name;
                                name.deserialize(event);
                                this->name = name;
                                break;
                        }
                        case events::EventType::RoomMember: {
                                events::StateEvent<events::MemberEventContent> member;
                                member.deserialize(event);

                                this->memberships.insert(member.stateKey(), member);

                                break;
                        }
                        case events::EventType::RoomPowerLevels: {
                                events::StateEvent<events::PowerLevelsEventContent> power_levels;
                                power_levels.deserialize(event);
                                this->power_levels = power_levels;
                                break;
                        }
                        case events::EventType::RoomTopic: {
                                events::StateEvent<events::TopicEventContent> topic;
                                topic.deserialize(event);
                                this->topic = topic;
                                break;
                        }
                        default: {
                                continue;
                        }
                        }
                } catch (const DeserializationException &e) {
                        qWarning() << e.what() << event;
                        continue;
                }
        }
}
