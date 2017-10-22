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

#include <QJsonValue>

#include "Deserializable.h"

namespace matrix {
namespace events {
enum class JoinRule
{
        // A user who wishes to join the room must first receive
        // an invite to the room from someone already inside of the room.
        Invite,

        // Reserved but not yet implemented by the Matrix specification.
        Knock,

        // Reserved but not yet implemented by the Matrix specification.
        Private,

        /// Anyone can join the room without any prior action.
        Public,
};

/*
 * Describes how users are allowed to join the room.
 */

class JoinRulesEventContent
  : public Deserializable
  , public Serializable
{
public:
        void deserialize(const QJsonValue &data) override;
        QJsonObject serialize() const override;

        JoinRule joinRule() const { return join_rule_; };

private:
        JoinRule join_rule_;
};

} // namespace events
} // namespace matrix
