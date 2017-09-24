#include <gtest/gtest.h>

#include <QJsonArray>
#include <QJsonObject>

#include "MessageEvent.h"
#include "MessageEventContent.h"

#include "Audio.h"
#include "Emote.h"
#include "File.h"
#include "Image.h"
#include "Location.h"
#include "Notice.h"
#include "Text.h"
#include "Video.h"

using namespace matrix::events;

TEST(MessageEvent, Audio)
{
        auto info =
          QJsonObject{ { "duration", 2140786 }, { "mimetype", "audio/mpeg" }, { "size", 1563688 } };

        auto content = QJsonObject{ { "body", "Bee Gees - Stayin' Alive" },
                                    { "msgtype", "m.audio" },
                                    { "url", "mxc://localhost/2sdfj23f33r3faad" },
                                    { "info", info } };

        auto event = QJsonObject{ { "content", content },
                                  { "event_id", "$asdfafdf8af:matrix.org" },
                                  { "room_id", "!aasdfaeae23r9:matrix.org" },
                                  { "sender", "@alice:matrix.org" },
                                  { "origin_server_ts", 1323238293289323LL },
                                  { "type", "m.room.message" } };

        MessageEvent<messages::Audio> audio;
        audio.deserialize(event);

        EXPECT_EQ(audio.msgContent().info().duration, 2140786);
        EXPECT_EQ(audio.msgContent().info().size, 1563688);
        EXPECT_EQ(audio.msgContent().info().mimetype, "audio/mpeg");
        EXPECT_EQ(audio.content().body(), "Bee Gees - Stayin' Alive");
}

TEST(MessageEvent, Emote)
{
        auto content = QJsonObject{ { "body", "emote message" }, { "msgtype", "m.emote" } };

        auto event = QJsonObject{ { "content", content },
                                  { "event_id", "$asdfafdf8af:matrix.org" },
                                  { "room_id", "!aasdfaeae23r9:matrix.org" },
                                  { "sender", "@alice:matrix.org" },
                                  { "origin_server_ts", 1323238293289323LL },
                                  { "type", "m.room.message" } };

        MessageEvent<messages::Emote> emote;
        emote.deserialize(event);

        EXPECT_EQ(emote.content().body(), "emote message");
}

TEST(MessageEvent, File)
{
        auto thumbnail_info = QJsonObject{
                { "h", 300 }, { "w", 400 }, { "size", 3432434 }, { "mimetype", "image/jpeg" }
        };

        auto file_info = QJsonObject{ { "size", 24242424 },
                                      { "mimetype", "application/msword" },
                                      { "thumbnail_url", "mxc://localhost/adfaefaFAFSDFF3" },
                                      { "thumbnail_info", thumbnail_info } };

        auto content = QJsonObject{ { "body", "something-important.doc" },
                                    { "filename", "something-important.doc" },
                                    { "url", "mxc://localhost/23d233d32r3r2r" },
                                    { "info", file_info },
                                    { "msgtype", "m.file" } };

        auto event = QJsonObject{ { "content", content },
                                  { "event_id", "$asdfafdf8af:matrix.org" },
                                  { "room_id", "!aasdfaeae23r9:matrix.org" },
                                  { "sender", "@alice:matrix.org" },
                                  { "origin_server_ts", 1323238293289323LL },
                                  { "type", "m.room.message" } };

        MessageEvent<messages::File> file;
        file.deserialize(event);

        EXPECT_EQ(file.content().body(), "something-important.doc");
        EXPECT_EQ(file.msgContent().info().thumbnail_info.h, 300);
        EXPECT_EQ(file.msgContent().info().thumbnail_info.w, 400);
        EXPECT_EQ(file.msgContent().info().thumbnail_info.mimetype, "image/jpeg");
        EXPECT_EQ(file.msgContent().info().mimetype, "application/msword");
        EXPECT_EQ(file.msgContent().info().size, 24242424);
        EXPECT_EQ(file.content().body(), "something-important.doc");
}

