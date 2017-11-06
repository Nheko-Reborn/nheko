#include <QDebug>
#include <QJsonArray>
#include <gtest/gtest.h>

#include "Event.h"
#include "RoomEvent.h"
#include "StateEvent.h"

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

using namespace matrix::events;

TEST(BaseEvent, Deserialization)
{
        // NameEventContent
        auto data =
          QJsonObject{{"content", QJsonObject{{"name", "Room Name"}}}, {"type", "m.room.name"}};

        Event<NameEventContent> name_event;
        name_event.deserialize(data);
        EXPECT_EQ(name_event.content().name(), "Room Name");
        EXPECT_EQ(name_event.serialize(), data);

        // TopicEventContent
        data = QJsonObject{{"content", QJsonObject{{"topic", "Room Topic"}}},
                           {"unsigned", QJsonObject{{"age", 22}, {"transaction_id", "randomid"}}},
                           {"type", "m.room.topic"}};

        Event<TopicEventContent> topic_event;
        topic_event.deserialize(data);
        EXPECT_EQ(topic_event.content().topic(), "Room Topic");
        EXPECT_EQ(topic_event.unsignedData().age(), 22);
        EXPECT_EQ(topic_event.unsignedData().transactionId(), "randomid");
        EXPECT_EQ(topic_event.serialize(), data);

        // AvatarEventContent
        data = QJsonObject{
          {"content", QJsonObject{{"url", "https://matrix.org"}}},
          {"unsigned", QJsonObject{{"age", 1343434343}, {"transaction_id", "m33434.33"}}},
          {"type", "m.room.avatar"}};

        Event<AvatarEventContent> avatar_event;
        avatar_event.deserialize(data);
        EXPECT_EQ(avatar_event.content().url().toString(), "https://matrix.org");
        EXPECT_EQ(avatar_event.unsignedData().age(), 1343434343);
        EXPECT_EQ(avatar_event.unsignedData().transactionId(), "m33434.33");
        EXPECT_EQ(avatar_event.serialize(), data);

        // AliasesEventContent
        data = QJsonObject{
          {"content",
           QJsonObject{{"aliases", QJsonArray{"#test:matrix.org", "#test2:matrix.org"}}}},
          {"unsigned", QJsonObject{{"transaction_id", "m33434.33"}}},
          {"type", "m.room.aliases"}};

        Event<AliasesEventContent> aliases_event;
        aliases_event.deserialize(data);
        EXPECT_EQ(aliases_event.content().aliases().size(), 2);
        EXPECT_EQ(aliases_event.unsignedData().transactionId(), "m33434.33");
        EXPECT_EQ(aliases_event.serialize(), data);

        // CreateEventContent
        data = QJsonObject{{"content", QJsonObject{{"creator", "@alice:matrix.org"}}},
                           {"unsigned", QJsonObject{{"age", 2233}}},
                           {"type", "m.room.create"}};

        Event<CreateEventContent> create_event;
        create_event.deserialize(data);
        EXPECT_EQ(create_event.content().creator(), "@alice:matrix.org");
        EXPECT_EQ(create_event.unsignedData().age(), 2233);
        EXPECT_EQ(create_event.serialize(), data);

        // JoinRulesEventContent
        data = QJsonObject{{"content", QJsonObject{{"join_rule", "private"}}},
                           {"type", "m.room.join_rules"}};

        Event<JoinRulesEventContent> join_rules_event;
        join_rules_event.deserialize(data);
        EXPECT_EQ(join_rules_event.content().joinRule(), JoinRule::Private);
        EXPECT_EQ(join_rules_event.serialize(), data);
}

TEST(BaseEvent, DeserializationException)
{
        auto data =
          QJsonObject{{"content", QJsonObject{{"rule", "private"}}}, {"type", "m.room.join_rules"}};

        Event<JoinRulesEventContent> event1;
        ASSERT_THROW(event1.deserialize(data), DeserializationException);

        data = QJsonObject{{"contents", QJsonObject{{"join_rule", "private"}}},
                           {"type", "m.room.join_rules"}};

        Event<JoinRulesEventContent> event2;
        ASSERT_THROW(event2.deserialize(data), DeserializationException);

        data = QJsonObject{{"contents", QJsonObject{{"join_rule", "private"}}},
                           {"unsigned", QJsonObject{{"age", "222"}}},
                           {"type", "m.room.join_rules"}};

        Event<JoinRulesEventContent> event3;
        ASSERT_THROW(event3.deserialize(data), DeserializationException);
}

