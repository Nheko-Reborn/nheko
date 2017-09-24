#include <gtest/gtest.h>

#include <QJsonArray>
#include <QJsonObject>

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

TEST(EventCollection, Deserialize)
{
        auto events = QJsonArray{
                QJsonObject{ { "content", QJsonObject{ { "name", "Name" } } },
                             { "event_id", "$asdfafdf8af:matrix.org" },
                             { "prev_content", QJsonObject{ { "name", "Previous Name" } } },
                             { "room_id", "!aasdfaeae23r9:matrix.org" },
                             { "sender", "@alice:matrix.org" },
                             { "origin_server_ts", 1323238293289323LL },
                             { "state_key", "" },
                             { "type", "m.room.name" } },
                QJsonObject{ { "content", QJsonObject{ { "topic", "Topic" } } },
                             { "event_id", "$asdfafdf8af:matrix.org" },
                             { "prev_content", QJsonObject{ { "topic", "Previous Topic" } } },
                             { "room_id", "!aasdfaeae23r9:matrix.org" },
                             { "state_key", "" },
                             { "sender", "@alice:matrix.org" },
                             { "origin_server_ts", 1323238293289323LL },
                             { "type", "m.room.topic" } },
        };

        for (const auto &event : events) {
                EventType ty = extractEventType(event.toObject());

                if (ty == EventType::RoomName) {
                        StateEvent<NameEventContent> name_event;
                        name_event.deserialize(event);

                        EXPECT_EQ(name_event.content().name(), "Name");
                        EXPECT_EQ(name_event.previousContent().name(), "Previous Name");
                } else if (ty == EventType::RoomTopic) {
                        StateEvent<TopicEventContent> topic_event;
                        topic_event.deserialize(event);

                        EXPECT_EQ(topic_event.content().topic(), "Topic");
                        EXPECT_EQ(topic_event.previousContent().topic(), "Previous Topic");
                } else {
                        ASSERT_EQ(false, true);
                }
        }
}

TEST(EventCollection, DeserializationException)
{
        // Using wrong event types.
        auto events = QJsonArray{
                QJsonObject{ { "content", QJsonObject{ { "name", "Name" } } },
                             { "event_id", "$asdfafdf8af:matrix.org" },
                             { "prev_content", QJsonObject{ { "name", "Previous Name" } } },
                             { "room_id", "!aasdfaeae23r9:matrix.org" },
                             { "sender", "@alice:matrix.org" },
                             { "origin_server_ts", 1323238293289323LL },
                             { "state_key", "" },
                             { "type", "m.room.topic" } },
                QJsonObject{ { "content", QJsonObject{ { "topic", "Topic" } } },
                             { "event_id", "$asdfafdf8af:matrix.org" },
                             { "prev_content", QJsonObject{ { "topic", "Previous Topic" } } },
                             { "room_id", "!aasdfaeae23r9:matrix.org" },
                             { "state_key", "" },
                             { "sender", "@alice:matrix.org" },
                             { "origin_server_ts", 1323238293289323LL },
                             { "type", "m.room.name" } },
        };

        for (const auto &event : events) {
                EventType ty = extractEventType(event.toObject());

                if (ty == EventType::RoomName) {
                        StateEvent<NameEventContent> name_event;

                        try {
                                name_event.deserialize(event);
                        } catch (const DeserializationException &e) {
                                ASSERT_STREQ("name key is missing", e.what());
                        }

                } else if (ty == EventType::RoomTopic) {
                        StateEvent<TopicEventContent> topic_event;

                        try {
                                topic_event.deserialize(event);
                        } catch (const DeserializationException &e) {
                                ASSERT_STREQ("topic key is missing", e.what());
                        }
                } else {
                        ASSERT_EQ(false, true);
                }
        }
}
