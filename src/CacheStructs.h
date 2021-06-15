// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QDateTime>
#include <QImage>
#include <QString>

#include <string>

#include <mtx/events/join_rules.hpp>

namespace cache {
enum class CacheVersion : int
{
        Older   = -1,
        Current = 0,
        Newer   = 1,
};
}

struct RoomMember
{
        QString user_id;
        QString display_name;
};

//! Used to uniquely identify a list of read receipts.
struct ReadReceiptKey
{
        std::string event_id;
        std::string room_id;
};

void
to_json(nlohmann::json &j, const ReadReceiptKey &key);

void
from_json(const nlohmann::json &j, ReadReceiptKey &key);

struct DescInfo
{
        QString event_id;
        QString userid;
        QString body;
        QString descriptiveTime;
        uint64_t timestamp;
        QDateTime datetime;
};

inline bool
operator==(const DescInfo &a, const DescInfo &b)
{
        return std::tie(a.timestamp, a.event_id, a.userid, a.body, a.descriptiveTime) ==
               std::tie(b.timestamp, b.event_id, b.userid, b.body, b.descriptiveTime);
}
inline bool
operator!=(const DescInfo &a, const DescInfo &b)
{
        return std::tie(a.timestamp, a.event_id, a.userid, a.body, a.descriptiveTime) !=
               std::tie(b.timestamp, b.event_id, b.userid, b.body, b.descriptiveTime);
}

//! UI info associated with a room.
struct RoomInfo
{
        //! The calculated name of the room.
        std::string name;
        //! The topic of the room.
        std::string topic;
        //! The calculated avatar url of the room.
        std::string avatar_url;
        //! The calculated version of this room set at creation time.
        std::string version;
        //! Whether or not the room is an invite.
        bool is_invite = false;
        //! Wheter or not the room is a space
        bool is_space = false;
        //! Total number of members in the room.
        size_t member_count = 0;
        //! Who can access to the room.
        mtx::events::state::JoinRule join_rule = mtx::events::state::JoinRule::Public;
        bool guest_access                      = false;
        //! Metadata describing the last message in the timeline.
        DescInfo msgInfo;
        //! The list of tags associated with this room
        std::vector<std::string> tags;
};

void
to_json(nlohmann::json &j, const RoomInfo &info);
void
from_json(const nlohmann::json &j, RoomInfo &info);

//! Basic information per member;
struct MemberInfo
{
        std::string name;
        std::string avatar_url;
};

void
to_json(nlohmann::json &j, const MemberInfo &info);
void
from_json(const nlohmann::json &j, MemberInfo &info);

struct RoomSearchResult
{
        std::string room_id;
        RoomInfo info;
};