TEST(RoomEvent, Deserialization)
{
        auto data = QJsonObject{{"content", QJsonObject{{"name", "Name"}}},
                                {"event_id", "$asdfafdf8af:matrix.org"},
                                {"room_id", "!aasdfaeae23r9:matrix.org"},
                                {"sender", "@alice:matrix.org"},
                                {"origin_server_ts", 1323238293289323LL},
                                {"type", "m.room.name"}};

        RoomEvent<NameEventContent> event;
        event.deserialize(data);

        EXPECT_EQ(event.eventId(), "$asdfafdf8af:matrix.org");
        EXPECT_EQ(event.roomId(), "!aasdfaeae23r9:matrix.org");
        EXPECT_EQ(event.sender(), "@alice:matrix.org");
        EXPECT_EQ(event.timestamp(), 1323238293289323);
        EXPECT_EQ(event.content().name(), "Name");
        EXPECT_EQ(event.serialize(), data);
}

TEST(RoomEvent, DeserializationException)
{
        auto data = QJsonObject{{"content", QJsonObject{{"name", "Name"}}},
                                {"event_id", "$asdfafdf8af:matrix.org"},
                                {"room_id", "!aasdfaeae23r9:matrix.org"},
                                {"origin_server_ts", 1323238293289323LL},
                                {"type", "m.room.name"}};

        RoomEvent<NameEventContent> event;

        try {
                event.deserialize(data);
        } catch (const DeserializationException &e) {
                ASSERT_STREQ("sender key is missing", e.what());
        }
}

TEST(StateEvent, Deserialization)
{
        auto data = QJsonObject{{"content", QJsonObject{{"name", "Name"}}},
                                {"event_id", "$asdfafdf8af:matrix.org"},
                                {"state_key", "some_state_key"},
                                {"prev_content", QJsonObject{{"name", "Previous Name"}}},
                                {"room_id", "!aasdfaeae23r9:matrix.org"},
                                {"sender", "@alice:matrix.org"},
                                {"origin_server_ts", 1323238293289323LL},
                                {"type", "m.room.name"}};

        StateEvent<NameEventContent> event;
        event.deserialize(data);

        EXPECT_EQ(event.eventId(), "$asdfafdf8af:matrix.org");
        EXPECT_EQ(event.roomId(), "!aasdfaeae23r9:matrix.org");
        EXPECT_EQ(event.sender(), "@alice:matrix.org");
        EXPECT_EQ(event.timestamp(), 1323238293289323);
        EXPECT_EQ(event.content().name(), "Name");
        EXPECT_EQ(event.stateKey(), "some_state_key");
        EXPECT_EQ(event.previousContent().name(), "Previous Name");
        EXPECT_EQ(event.serialize(), data);
}

TEST(StateEvent, DeserializationException)
{
        auto data = QJsonObject{{"content", QJsonObject{{"name", "Name"}}},
                                {"event_id", "$asdfafdf8af:matrix.org"},
                                {"prev_content", QJsonObject{{"name", "Previous Name"}}},
                                {"room_id", "!aasdfaeae23r9:matrix.org"},
                                {"sender", "@alice:matrix.org"},
                                {"origin_server_ts", 1323238293289323LL},
                                {"type", "m.room.name"}};

        StateEvent<NameEventContent> event;

        try {
                event.deserialize(data);
        } catch (const DeserializationException &e) {
                ASSERT_STREQ("state_key key is missing", e.what());
        }
}