TEST(MessageEvent, Image)
{
        auto thumbinfo = QJsonObject{
                { "h", 11 }, { "w", 22 }, { "size", 212 }, { "mimetype", "img/jpeg" },
        };

        auto imginfo = QJsonObject{
                { "h", 110 },
                { "w", 220 },
                { "size", 2120 },
                { "mimetype", "img/jpeg" },
                { "thumbnail_url", "https://images.com/image-thumb.jpg" },
                { "thumbnail_info", thumbinfo },
        };

        auto content = QJsonObject{ { "body", "Image title" },
                                    { "msgtype", "m.image" },
                                    { "url", "https://images.com/image.jpg" },
                                    { "info", imginfo } };

        auto event = QJsonObject{ { "content", content },
                                  { "event_id", "$asdfafdf8af:matrix.org" },
                                  { "room_id", "!aasdfaeae23r9:matrix.org" },
                                  { "sender", "@alice:matrix.org" },
                                  { "origin_server_ts", 1323238293289323LL },
                                  { "type", "m.room.message" } };

        MessageEvent<messages::Image> img;
        img.deserialize(event);

        EXPECT_EQ(img.content().body(), "Image title");
        EXPECT_EQ(img.msgContent().info().h, 110);
        EXPECT_EQ(img.msgContent().info().w, 220);
        EXPECT_EQ(img.msgContent().info().thumbnail_info.w, 22);
        EXPECT_EQ(img.msgContent().info().mimetype, "img/jpeg");
        EXPECT_EQ(img.msgContent().info().thumbnail_url, "https://images.com/image-thumb.jpg");
}

TEST(MessageEvent, Location)
{
        auto thumbnail_info = QJsonObject{
                { "h", 300 }, { "w", 400 }, { "size", 3432434 }, { "mimetype", "image/jpeg" }
        };

        auto info = QJsonObject{ { "thumbnail_url", "mxc://localhost/adfaefaFAFSDFF3" },
                                 { "thumbnail_info", thumbnail_info } };

        auto content = QJsonObject{ { "body", "Big Ben, London, UK" },
                                    { "geo_uri", "geo:51.5008,0.1247" },
                                    { "info", info },
                                    { "msgtype", "m.location" } };

        auto event = QJsonObject{ { "content", content },
                                  { "event_id", "$asdfafdf8af:matrix.org" },
                                  { "room_id", "!aasdfaeae23r9:matrix.org" },
                                  { "sender", "@alice:matrix.org" },
                                  { "origin_server_ts", 1323238293289323LL },
                                  { "type", "m.room.message" } };

        MessageEvent<messages::Location> location;
        location.deserialize(event);

        EXPECT_EQ(location.msgContent().info().thumbnail_info.h, 300);
        EXPECT_EQ(location.msgContent().info().thumbnail_info.w, 400);
        EXPECT_EQ(location.msgContent().info().thumbnail_info.mimetype, "image/jpeg");
        EXPECT_EQ(location.msgContent().info().thumbnail_url, "mxc://localhost/adfaefaFAFSDFF3");
        EXPECT_EQ(location.content().body(), "Big Ben, London, UK");
}

TEST(MessageEvent, Notice)
{
        auto content = QJsonObject{ { "body", "notice message" }, { "msgtype", "m.notice" } };

        auto event = QJsonObject{ { "content", content },
                                  { "event_id", "$asdfafdf8af:matrix.org" },
                                  { "room_id", "!aasdfaeae23r9:matrix.org" },
                                  { "sender", "@alice:matrix.org" },
                                  { "origin_server_ts", 1323238293289323LL },
                                  { "type", "m.room.message" } };

        MessageEvent<messages::Notice> notice;
        notice.deserialize(event);

        EXPECT_EQ(notice.content().body(), "notice message");
}

TEST(MessageEvent, Text)
{
        auto content = QJsonObject{ { "body", "text message" }, { "msgtype", "m.text" } };

        auto event = QJsonObject{ { "content", content },
                                  { "event_id", "$asdfafdf8af:matrix.org" },
                                  { "room_id", "!aasdfaeae23r9:matrix.org" },
                                  { "sender", "@alice:matrix.org" },
                                  { "origin_server_ts", 1323238293289323LL },
                                  { "type", "m.room.message" } };

        MessageEvent<messages::Text> text;
        text.deserialize(event);

        EXPECT_EQ(text.content().body(), "text message");
}

