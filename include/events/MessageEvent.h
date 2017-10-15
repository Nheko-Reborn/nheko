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

#include "MessageEventContent.h"
#include "RoomEvent.h"

namespace matrix {
namespace events {
template<class MsgContent>
class MessageEvent : public RoomEvent<MessageEventContent>
{
public:
        inline MsgContent msgContent() const;

        void deserialize(const QJsonValue &data) override;

private:
        MsgContent msg_content_;
};

template<class MsgContent>
inline MsgContent
MessageEvent<MsgContent>::msgContent() const
{
        return msg_content_;
}

template<class MsgContent>
void
MessageEvent<MsgContent>::deserialize(const QJsonValue &data)
{
        RoomEvent<MessageEventContent>::deserialize(data);

        msg_content_.deserialize(data.toObject().value("content").toObject());
}

namespace messages {
struct ThumbnailInfo
{
        int h;
        int w;
        int size;

        QString mimetype;
};
} // namespace messages
} // namespace events
} // namespace matrix