TEST(EventType, Mapping)
{
        EXPECT_EQ(extractEventType(QJsonObject{{"type", "m.room.aliases"}}),
                  EventType::RoomAliases);
        EXPECT_EQ(extractEventType(QJsonObject{{"type", "m.room.avatar"}}), EventType::RoomAvatar);
        EXPECT_EQ(extractEventType(QJsonObject{{"type", "m.room.canonical_alias"}}),
                  EventType::RoomCanonicalAlias);
        EXPECT_EQ(extractEventType(QJsonObject{{"type", "m.room.create"}}), EventType::RoomCreate);
        EXPECT_EQ(extractEventType(QJsonObject{{"type", "m.room.history_visibility"}}),
                  EventType::RoomHistoryVisibility);
        EXPECT_EQ(extractEventType(QJsonObject{{"type", "m.room.join_rules"}}),
                  EventType::RoomJoinRules);
        EXPECT_EQ(extractEventType(QJsonObject{{"type", "m.room.member"}}), EventType::RoomMember);
        EXPECT_EQ(extractEventType(QJsonObject{{"type", "m.room.message"}}),
                  EventType::RoomMessage);
        EXPECT_EQ(extractEventType(QJsonObject{{"type", "m.room.name"}}), EventType::RoomName);
        EXPECT_EQ(extractEventType(QJsonObject{{"type", "m.room.power_levels"}}),
                  EventType::RoomPowerLevels);
        EXPECT_EQ(extractEventType(QJsonObject{{"type", "m.room.topic"}}), EventType::RoomTopic);
        EXPECT_EQ(extractEventType(QJsonObject{{"type", "m.room.unknown"}}),
                  EventType::Unsupported);
}

TEST(AliasesEventContent, Deserialization)
{
        auto data = QJsonObject{{"aliases", QJsonArray{"#test:matrix.org", "#test2:matrix.org"}}};

        AliasesEventContent content;
        content.deserialize(data);

        EXPECT_EQ(content.aliases().size(), 2);
        EXPECT_EQ(content.serialize(), data);
}

TEST(AliasesEventContent, NotAnObject)
{
        auto data = QJsonArray{"#test:matrix.org", "#test2:matrix.org"};

        AliasesEventContent content;
        ASSERT_THROW(content.deserialize(data), DeserializationException);
}

TEST(AliasesEventContent, MissingKey)
{
        auto data = QJsonObject{{"key", QJsonArray{"#test:matrix.org", "#test2:matrix.org"}}};

        AliasesEventContent content;
        ASSERT_THROW(content.deserialize(data), DeserializationException);

        try {
                content.deserialize(data);
        } catch (const DeserializationException &e) {
                ASSERT_STREQ("aliases key is missing", e.what());
        }
}

TEST(AvatarEventContent, Deserialization)
{
        auto data = QJsonObject{{"url", "https://matrix.org/avatar.png"}};

        AvatarEventContent content;
        content.deserialize(data);

        EXPECT_EQ(content.url().toString(), "https://matrix.org/avatar.png");
        EXPECT_EQ(content.serialize(), data);
}

TEST(AvatarEventContent, NotAnObject)
{
        auto data = QJsonArray{"key", "url"};

        AvatarEventContent content;
        ASSERT_THROW(content.deserialize(data), DeserializationException);
}

TEST(AvatarEventContent, MissingKey)
{
        auto data = QJsonObject{{"key", "https://matrix.org"}};

        AvatarEventContent content;
        ASSERT_THROW(content.deserialize(data), DeserializationException);

        try {
                content.deserialize(data);
        } catch (const DeserializationException &e) {
                ASSERT_STREQ("url key is missing", e.what());
        }
}

TEST(CreateEventContent, Deserialization)
{
        auto data = QJsonObject{{"creator", "@alice:matrix.org"}};

        CreateEventContent content;
        content.deserialize(data);

        EXPECT_EQ(content.creator(), "@alice:matrix.org");
        EXPECT_EQ(content.serialize(), data);
}

TEST(CreateEventContent, NotAnObject)
{
        auto data = QJsonArray{"creator", "alice"};

        CreateEventContent content;

        ASSERT_THROW(content.deserialize(data), DeserializationException);
}

TEST(CreateEventContent, MissingKey)
{
        auto data = QJsonObject{{"key", "@alice:matrix.org"}};

        CreateEventContent content;
        ASSERT_THROW(content.deserialize(data), DeserializationException);

        try {
                content.deserialize(data);
        } catch (const DeserializationException &e) {
                ASSERT_STREQ("creator key is missing", e.what());
        }
}