TEST(MessageEvent, Video)
{
        auto thumbnail_info = QJsonObject{
                { "h", 300 }, { "w", 400 }, { "size", 3432434 }, { "mimetype", "image/jpeg" }
        };

        auto video_info = QJsonObject{ { "h", 222 },
                                       { "w", 333 },
                                       { "duration", 232323 },
                                       { "size", 24242424 },
                                       { "mimetype", "video/mp4" },
                                       { "thumbnail_url", "mxc://localhost/adfaefaFAFSDFF3" },
                                       { "thumbnail_info", thumbnail_info } };

        auto content = QJsonObject{ { "body", "Gangnam Style" },
                                    { "url", "mxc://localhost/23d233d32r3r2r" },
                                    { "info", video_info },
                                    { "msgtype", "m.video" } };

        auto event = QJsonObject{ { "content", content },
                                  { "event_id", "$asdfafdf8af:matrix.org" },
                                  { "room_id", "!aasdfaeae23r9:matrix.org" },
                                  { "sender", "@alice:matrix.org" },
                                  { "origin_server_ts", 1323238293289323LL },
                                  { "type", "m.room.message" } };

        MessageEvent<messages::Video> video;
        video.deserialize(event);

        EXPECT_EQ(video.msgContent().info().thumbnail_info.h, 300);
        EXPECT_EQ(video.msgContent().info().thumbnail_info.w, 400);
        EXPECT_EQ(video.msgContent().info().thumbnail_info.mimetype, "image/jpeg");
        EXPECT_EQ(video.msgContent().info().duration, 232323);
        EXPECT_EQ(video.msgContent().info().size, 24242424);
        EXPECT_EQ(video.msgContent().info().mimetype, "video/mp4");
        EXPECT_EQ(video.content().body(), "Gangnam Style");
}

TEST(MessageEvent, Types)
{
        EXPECT_EQ(
          extractMessageEventType(QJsonObject{
            { "content", QJsonObject{ { "msgtype", "m.audio" } } }, { "type", "m.room.message" },
          }),
          MessageEventType::Audio);
        EXPECT_EQ(
          extractMessageEventType(QJsonObject{
            { "content", QJsonObject{ { "msgtype", "m.emote" } } }, { "type", "m.room.message" },
          }),
          MessageEventType::Emote);
        EXPECT_EQ(
          extractMessageEventType(QJsonObject{
            { "content", QJsonObject{ { "msgtype", "m.file" } } }, { "type", "m.room.message" },
          }),
          MessageEventType::File);
        EXPECT_EQ(
          extractMessageEventType(QJsonObject{
            { "content", QJsonObject{ { "msgtype", "m.image" } } }, { "type", "m.room.message" },
          }),
          MessageEventType::Image);
        EXPECT_EQ(
          extractMessageEventType(QJsonObject{
            { "content", QJsonObject{ { "msgtype", "m.location" } } }, { "type", "m.room.message" },
          }),
          MessageEventType::Location);
        EXPECT_EQ(
          extractMessageEventType(QJsonObject{
            { "content", QJsonObject{ { "msgtype", "m.notice" } } }, { "type", "m.room.message" },
          }),
          MessageEventType::Notice);
        EXPECT_EQ(
          extractMessageEventType(QJsonObject{
            { "content", QJsonObject{ { "msgtype", "m.text" } } }, { "type", "m.room.message" },
          }),
          MessageEventType::Text);
        EXPECT_EQ(
          extractMessageEventType(QJsonObject{
            { "content", QJsonObject{ { "msgtype", "m.video" } } }, { "type", "m.room.message" },
          }),
          MessageEventType::Video);
        EXPECT_EQ(
          extractMessageEventType(QJsonObject{
            { "content", QJsonObject{ { "msgtype", "m.random" } } }, { "type", "m.room.message" },
          }),
          MessageEventType::Unknown);
}
