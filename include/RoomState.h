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

#include <mtx.hpp>

class RoomState
{
public:
        RoomState() = default;
        RoomState(const mtx::responses::Timeline &timeline);
        RoomState(const mtx::responses::State &state);

        // Calculate room data that are not immediatly accessible. Like room name and
        // avatar.
        //
        // e.g If the room is 1-on-1 name and avatar should be extracted from a user.
        void resolveName();
        void resolveAvatar();
        void parse(const nlohmann::json &object);

        QUrl getAvatar() const { return avatar_; };
        QString getName() const { return name_; };
        QString getTopic() const
        {
                return QString::fromStdString(topic.content.topic).simplified();
        };

        void removeLeaveMemberships();
        void update(const RoomState &state);

        template<class Collection>
        void updateFromEvents(const std::vector<Collection> &collection);

        std::string serialize() const;

        // The latest state events.
        mtx::events::StateEvent<mtx::events::state::Aliases> aliases;
        mtx::events::StateEvent<mtx::events::state::Avatar> avatar;
        mtx::events::StateEvent<mtx::events::state::CanonicalAlias> canonical_alias;
        mtx::events::StateEvent<mtx::events::state::Create> create;
        mtx::events::StateEvent<mtx::events::state::HistoryVisibility> history_visibility;
        mtx::events::StateEvent<mtx::events::state::JoinRules> join_rules;
        mtx::events::StateEvent<mtx::events::state::Name> name;
        mtx::events::StateEvent<mtx::events::state::PowerLevels> power_levels;
        mtx::events::StateEvent<mtx::events::state::Topic> topic;

        // Contains the m.room.member events for all the joined users.
        using UserID = std::string;
        std::map<UserID, mtx::events::StateEvent<mtx::events::state::Member>> memberships;

private:
        QUrl avatar_;
        QString name_;

        // It defines the user whose avatar is used for the room. If the room has an
        // avatar event this should be empty.
        QString userAvatar_;
};

Q_DECLARE_METATYPE(RoomState)

template<class Collection>
void
RoomState::updateFromEvents(const std::vector<Collection> &collection)
{
        using Aliases           = mtx::events::StateEvent<mtx::events::state::Aliases>;
        using Avatar            = mtx::events::StateEvent<mtx::events::state::Avatar>;
        using CanonicalAlias    = mtx::events::StateEvent<mtx::events::state::CanonicalAlias>;
        using Create            = mtx::events::StateEvent<mtx::events::state::Create>;
        using HistoryVisibility = mtx::events::StateEvent<mtx::events::state::HistoryVisibility>;
        using JoinRules         = mtx::events::StateEvent<mtx::events::state::JoinRules>;
        using Member            = mtx::events::StateEvent<mtx::events::state::Member>;
        using Name              = mtx::events::StateEvent<mtx::events::state::Name>;
        using PowerLevels       = mtx::events::StateEvent<mtx::events::state::PowerLevels>;
        using Topic             = mtx::events::StateEvent<mtx::events::state::Topic>;

        for (const auto &event : collection) {
                if (mpark::holds_alternative<Aliases>(event)) {
                        this->aliases = mpark::get<Aliases>(event);
                } else if (mpark::holds_alternative<Avatar>(event)) {
                        this->avatar = mpark::get<Avatar>(event);
                } else if (mpark::holds_alternative<CanonicalAlias>(event)) {
                        this->canonical_alias = mpark::get<CanonicalAlias>(event);
                } else if (mpark::holds_alternative<Create>(event)) {
                        this->create = mpark::get<Create>(event);
                } else if (mpark::holds_alternative<HistoryVisibility>(event)) {
                        this->history_visibility = mpark::get<HistoryVisibility>(event);
                } else if (mpark::holds_alternative<JoinRules>(event)) {
                        this->join_rules = mpark::get<JoinRules>(event);
                } else if (mpark::holds_alternative<Name>(event)) {
                        this->name = mpark::get<Name>(event);
                } else if (mpark::holds_alternative<Member>(event)) {
                        auto membership = mpark::get<Member>(event);
                        this->memberships.emplace(membership.state_key, membership);
                } else if (mpark::holds_alternative<PowerLevels>(event)) {
                        this->power_levels = mpark::get<PowerLevels>(event);
                } else if (mpark::holds_alternative<Topic>(event)) {
                        this->topic = mpark::get<Topic>(event);
                }
        }
}
