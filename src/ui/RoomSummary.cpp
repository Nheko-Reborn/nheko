// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "RoomSummary.h"

#include <QMetaType>

#include "Cache.h"
#include "ChatPage.h"
#include "Logging.h"
#include "MatrixClient.h"
#include "Utils.h"

RoomSummary::RoomSummary(std::string roomIdOrAlias_,
                         std::vector<std::string> vias_,
                         QString r_,
                         QObject *p)
  : QObject(p)
  , roomIdOrAlias(std::move(roomIdOrAlias_))
  , vias(std::move(vias_))
  , reason_(std::move(r_))
{
    if (roomIdOrAlias.empty())
        return;

    if (roomIdOrAlias[0] == '!') {
        auto temp = cache::singleRoomInfo(roomIdOrAlias);

        if (temp.member_count) {
            mtx::responses::PublicRoom newInfo{};
            // newInfo.aliases;
            // newInfo.canonical_alias = "";
            newInfo.name               = temp.name;
            newInfo.room_id            = roomIdOrAlias;
            newInfo.topic              = temp.topic;
            newInfo.num_joined_members = temp.member_count;
            // newInfo.world_readable;
            newInfo.guest_can_join = temp.guest_access;
            newInfo.avatar_url     = temp.avatar_url;

            newInfo.join_rule    = temp.join_rule;
            newInfo.room_type    = temp.is_space ? mtx::events::state::room_type::space : "";
            newInfo.room_version = temp.version;
            newInfo.membership   = mtx::events::state::Membership::Join;
            // newInfo.encryption;

            this->room = std::move(newInfo);
            loaded_    = true;
            return;
        }

        // newInfo.encryption;
    }

    auto ctx = std::make_shared<RoomSummaryProxy>();

    connect(ctx.get(), &RoomSummaryProxy::failed, this, [this]() {
        loaded_ = true;
        emit loaded();
    });
    connect(
      ctx.get(), &RoomSummaryProxy::loaded, this, [this](const mtx::responses::PublicRoom &resp) {
          loaded_ = true;
          room    = resp;
          emit loaded();
      });

    http::client()->get_summary(
      roomIdOrAlias,
      [proxy = std::move(ctx)](const mtx::responses::PublicRoom &room_, mtx::http::RequestErr e) {
          if (e) {
              nhlog::net()->warn("RoomSummary failed to load: {}", *e);
              emit proxy->failed();
          } else {
              nhlog::net()->info("RoomSummary loaded. Avatar URL: {}", room_.avatar_url);
              emit proxy->loaded(room_);
          }
      },
      vias);
}

QString
RoomSummary::roomName() const
{
    return utils::replaceEmoji(
      QString::fromStdString(room ? room->name : roomIdOrAlias).toHtmlEscaped());
}
QString
RoomSummary::roomTopic() const
{
    return room ? utils::replaceEmoji(
                    utils::linkifyMessage(QString::fromStdString(room->topic)
                                            .toHtmlEscaped()
                                            .replace(QLatin1String("\n"), QLatin1String("<br>"))))
                : "";
}

void
RoomSummary::join()
{
    if (isKnockOnly())
        ChatPage::instance()->knockRoom(
          QString::fromStdString(roomIdOrAlias), vias, reason_, false, false);
    else
        ChatPage::instance()->joinRoomVia(roomIdOrAlias, vias, false, reason_);
}

void
RoomSummary::promptJoin()
{
    if (isKnockOnly())
        ChatPage::instance()->knockRoom(
          QString::fromStdString(roomIdOrAlias), vias, reason_, false, true);
    else
        ChatPage::instance()->joinRoomVia(roomIdOrAlias, vias, true, reason_);
}

#include "moc_RoomSummary.cpp"
