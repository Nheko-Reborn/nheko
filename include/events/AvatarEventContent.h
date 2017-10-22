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
#include <QUrl>

#include "Deserializable.h"

namespace matrix {
namespace events {
/*
 * A picture that is associated with the room.
 */

class AvatarEventContent
  : public Deserializable
  , public Serializable
{
public:
        void deserialize(const QJsonValue &data) override;
        QJsonObject serialize() const override;

        QUrl url() const { return url_; };

private:
        QUrl url_;
};

} // namespace events
} // namespace matrix