TEST(HistoryVisibilityEventContent, Deserialization)
{
        auto data = QJsonObject{{"history_visibility", "invited"}};

        HistoryVisibilityEventContent content;
        content.deserialize(data);
        EXPECT_EQ(content.historyVisibility(), HistoryVisibility::Invited);

        data = QJsonObject{{"history_visibility", "joined"}};

        content.deserialize(data);
        EXPECT_EQ(content.historyVisibility(), HistoryVisibility::Joined);

        data = QJsonObject{{"history_visibility", "shared"}};

        content.deserialize(data);
        EXPECT_EQ(content.historyVisibility(), HistoryVisibility::Shared);

        data = QJsonObject{{"history_visibility", "world_readable"}};

        content.deserialize(data);
        EXPECT_EQ(content.historyVisibility(), HistoryVisibility::WorldReadable);
}

TEST(HistoryVisibilityEventContent, NotAnObject)
{
        auto data = QJsonArray{"history_visibility", "alice"};

        HistoryVisibilityEventContent content;

        ASSERT_THROW(content.deserialize(data), DeserializationException);
}

TEST(HistoryVisibilityEventContent, InvalidHistoryVisibility)
{
        auto data = QJsonObject{{"history_visibility", "wrong"}};

        HistoryVisibilityEventContent content;
        ASSERT_THROW(content.deserialize(data), DeserializationException);

        try {
                content.deserialize(data);
        } catch (const DeserializationException &e) {
                ASSERT_STREQ("Unknown history_visibility value: wrong", e.what());
        }
}

TEST(HistoryVisibilityEventContent, MissingKey)
{
        auto data = QJsonObject{{"key", "joined"}};

        HistoryVisibilityEventContent content;
        ASSERT_THROW(content.deserialize(data), DeserializationException);

        try {
                content.deserialize(data);
        } catch (const DeserializationException &e) {
                ASSERT_STREQ("history_visibility key is missing", e.what());
        }
}

TEST(JoinRulesEventContent, Deserialization)
{
        auto data = QJsonObject{{"join_rule", "invite"}};

        JoinRulesEventContent content;
        content.deserialize(data);
        EXPECT_EQ(content.joinRule(), JoinRule::Invite);

        data = QJsonObject{{"join_rule", "knock"}};

        content.deserialize(data);
        EXPECT_EQ(content.joinRule(), JoinRule::Knock);

        data = QJsonObject{{"join_rule", "private"}};

        content.deserialize(data);
        EXPECT_EQ(content.joinRule(), JoinRule::Private);

        data = QJsonObject{{"join_rule", "public"}};

        content.deserialize(data);
        EXPECT_EQ(content.joinRule(), JoinRule::Public);
}

TEST(JoinRulesEventContent, NotAnObject)
{
        auto data = QJsonArray{"rule", "alice"};

        JoinRulesEventContent content;

        ASSERT_THROW(content.deserialize(data), DeserializationException);
}

TEST(JoinRulesEventContent, InvalidHistoryVisibility)
{
        auto data = QJsonObject{{"join_rule", "wrong"}};

        JoinRulesEventContent content;
        ASSERT_THROW(content.deserialize(data), DeserializationException);

        try {
                content.deserialize(data);
        } catch (const DeserializationException &e) {
                ASSERT_STREQ("Unknown join_rule value: wrong", e.what());
        }
}

TEST(JoinRulesEventContent, MissingKey)
{
        auto data = QJsonObject{{"key", "invite"}};

        JoinRulesEventContent content;
        ASSERT_THROW(content.deserialize(data), DeserializationException);

        try {
                content.deserialize(data);
        } catch (const DeserializationException &e) {
                ASSERT_STREQ("join_rule key is missing", e.what());
        }
}

TEST(CanonicalAliasEventContent, Deserialization)
{
        auto data = QJsonObject{{"alias", "Room Alias"}};

        CanonicalAliasEventContent content;
        content.deserialize(data);

        EXPECT_EQ(content.alias(), "Room Alias");
        EXPECT_EQ(content.serialize(), data);
}

TEST(CanonicalAliasEventContent, NotAnObject)
{
        auto data = QJsonArray{"alias", "Room Alias"};

        CanonicalAliasEventContent content;

        ASSERT_THROW(content.deserialize(data), DeserializationException);
}

TEST(CanonicalAliasEventContent, MissingKey)
{
        auto data = QJsonObject{{"key", "alias"}};

        CanonicalAliasEventContent content;
        ASSERT_THROW(content.deserialize(data), DeserializationException);

        try {
                content.deserialize(data);
        } catch (const DeserializationException &e) {
                ASSERT_STREQ("alias key is missing", e.what());
        }
}

