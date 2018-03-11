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

#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <QSettings>

#include "RoomState.h"

RoomState::RoomState(const mtx::responses::Timeline &timeline)
{
        updateFromEvents(timeline.events);
}
RoomState::RoomState(const mtx::responses::State &state) { updateFromEvents(state.events); }

void
RoomState::resolveName()
{
        name_ = "Empty Room";
        userAvatar_.clear();

        if (!name.content.name.empty()) {
                name_ = QString::fromStdString(name.content.name).simplified();
                return;
        }

        if (!canonical_alias.content.alias.empty()) {
                name_ = QString::fromStdString(canonical_alias.content.alias).simplified();
                return;
        }

        // FIXME: Doesn't follow the spec guidelines.
        if (aliases.content.aliases.size() != 0) {
                name_ = QString::fromStdString(aliases.content.aliases[0]).simplified();
                return;
        }

        QSettings settings;
        auto user_id = settings.value("auth/user_id").toString();

        // TODO: Display names should be sorted alphabetically.
        for (const auto membership : memberships) {
                const auto stateKey = QString::fromStdString(membership.second.state_key);

                if (stateKey == user_id) {
                        name_ = QString::fromStdString(membership.second.content.display_name);

                        if (name_.isEmpty())
                                name_ = stateKey;

                        userAvatar_ = stateKey;

                        continue;
                }

                if (membership.second.content.membership == mtx::events::state::Membership::Join ||
                    membership.second.content.membership ==
                      mtx::events::state::Membership::Invite) {
                        userAvatar_ = stateKey;

                        name_ = QString::fromStdString(membership.second.content.display_name);

                        if (name_.isEmpty())
                                name_ = stateKey;

                        // TODO: pluralization
                        if (memberships.size() > 2)
                                name_ = QString("%1 and %2 others")
                                          .arg(name_)
                                          .arg(memberships.size() - 2);
                        break;
                }
        }
}

void
RoomState::resolveAvatar()
{
        if (userAvatar_.isEmpty()) {
                avatar_ = QString::fromStdString(avatar.content.url);
                return;
        }

        if (memberships.count(userAvatar_.toStdString()) != 0) {
                avatar_ =
                  QString::fromStdString(memberships[userAvatar_.toStdString()].content.avatar_url);
        } else {
                qWarning() << "Setting room avatar from unknown user id" << userAvatar_;
        }
}

// Should be used only after initial sync.
void
RoomState::removeLeaveMemberships()
{
        for (auto it = memberships.cbegin(); it != memberships.cend();) {
                if (it->second.content.membership == mtx::events::state::Membership::Leave) {
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

        if (aliases.event_id != state.aliases.event_id)
                aliases = state.aliases;

        if (avatar.event_id != state.avatar.event_id) {
                avatar                 = state.avatar;
                needsAvatarCalculation = true;
        }

        if (canonical_alias.event_id != state.canonical_alias.event_id) {
                canonical_alias      = state.canonical_alias;
                needsNameCalculation = true;
        }

        if (create.event_id != state.create.event_id)
                create = state.create;

        if (history_visibility.event_id != state.history_visibility.event_id)
                history_visibility = state.history_visibility;

        if (join_rules.event_id != state.join_rules.event_id)
                join_rules = state.join_rules;

        if (name.event_id != state.name.event_id) {
                name                 = state.name;
                needsNameCalculation = true;
        }

        if (power_levels.event_id != state.power_levels.event_id)
                power_levels = state.power_levels;

        if (topic.event_id != state.topic.event_id)
                topic = state.topic;

        for (auto it = state.memberships.cbegin(); it != state.memberships.cend(); ++it) {
                auto membershipState = it->second.content.membership;

                if (it->first == userAvatar_.toStdString()) {
                        needsNameCalculation   = true;
                        needsAvatarCalculation = true;
                }

                if (membershipState == mtx::events::state::Membership::Leave)
                        this->memberships.erase(it->first);
                else
                        this->memberships.emplace(it->first, it->second);
        }

        if (needsNameCalculation)
                resolveName();

        if (needsAvatarCalculation)
                resolveAvatar();
}

std::string
RoomState::serialize() const
{
        nlohmann::json obj;

        if (!aliases.event_id.empty())
                obj["aliases"] = aliases;

        if (!avatar.event_id.empty())
                obj["avatar"] = avatar;

        if (!canonical_alias.event_id.empty())
                obj["canonical_alias"] = canonical_alias;

        if (!create.event_id.empty())
                obj["create"] = create;

        if (!history_visibility.event_id.empty())
                obj["history_visibility"] = history_visibility;

        if (!join_rules.event_id.empty())
                obj["join_rules"] = join_rules;

        if (!name.event_id.empty())
                obj["name"] = name;

        if (!power_levels.event_id.empty())
                obj["power_levels"] = power_levels;

        if (!topic.event_id.empty())
                obj["topic"] = topic;

        return obj.dump();
}

void
RoomState::parse(const nlohmann::json &object)
{
        if (object.count("aliases") != 0) {
                try {
                        aliases = object.at("aliases")
                                    .get<mtx::events::StateEvent<mtx::events::state::Aliases>>();
                } catch (std::exception &e) {
                        qWarning() << "RoomState::parse - aliases" << e.what();
                }
        }

        if (object.count("avatar") != 0) {
                try {
                        avatar = object.at("avatar")
                                   .get<mtx::events::StateEvent<mtx::events::state::Avatar>>();
                } catch (std::exception &e) {
                        qWarning() << "RoomState::parse - avatar" << e.what();
                }
        }

        if (object.count("canonical_alias") != 0) {
                try {
                        canonical_alias =
                          object.at("canonical_alias")
                            .get<mtx::events::StateEvent<mtx::events::state::CanonicalAlias>>();
                } catch (std::exception &e) {
                        qWarning() << "RoomState::parse - canonical_alias" << e.what();
                }
        }

        if (object.count("create") != 0) {
                try {
                        create = object.at("create")
                                   .get<mtx::events::StateEvent<mtx::events::state::Create>>();
                } catch (std::exception &e) {
                        qWarning() << "RoomState::parse - create" << e.what();
                }
        }

        if (object.count("history_visibility") != 0) {
                try {
                        history_visibility =
                          object.at("history_visibility")
                            .get<mtx::events::StateEvent<mtx::events::state::HistoryVisibility>>();
                } catch (std::exception &e) {
                        qWarning() << "RoomState::parse - history_visibility" << e.what();
                }
        }

        if (object.count("join_rules") != 0) {
                try {
                        join_rules =
                          object.at("join_rules")
                            .get<mtx::events::StateEvent<mtx::events::state::JoinRules>>();
                } catch (std::exception &e) {
                        qWarning() << "RoomState::parse - join_rules" << e.what();
                }
        }

        if (object.count("name") != 0) {
                try {
                        name = object.at("name")
                                 .get<mtx::events::StateEvent<mtx::events::state::Name>>();
                } catch (std::exception &e) {
                        qWarning() << "RoomState::parse - name" << e.what();
                }
        }

        if (object.count("power_levels") != 0) {
                try {
                        power_levels =
                          object.at("power_levels")
                            .get<mtx::events::StateEvent<mtx::events::state::PowerLevels>>();
                } catch (std::exception &e) {
                        qWarning() << "RoomState::parse - power_levels" << e.what();
                }
        }

        if (object.count("topic") != 0) {
                try {
                        topic = object.at("topic")
                                  .get<mtx::events::StateEvent<mtx::events::state::Topic>>();
                } catch (std::exception &e) {
                        qWarning() << "RoomState::parse - topic" << e.what();
                }
        }
}
