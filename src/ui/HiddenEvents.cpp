// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "HiddenEvents.h"

#include "Cache_p.h"
#include "MainWindow.h"
#include "MatrixClient.h"
#include "timeline/TimelineModel.h"

void
HiddenEvents::load()
{
    using namespace mtx::events;
    mtx::events::account_data::nheko_extensions::HiddenEvents hiddenEvents;
    hiddenEvents.hidden_event_types = std::vector{
      EventType::Reaction,
      EventType::CallCandidates,
      EventType::Unsupported,
    };

    if (auto temp =
          cache::client()->getAccountData(mtx::events::EventType::NhekoHiddenEvents, "")) {
        auto h = std::get<
          mtx::events::AccountDataEvent<mtx::events::account_data::nheko_extensions::HiddenEvents>>(
          *temp);
        if (h.content.hidden_event_types)
            hiddenEvents = std::move(h.content);
    }

    if (!roomid_.isEmpty()) {
        if (auto temp = cache::client()->getAccountData(mtx::events::EventType::NhekoHiddenEvents,
                                                        roomid_.toStdString())) {
            auto h = std::get<mtx::events::AccountDataEvent<
              mtx::events::account_data::nheko_extensions::HiddenEvents>>(*temp);
            if (h.content.hidden_event_types)
                hiddenEvents = std::move(h.content);
        }
    }

    hiddenEvents_.clear();
    hiddenEvents_ = std::move(hiddenEvents.hidden_event_types.value());
    emit hiddenEventsChanged();
}

Q_INVOKABLE void
HiddenEvents::toggle(int type)
{
    auto t = qml_mtx_events::fromRoomEventType(static_cast<qml_mtx_events::EventType>(type));
    if (auto it = std::find(begin(hiddenEvents_), end(hiddenEvents_), t); it != end(hiddenEvents_))
        hiddenEvents_.erase(it);
    else
        hiddenEvents_.push_back(t);
    emit hiddenEventsChanged();
}

QVariantList
HiddenEvents::hiddenEvents() const
{
    QVariantList l;
    for (const auto &e : hiddenEvents_) {
        l.push_back(qml_mtx_events::toRoomEventType(e));
    }

    return l;
}

void
HiddenEvents::save()
{
    mtx::events::account_data::nheko_extensions::HiddenEvents hiddenEvents;
    hiddenEvents.hidden_event_types = hiddenEvents_;

    if (roomid_.isEmpty())
        http::client()->put_account_data(hiddenEvents, [](mtx::http::RequestErr e) {
            if (e) {
                nhlog::net()->error("Failed to set hidden events: {}", *e);
                MainWindow::instance()->showNotification(
                  tr("Failed to set hidden events: %1")
                    .arg(QString::fromStdString(e->matrix_error.error)));
            }
        });
    else
        http::client()->put_room_account_data(
          roomid_.toStdString(), hiddenEvents, [](mtx::http::RequestErr e) {
              if (e) {
                  nhlog::net()->error("Failed to set hidden events: {}", *e);
                  MainWindow::instance()->showNotification(
                    tr("Failed to set hidden events: %1")
                      .arg(QString::fromStdString(e->matrix_error.error)));
              }
          });
}