TEST(MemberEventContent, Deserialization)
{
        MemberEventContent content;

        auto data = QJsonObject{{"membership", "join"}};

        content.deserialize(data);
        EXPECT_EQ(content.membershipState(), Membership::Join);

        data = QJsonObject{{"membership", "invite"}, {"displayname", "Username"}};

        content.deserialize(data);
        EXPECT_EQ(content.membershipState(), Membership::Invite);
        EXPECT_EQ(content.displayName(), "Username");

        data = QJsonObject{{"membership", "leave"}, {"avatar_url", "https://matrix.org"}};

        content.deserialize(data);
        EXPECT_EQ(content.membershipState(), Membership::Leave);
        EXPECT_EQ(content.avatarUrl().toString(), "https://matrix.org");

        data = QJsonObject{{"membership", "ban"}};

        content.deserialize(data);
        EXPECT_EQ(content.membershipState(), Membership::Ban);

        data = QJsonObject{{"membership", "knock"}};

        content.deserialize(data);
        EXPECT_EQ(content.membershipState(), Membership::Knock);
}

TEST(MemberEventContent, InvalidMembership)
{
        auto data = QJsonObject{{"membership", "wrong"}};

        MemberEventContent content;
        ASSERT_THROW(content.deserialize(data), DeserializationException);

        try {
                content.deserialize(data);
        } catch (const DeserializationException &e) {
                ASSERT_STREQ("Unknown membership value: wrong", e.what());
        }
}

TEST(MemberEventContent, NotAnObject)
{
        auto data = QJsonArray{"name", "join"};

        MemberEventContent content;

        ASSERT_THROW(content.deserialize(data), DeserializationException);
}

TEST(MemberEventContent, MissingName)
{
        auto data = QJsonObject{{"key", "random"}};

        MemberEventContent content;
        ASSERT_THROW(content.deserialize(data), DeserializationException);

        try {
                content.deserialize(data);
        } catch (const DeserializationException &e) {
                ASSERT_STREQ("membership key is missing", e.what());
        }
}

TEST(NameEventContent, Deserialization)
{
        auto data = QJsonObject{{"name", "Room Name"}};

        NameEventContent content;
        content.deserialize(data);

        EXPECT_EQ(content.name(), "Room Name");
        EXPECT_EQ(content.serialize(), data);
}

TEST(NameEventContent, NotAnObject)
{
        auto data = QJsonArray{"name", "Room Name"};

        NameEventContent content;

        ASSERT_THROW(content.deserialize(data), DeserializationException);
}

TEST(NameEventContent, MissingName)
{
        auto data = QJsonObject{{"key", "Room Name"}};

        NameEventContent content;
        ASSERT_THROW(content.deserialize(data), DeserializationException);

        try {
                content.deserialize(data);
        } catch (const DeserializationException &e) {
                ASSERT_STREQ("name key is missing", e.what());
        }
}

TEST(PowerLevelsEventContent, DefaultValues)
{
        PowerLevelsEventContent power_levels;

        EXPECT_EQ(power_levels.banLevel(), static_cast<int>(PowerLevels::Moderator));
        EXPECT_EQ(power_levels.inviteLevel(), static_cast<int>(PowerLevels::Moderator));
        EXPECT_EQ(power_levels.kickLevel(), static_cast<int>(PowerLevels::Moderator));
        EXPECT_EQ(power_levels.redactLevel(), static_cast<int>(PowerLevels::Moderator));

        EXPECT_EQ(power_levels.eventsDefaultLevel(), static_cast<int>(PowerLevels::User));
        EXPECT_EQ(power_levels.usersDefaultLevel(), static_cast<int>(PowerLevels::User));
        EXPECT_EQ(power_levels.stateDefaultLevel(), static_cast<int>(PowerLevels::Moderator));

        // Default levels.
        EXPECT_EQ(power_levels.userLevel("@joe:matrix.org"), static_cast<int>(PowerLevels::User));
        EXPECT_EQ(power_levels.eventLevel("m.room.message"), static_cast<int>(PowerLevels::User));
}

