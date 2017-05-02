#include <gtest/gtest.h>
#include <QJsonArray>

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

TEST(AliasesEventContent, Deserialization)
{
	auto data = QJsonObject{
		{"aliases", QJsonArray{"#test:matrix.org", "#test2:matrix.org"}}};

	AliasesEventContent content;
	content.deserialize(data);

	EXPECT_EQ(content.aliases().size(), 2);
}

TEST(AliasesEventContent, NotAnObject)
{
	auto data = QJsonArray{"#test:matrix.org", "#test2:matrix.org"};

	AliasesEventContent content;
	ASSERT_THROW(content.deserialize(data), DeserializationException);
}

TEST(AliasesEventContent, MissingKey)
{
	auto data = QJsonObject{
		{"key", QJsonArray{"#test:matrix.org", "#test2:matrix.org"}}};

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
	EXPECT_EQ(content.membershipState(), Membership::JoinState);

	data = QJsonObject{{"membership", "invite"}, {"displayname", "Username"}};

	content.deserialize(data);
	EXPECT_EQ(content.membershipState(), Membership::InviteState);
	EXPECT_EQ(content.displayName(), "Username");

	data = QJsonObject{{"membership", "leave"}, {"avatar_url", "https://matrix.org"}};

	content.deserialize(data);
	EXPECT_EQ(content.membershipState(), Membership::LeaveState);
	EXPECT_EQ(content.avatarUrl().toString(), "https://matrix.org");

	data = QJsonObject{{"membership", "ban"}};

	content.deserialize(data);
	EXPECT_EQ(content.membershipState(), Membership::BanState);

	data = QJsonObject{{"membership", "knock"}};

	content.deserialize(data);
	EXPECT_EQ(content.membershipState(), Membership::KnockState);
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

	EXPECT_EQ(power_levels.banLevel(), PowerLevels::Moderator);
	EXPECT_EQ(power_levels.inviteLevel(), PowerLevels::Moderator);
	EXPECT_EQ(power_levels.kickLevel(), PowerLevels::Moderator);
	EXPECT_EQ(power_levels.redactLevel(), PowerLevels::Moderator);

	EXPECT_EQ(power_levels.eventsDefaultLevel(), PowerLevels::User);
	EXPECT_EQ(power_levels.usersDefaultLevel(), PowerLevels::User);
	EXPECT_EQ(power_levels.stateDefaultLevel(), PowerLevels::Moderator);

	// Default levels.
	EXPECT_EQ(power_levels.userLevel("@joe:matrix.org"), PowerLevels::User);
	EXPECT_EQ(power_levels.eventLevel("m.room.message"), PowerLevels::User);
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
	EXPECT_EQ(power_levels.kickLevel(), PowerLevels::Moderator);
	EXPECT_EQ(power_levels.redactLevel(), PowerLevels::Moderator);

	EXPECT_EQ(power_levels.eventsDefaultLevel(), 5);
	EXPECT_EQ(power_levels.stateDefaultLevel(), PowerLevels::Moderator);
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
