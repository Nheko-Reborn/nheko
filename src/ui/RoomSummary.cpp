// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "RoomSummary.h"

#include <QMetaType>

#include "ChatPage.h"
#include "MatrixClient.h"

RoomSummary::RoomSummary(std::string roomIdOrAlias_,
                         std::vector<std::string> vias_,
                         QString r_,
                         QObject *p)
  : QObject(p)
  , roomIdOrAlias(std::move(roomIdOrAlias_))
  , vias(std::move(vias_))
  , reason_(std::move(r_))
{
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
              emit proxy->failed();
          } else {
              emit proxy->loaded(room_);
          }
      },
      vias);
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