TEST(PowerLevelsEventContent, FullDeserialization)
{
        auto data = QJsonObject{
          {"ban", 1},
          {"invite", 2},
          {"kick", 3},
          {"redact", 4},

          {"events_default", 5},
          {"state_default", 6},
          {"users_default", 7},

          {"events", QJsonObject{{"m.message.text", 8}, {"m.message.image", 9}}},
          {"users", QJsonObject{{"@alice:matrix.org", 10}, {"@bob:matrix.org", 11}}},
        };

        PowerLevelsEventContent power_levels;
        power_levels.deserialize(data);

        EXPECT_EQ(power_levels.banLevel(), 1);
        EXPECT_EQ(power_levels.inviteLevel(), 2);
        EXPECT_EQ(power_levels.kickLevel(), 3);
        EXPECT_EQ(power_levels.redactLevel(), 4);

        EXPECT_EQ(power_levels.eventsDefaultLevel(), 5);
        EXPECT_EQ(power_levels.stateDefaultLevel(), 6);
        EXPECT_EQ(power_levels.usersDefaultLevel(), 7);

        EXPECT_EQ(power_levels.userLevel("@alice:matrix.org"), 10);
        EXPECT_EQ(power_levels.userLevel("@bob:matrix.org"), 11);
        EXPECT_EQ(power_levels.userLevel("@carl:matrix.org"), 7);

        EXPECT_EQ(power_levels.eventLevel("m.message.text"), 8);
        EXPECT_EQ(power_levels.eventLevel("m.message.image"), 9);
        EXPECT_EQ(power_levels.eventLevel("m.message.gif"), 5);

        EXPECT_EQ(power_levels.serialize(), data);
}

TEST(PowerLevelsEventContent, PartialDeserialization)
{
        auto data = QJsonObject{
          {"ban", 1},
          {"invite", 2},

          {"events_default", 5},
          {"users_default", 7},

          {"users", QJsonObject{{"@alice:matrix.org", 10}, {"@bob:matrix.org", 11}}},
        };

        PowerLevelsEventContent power_levels;
        power_levels.deserialize(data);

        EXPECT_EQ(power_levels.banLevel(), 1);
        EXPECT_EQ(power_levels.inviteLevel(), 2);
        EXPECT_EQ(power_levels.kickLevel(), static_cast<int>(PowerLevels::Moderator));
        EXPECT_EQ(power_levels.redactLevel(), static_cast<int>(PowerLevels::Moderator));

        EXPECT_EQ(power_levels.eventsDefaultLevel(), 5);
        EXPECT_EQ(power_levels.stateDefaultLevel(), static_cast<int>(PowerLevels::Moderator));
        EXPECT_EQ(power_levels.usersDefaultLevel(), 7);

        EXPECT_EQ(power_levels.userLevel("@alice:matrix.org"), 10);
        EXPECT_EQ(power_levels.userLevel("@bob:matrix.org"), 11);
        EXPECT_EQ(power_levels.userLevel("@carl:matrix.org"), 7);

        EXPECT_EQ(power_levels.eventLevel("m.message.text"), 5);
        EXPECT_EQ(power_levels.eventLevel("m.message.image"), 5);
        EXPECT_EQ(power_levels.eventLevel("m.message.gif"), 5);
}

TEST(PowerLevelsEventContent, NotAnObject)
{
        auto data = QJsonArray{"test", "test2"};

        PowerLevelsEventContent power_levels;

        ASSERT_THROW(power_levels.deserialize(data), DeserializationException);
}

TEST(TopicEventContent, Deserialization)
{
        auto data = QJsonObject{{"topic", "Room Topic"}};

        TopicEventContent content;
        content.deserialize(data);

        EXPECT_EQ(content.topic(), "Room Topic");
        EXPECT_EQ(content.serialize(), data);
}

TEST(TopicEventContent, NotAnObject)
{
        auto data = QJsonArray{"topic", "Room Topic"};

        TopicEventContent content;

        ASSERT_THROW(content.deserialize(data), DeserializationException);
}

TEST(TopicEventContent, MissingName)
{
        auto data = QJsonObject{{"key", "Room Name"}};

        TopicEventContent content;
        ASSERT_THROW(content.deserialize(data), DeserializationException);

        try {
                content.deserialize(data);
        } catch (const DeserializationException &e) {
                ASSERT_STREQ("topic key is missing", e.what());
        }
}
