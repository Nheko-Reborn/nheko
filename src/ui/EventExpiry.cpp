// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "EventExpiry.h"

#include "Cache_p.h"
#include "Logging.h"
#include "MainWindow.h"
#include "MatrixClient.h"
#include "timeline/TimelineModel.h"

void
EventExpiry::load()
{
    using namespace mtx::events;

    this->event = {};

    if (auto temp = cache::client()->getAccountData(mtx::events::EventType::NhekoEventExpiry, "")) {
        auto h = std::get<
          mtx::events::AccountDataEvent<mtx::events::account_data::nheko_extensions::EventExpiry>>(
          *temp);
        this->event = std::move(h.content);
    }

    if (!roomid_.isEmpty()) {
        if (auto temp = cache::client()->getAccountData(mtx::events::EventType::NhekoEventExpiry,
                                                        roomid_.toStdString())) {
            auto h      = std::get<mtx::events::AccountDataEvent<
                   mtx::events::account_data::nheko_extensions::EventExpiry>>(*temp);
            this->event = std::move(h.content);
        }
    }

    emit expireEventsAfterDaysChanged();
    emit expireEventsAfterCountChanged();
    emit protectLatestEventsChanged();
    emit expireStateEventsChanged();
}

void
EventExpiry::save()
{
    if (roomid_.isEmpty())
        http::client()->put_account_data(event, [](mtx::http::RequestErr e) {
            if (e) {
                nhlog::net()->error("Failed to set hidden events: {}", *e);
                MainWindow::instance()->showNotification(
                  tr("Failed to set hidden events: %1")
                    .arg(QString::fromStdString(e->matrix_error.error)));
            }
        });
    else
        http::client()->put_room_account_data(
          roomid_.toStdString(), event, [](mtx::http::RequestErr e) {
              if (e) {
                  nhlog::net()->error("Failed to set hidden events: {}", *e);
                  MainWindow::instance()->showNotification(
                    tr("Failed to set hidden events: %1")
                      .arg(QString::fromStdString(e->matrix_error.error)));
              }
          });
}

int
EventExpiry::expireEventsAfterDays() const
{
    return event.expire_after_ms / (1000 * 60 * 60 * 24);
}

int
EventExpiry::expireEventsAfterCount() const
{
    return event.keep_only_latest;
}

int
EventExpiry::protectLatestEvents() const
{
    return event.protect_latest;
}

bool
EventExpiry::expireStateEvents() const
{
    return !event.exclude_state_events;
}

void
EventExpiry::setExpireEventsAfterDays(int val)
{
    if (val > 0)
        this->event.expire_after_ms = std::uint64_t(val) * (1000 * 60 * 60 * 24);
    else
        this->event.expire_after_ms = 0;
    emit expireEventsAfterDaysChanged();
}

void
EventExpiry::setProtectLatestEvents(int val)
{
    if (val > 0)
        this->event.protect_latest = std::uint64_t(val);
    else
        this->event.expire_after_ms = 0;
    emit protectLatestEventsChanged();
}

void
EventExpiry::setExpireEventsAfterCount(int val)
{
    if (val > 0)
        this->event.keep_only_latest = std::uint64_t(val);
    else
        this->event.keep_only_latest = 0;
    emit expireEventsAfterCountChanged();
}

void
EventExpiry::setExpireStateEvents(bool val)
{
    this->event.exclude_state_events = !val;
    emit expireEventsAfterCountChanged();
}

#include "moc_EventExpiry.cpp"
