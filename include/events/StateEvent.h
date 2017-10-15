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

#include "RoomEvent.h"

namespace matrix {
namespace events {
template<class Content>
class StateEvent : public RoomEvent<Content>
{
public:
        inline QString stateKey() const;
        inline Content previousContent() const;

        void deserialize(const QJsonValue &data);
        QJsonObject serialize() const;

private:
        QString state_key_;
        Content prev_content_;
};

template<class Content>
inline QString
StateEvent<Content>::stateKey() const
{
        return state_key_;
}

template<class Content>
inline Content
StateEvent<Content>::previousContent() const
{
        return prev_content_;
}

template<class Content>
void
StateEvent<Content>::deserialize(const QJsonValue &data)
{
        RoomEvent<Content>::deserialize(data);

        auto object = data.toObject();

        if (!object.contains("state_key"))
                throw DeserializationException("state_key key is missing");

        state_key_ = object.value("state_key").toString();

        if (object.contains("prev_content"))
                prev_content_.deserialize(object.value("prev_content"));
}

template<class Content>
QJsonObject
StateEvent<Content>::serialize() const
{
        QJsonObject object = RoomEvent<Content>::serialize();

        object["state_key"] = state_key_;

        auto prev = prev_content_.serialize();

        if (!prev.isEmpty())
                object["prev_content"] = prev;

        return object;
}
} // namespace events
} // namespace matrix
