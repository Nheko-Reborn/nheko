// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "EventStore.h"

#include <QThread>
#include <QTimer>

#include <mtx/responses/common.hpp>

#include "Cache.h"
#include "Cache_p.h"
#include "ChatPage.h"
#include "EventAccessors.h"
#include "Logging.h"
#include "MatrixClient.h"
#include "UserSettingsPage.h"
#include "Utils.h"

Q_DECLARE_METATYPE(Reaction)

QCache<EventStore::IdIndex, olm::DecryptionResult> EventStore::decryptedEvents_{1000};
QCache<EventStore::IdIndex, mtx::events::collections::TimelineEvents> EventStore::events_by_id_{
  1000};
QCache<EventStore::Index, mtx::events::collections::TimelineEvents> EventStore::events_{1000};

EventStore::EventStore(std::string room_id, QObject *)
  : room_id_(std::move(room_id))
{
    static auto reactionType = qRegisterMetaType<Reaction>();
    (void)reactionType;

    auto range = cache::client()->getTimelineRange(room_id_);

    if (range) {
        this->first = range->first;
        this->last  = range->last;
    }

    connect(
      this,
      &EventStore::eventFetched,
      this,
      [this](
        std::string id, std::string relatedTo, mtx::events::collections::TimelineEvents timeline) {
          cache::client()->storeEvent(room_id_, id, {timeline});

          if (!relatedTo.empty()) {
              if (relatedTo == "pins") {
                  emit pinsChanged();
              } else {
                  auto idx = idToIndex(relatedTo);
                  if (idx)
                      emit dataChanged(*idx, *idx);
              }
          }
      },
      Qt::QueuedConnection);

    connect(
      this,
      &EventStore::oldMessagesRetrieved,
      this,
      [this](const mtx::responses::Messages &res) {
          if (res.end.empty() || cache::client()->previousBatchToken(room_id_) == res.end) {
              noMoreMessages = true;
              emit fetchedMore();
              return;
          }

          uint64_t newFirst = cache::client()->saveOldMessages(room_id_, res);
          if (newFirst == first)
              fetchMore();
          else {
              if (this->last != std::numeric_limits<uint64_t>::max()) {
                  auto oldFirst = this->first;
                  emit beginInsertRows(toExternalIdx(newFirst), toExternalIdx(this->first - 1));
                  this->first = newFirst;
                  emit endInsertRows();
                  emit fetchedMore();
                  emit dataChanged(toExternalIdx(oldFirst), toExternalIdx(oldFirst));
              } else {
                  auto range = cache::client()->getTimelineRange(room_id_);

                  if (range && range->last - range->first != 0) {
                      emit beginInsertRows(0, int(range->last - range->first));
                      this->first = range->first;
                      this->last  = range->last;
                      emit endInsertRows();
                      emit fetchedMore();
                  } else {
                      fetchMore();
                  }
              }
          }
      },
      Qt::QueuedConnection);

    connect(this, &EventStore::processPending, this, [this]() {
        if (!current_txn.empty()) {
            nhlog::ui()->debug("Already processing {}", current_txn);
            return;
        }

        auto event = cache::client()->firstPendingMessage(room_id_);

        if (!event) {
            nhlog::ui()->debug("No event to send");
            return;
        }

        std::visit(
          [this](const auto &e) {
              const auto &txn_id = e.event_id;
              this->current_txn  = txn_id;

              if (txn_id.empty() || txn_id[0] != 'm') {
                  nhlog::ui()->debug("Invalid txn id '{}'", txn_id);
                  cache::client()->removePendingStatus(room_id_, txn_id);
                  return;
              }

              if constexpr (mtx::events::message_content_to_type<decltype(e.content)> !=
                            mtx::events::EventType::Unsupported)
                  http::client()->send_room_message(
                    room_id_,
                    txn_id,
                    e.content,
                    [this, txn_id, e](const mtx::responses::EventId &event_id,
                                      mtx::http::RequestErr err) {
                        if (err) {
                            const int status_code = static_cast<int>(err->status_code);
                            nhlog::net()->warn("[{}] failed to send message: {} {}",
                                               txn_id,
                                               err->matrix_error.error,
                                               status_code);
                            emit messageFailed(txn_id);
                            return;
                        }

                        emit messageSent(txn_id, event_id.event_id.to_string());
                        if constexpr (std::is_same_v<decltype(e.content),
                                                     mtx::events::msg::Encrypted>) {
                            auto event = decryptEvent({room_id_, e.event_id}, e);
                            if (event->event) {
                                if (auto dec = std::get_if<mtx::events::RoomEvent<
                                      mtx::events::msg::KeyVerificationRequest>>(
                                      &event->event.value())) {
                                    emit updateFlowEventId(event_id.event_id.to_string());
                                }
                            }
                        }
                    });
          },
          event->data);
    });

    connect(
      this,
      &EventStore::messageFailed,
      this,
      [this](std::string txn_id) {
          if (current_txn == txn_id) {
              current_txn_error_count++;
              if (current_txn_error_count > 10) {
                  nhlog::ui()->debug("failing txn id '{}'", txn_id);
                  cache::client()->removePendingStatus(room_id_, txn_id);
                  current_txn_error_count = 0;
              }
          }
          QTimer::singleShot(1000, this, [this]() {
              nhlog::ui()->debug("timeout");
              this->current_txn = "";
              emit processPending();
          });
      },
      Qt::QueuedConnection);

    connect(
      this,
      &EventStore::messageSent,
      this,
      [this](std::string txn_id, std::string event_id) {
          nhlog::ui()->debug("sent {}", txn_id);

          // Replace the event_id in pending edits/replies/redactions with the actual
          // event_id of this event. This allows one to edit and reply to events that are
          // currently pending.
          for (const auto &pending_event_id : cache::client()->pendingEvents(room_id_)) {
              if (auto pending_event = cache::client()->getEvent(room_id_, pending_event_id)) {
                  bool was_encrypted = false;
                  mtx::events::EncryptedEvent<mtx::events::msg::Encrypted> original_encrypted;
                  if (auto encrypted =
                        std::get_if<mtx::events::EncryptedEvent<mtx::events::msg::Encrypted>>(
                          &pending_event->data)) {
                      auto d_event = decryptEvent({room_id_, encrypted->event_id}, *encrypted);
                      if (d_event->event) {
                          was_encrypted       = true;
                          original_encrypted  = *encrypted;
                          pending_event->data = *d_event->event;
                      }
                  }

                  auto relations = mtx::accessors::relations(pending_event->data);

                  // Replace the blockquote in fallback reply
                  auto related_text = std::get_if<mtx::events::RoomEvent<mtx::events::msg::Text>>(
                    &pending_event->data);
                  if (related_text && relations.reply_to() == txn_id) {
                      size_t index = related_text->content.formatted_body.find(txn_id);
                      if (index != std::string::npos) {
                          related_text->content.formatted_body.replace(
                            index, txn_id.length(), event_id);
                      }
                  }

                  bool replaced_txn = false;
                  for (mtx::common::Relation &rel : relations.relations) {
                      if (rel.event_id == txn_id) {
                          rel.event_id = event_id;
                          replaced_txn = true;
                      }
                  }

                  if (!replaced_txn)
                      continue;

                  mtx::accessors::set_relations(pending_event->data, std::move(relations));

                  // reencrypt. This is a bit of a hack and might make people able to read the
                  // message, that were in the room at the time of sending the last pending message.
                  // That window is pretty small though, so it should be good enough. We also just
                  // fail, if there was no session. But there SHOULD always be one. Let's wait until
                  // I am proven wrong :3
                  if (was_encrypted) {
                      auto session = cache::getOutboundMegolmSession(room_id_);
                      if (!session.session)
                          continue;

                      std::visit(
                        [&pending_event, &original_encrypted, &session, this](auto &msg) {
                            nlohmann::json doc = {{"type", mtx::events::to_string(msg.type)},
                                                  {"content", nlohmann::json(msg.content)},
                                                  {"room_id", room_id_}};

                            auto data = olm::encrypt_group_message_with_session(
                              session.session, http::client()->device_id(), doc);

                            session.data.message_index =
                              olm_outbound_group_session_message_index(session.session.get());
                            cache::updateOutboundMegolmSession(
                              room_id_, session.data, session.session);

                            original_encrypted.content = data;
                            pending_event->data        = original_encrypted;
                        },
                        pending_event->data);
                  }

                  cache::client()->replaceEvent(room_id_, pending_event_id, *pending_event);

                  auto idx = idToIndex(pending_event_id);

                  events_by_id_.remove({room_id_, pending_event_id});
                  events_.remove({room_id_, toInternalIdx(*idx)});
              }
          }

          http::client()->read_event(
            room_id_,
            event_id,
            [this, event_id](mtx::http::RequestErr err) {
                if (err) {
                    nhlog::net()->warn("failed to read_event ({}, {})", room_id_, event_id);
                }
            },
            !UserSettings::instance()->readReceipts());

          auto idx = idToIndex(event_id);

          if (idx)
              emit dataChanged(*idx, *idx);

          cache::client()->removePendingStatus(room_id_, txn_id);
          this->current_txn             = "";
          this->current_txn_error_count = 0;
          emit processPending();
      },
      Qt::QueuedConnection);
}

void
EventStore::addPending(mtx::events::collections::TimelineEvents event)
{
    if (this->thread() != QThread::currentThread())
        nhlog::db()->warn("{} called from a different thread!", __func__);

    cache::client()->savePendingMessage(this->room_id_, {event});
    mtx::responses::Timeline events;
    events.limited = false;
    events.events.emplace_back(event);
    handleSync(events);

    emit processPending();
}

void
EventStore::clearTimeline()
{
    emit beginResetModel();

    cache::client()->clearTimeline(room_id_);
    auto range = cache::client()->getTimelineRange(room_id_);
    if (range) {
        nhlog::db()->info("Range {} {}", range->last, range->first);
        this->last  = range->last;
        this->first = range->first;
    } else {
        this->first = std::numeric_limits<uint64_t>::max();
        this->last  = std::numeric_limits<uint64_t>::max();
    }
    nhlog::ui()->info("Range {} {}", this->last, this->first);

    decryptedEvents_.clear();
    events_.clear();

    emit endResetModel();
}

void
EventStore::receivedSessionKey(const std::string &session_id)
{
    if (!pending_key_requests.count(session_id))
        return;

    auto request = pending_key_requests.at(session_id);

    // Don't request keys again until Nheko is restarted (for now)
    pending_key_requests[session_id].events.clear();

    if (!request.events.empty())
        olm::send_key_request_for(request.events.front(), request.request_id, true);

    for (const auto &e : request.events) {
        auto idx = idToIndex(e.event_id);
        if (idx) {
            decryptedEvents_.remove({room_id_, e.event_id});
            events_by_id_.remove({room_id_, e.event_id});
            events_.remove({room_id_, toInternalIdx(*idx)});
            emit dataChanged(*idx, *idx);
        }
    }
}

void
EventStore::handleSync(const mtx::responses::Timeline &events)
{
    if (this->thread() != QThread::currentThread())
        nhlog::db()->warn("{} called from a different thread!", __func__);

    auto range = cache::client()->getTimelineRange(room_id_);
    if (!range) {
        emit beginResetModel();
        this->first = std::numeric_limits<uint64_t>::max();
        this->last  = std::numeric_limits<uint64_t>::max();

        decryptedEvents_.clear();
        events_.clear();
        emit endResetModel();
        return;
    }

    if (events.limited) {
        emit beginResetModel();
        this->last  = range->last;
        this->first = range->first;

        decryptedEvents_.clear();
        events_.clear();
        emit endResetModel();
    } else if (range->last > this->last) {
        emit beginInsertRows(toExternalIdx(this->last + 1), toExternalIdx(range->last));
        this->last = range->last;
        emit endInsertRows();
    }

    for (const auto &event : events.events) {
        std::set<std::string> relates_to;
        std::string edited_event;
        if (auto redaction =
              std::get_if<mtx::events::RedactionEvent<mtx::events::msg::Redaction>>(&event)) {
            // fixup reactions
            auto redacted = events_by_id_.object({room_id_, redaction->redacts});
            if (redacted) {
                auto id = mtx::accessors::relations(*redacted);
                if (id.annotates()) {
                    auto idx = idToIndex(id.annotates()->event_id);
                    if (idx) {
                        events_by_id_.remove({room_id_, redaction->redacts});
                        events_.remove({room_id_, toInternalIdx(*idx)});
                        emit dataChanged(*idx, *idx);
                    }
                }
            }

            relates_to.insert(redaction->redacts);
        } else {
            for (const auto &r : mtx::accessors::relations(event).relations) {
                relates_to.insert(r.event_id);

                if (r.rel_type == mtx::common::RelationType::Replace)
                    edited_event = r.event_id;
            }
        }

        for (const auto &relates_to_id : relates_to) {
            auto idx = cache::client()->getTimelineIndex(room_id_, relates_to_id);
            if (idx) {
                events_by_id_.remove({room_id_, relates_to_id});
                decryptedEvents_.remove({room_id_, relates_to_id});
                events_.remove({room_id_, *idx});
                emit dataChanged(toExternalIdx(*idx), toExternalIdx(*idx));
            }
        }

        if (auto txn_id = mtx::accessors::transaction_id(event); !txn_id.empty()) {
            auto idx = cache::client()->getTimelineIndex(room_id_, mtx::accessors::event_id(event));
            if (idx) {
                Index index{room_id_, *idx};
                events_.remove(index);
                emit dataChanged(toExternalIdx(*idx), toExternalIdx(*idx));
            }
        }

        if (!edited_event.empty()) {
            for (const auto &downstream_event :
                 cache::client()->relatedEvents(room_id_, edited_event)) {
                auto idx = cache::client()->getTimelineIndex(room_id_, downstream_event);
                if (idx) {
                    emit dataChanged(toExternalIdx(*idx), toExternalIdx(*idx));
                }
            }
        }

        // decrypting and checking some encrypted messages
        if (auto encrypted =
              std::get_if<mtx::events::EncryptedEvent<mtx::events::msg::Encrypted>>(&event)) {
            auto d_event = decryptEvent({room_id_, encrypted->event_id}, *encrypted);
            if (d_event->event &&
                std::visit([](auto e) { return (e.sender != utils::localUser().toStdString()); },
                           *d_event->event)) {
                handle_room_verification(*d_event->event);
            }
        }
    }
}

namespace {
template<class... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;
}

void
EventStore::handle_room_verification(mtx::events::collections::TimelineEvents event)
{
    std::visit(
      overloaded{
        [this](const mtx::events::RoomEvent<mtx::events::msg::KeyVerificationRequest> &msg) {
            nhlog::db()->debug("handle_room_verification: Request");
            emit startDMVerification(msg);
        },
        [](const mtx::events::RoomEvent<mtx::events::msg::KeyVerificationCancel> &msg) {
            nhlog::db()->debug("handle_room_verification: Cancel");
            ChatPage::instance()->receivedDeviceVerificationCancel(msg.content);
        },
        [](const mtx::events::RoomEvent<mtx::events::msg::KeyVerificationAccept> &msg) {
            nhlog::db()->debug("handle_room_verification: Accept");
            ChatPage::instance()->receivedDeviceVerificationAccept(msg.content);
        },
        [](const mtx::events::RoomEvent<mtx::events::msg::KeyVerificationKey> &msg) {
            nhlog::db()->debug("handle_room_verification: Key");
            ChatPage::instance()->receivedDeviceVerificationKey(msg.content);
        },
        [](const mtx::events::RoomEvent<mtx::events::msg::KeyVerificationMac> &msg) {
            nhlog::db()->debug("handle_room_verification: Mac");
            ChatPage::instance()->receivedDeviceVerificationMac(msg.content);
        },
        [](const mtx::events::RoomEvent<mtx::events::msg::KeyVerificationReady> &msg) {
            nhlog::db()->debug("handle_room_verification: Ready");
            ChatPage::instance()->receivedDeviceVerificationReady(msg.content);
        },
        [](const mtx::events::RoomEvent<mtx::events::msg::KeyVerificationDone> &msg) {
            nhlog::db()->debug("handle_room_verification: Done");
            ChatPage::instance()->receivedDeviceVerificationDone(msg.content);
        },
        [](const mtx::events::RoomEvent<mtx::events::msg::KeyVerificationStart> &msg) {
            nhlog::db()->debug("handle_room_verification: Start");
            ChatPage::instance()->receivedDeviceVerificationStart(msg.content, msg.sender);
        },
        [](const auto &) {},
      },
      event);
}

std::vector<mtx::events::collections::TimelineEvents>
EventStore::edits(const std::string &event_id)
{
    auto event_ids = cache::client()->relatedEvents(room_id_, event_id);

    auto original_event = get(event_id, "", false, false);
    if (!original_event ||
        std::holds_alternative<mtx::events::RoomEvent<mtx::events::msg::Redacted>>(*original_event))
        return {};

    const auto &original_sender    = mtx::accessors::sender(*original_event);
    const auto &original_relations = mtx::accessors::relations(*original_event);

    std::vector<mtx::events::collections::TimelineEvents> edits;
    for (const auto &id : event_ids) {
        auto related_event = get(id, event_id, false, false);
        if (!related_event)
            continue;

        const auto &edit_rel = mtx::accessors::relations(*related_event);
        if (edit_rel.replaces() == event_id &&
            original_sender == mtx::accessors::sender(*related_event)) {
            auto related_ev = *related_event;
            if (edit_rel.synthesized && original_relations.reply_to() && !edit_rel.reply_to()) {
                auto edit_rel_copy = edit_rel;
                edit_rel_copy.relations.push_back(
                  {mtx::common::RelationType::InReplyTo, original_relations.reply_to().value()});
                mtx::accessors::set_relations(related_ev, std::move(edit_rel_copy));
            }
            edits.push_back(std::move(related_ev));
        }
    }

    auto c = cache::client();
    std::sort(edits.begin(),
              edits.end(),
              [this, c](const mtx::events::collections::TimelineEvents &a,
                        const mtx::events::collections::TimelineEvents &b) {
                  return c->getArrivalIndex(this->room_id_, mtx::accessors::event_id(a)) <
                         c->getArrivalIndex(this->room_id_, mtx::accessors::event_id(b));
              });

    return edits;
}

QVariantList
EventStore::reactions(const std::string &event_id)
{
    auto event_ids = cache::client()->relatedEvents(room_id_, event_id);

    struct TempReaction
    {
        int count = 0;
        std::vector<std::string> users;
        std::string reactedBySelf;
    };
    std::map<std::string, TempReaction> aggregation;
    std::vector<Reaction> reactions;

    auto self = http::client()->user_id().to_string();
    for (const auto &id : event_ids) {
        auto related_event = get(id, event_id);
        if (!related_event)
            continue;

        if (auto reaction =
              std::get_if<mtx::events::RoomEvent<mtx::events::msg::Reaction>>(related_event);
            reaction && reaction->content.relations.annotates() &&
            reaction->content.relations.annotates()->key) {
            auto key  = reaction->content.relations.annotates()->key.value();
            auto &agg = aggregation[key];

            if (agg.count == 0) {
                Reaction temp{};
                temp.key_ = QString::fromStdString(key);
                reactions.push_back(temp);
            }

            agg.count++;
            agg.users.push_back(cache::displayName(room_id_, reaction->sender));
            if (reaction->sender == self)
                agg.reactedBySelf = reaction->event_id;
        }
    }

    QVariantList temp;
    temp.reserve(static_cast<int>(reactions.size()));
    for (auto &reaction : reactions) {
        const auto &agg            = aggregation[reaction.key_.toStdString()];
        reaction.count_            = agg.count;
        reaction.selfReactedEvent_ = QString::fromStdString(agg.reactedBySelf);

        bool firstReaction = true;
        for (const auto &user : agg.users) {
            if (firstReaction)
                firstReaction = false;
            else
                reaction.users_ += QLatin1String(", ");

            reaction.users_ += QString::fromStdString(user);
        }

        temp.append(QVariant::fromValue(reaction));
    }

    return temp;
}

mtx::events::collections::TimelineEvents *
EventStore::get(int idx, bool decrypt)
{
    if (this->thread() != QThread::currentThread())
        nhlog::db()->warn("{} called from a different thread!", __func__);

    Index index{room_id_, toInternalIdx(idx)};
    if (index.idx > last || index.idx < first)
        return nullptr;

    auto event_ptr = events_.object(index);
    if (!event_ptr) {
        auto event_id = cache::client()->getTimelineEventId(room_id_, index.idx);
        if (!event_id)
            return nullptr;

        std::optional<mtx::events::collections::TimelineEvent> event;
        auto edits_ = edits(*event_id);
        if (edits_.empty())
            event = cache::client()->getEvent(room_id_, *event_id);
        else
            event = {edits_.back()};

        if (!event)
            return nullptr;
        else
            event_ptr = new mtx::events::collections::TimelineEvents(std::move(event->data));
        events_.insert(index, event_ptr);
    }

    if (decrypt) {
        if (auto encrypted =
              std::get_if<mtx::events::EncryptedEvent<mtx::events::msg::Encrypted>>(event_ptr)) {
            auto decrypted = decryptEvent({room_id_, encrypted->event_id}, *encrypted);
            if (decrypted->event)
                return &*decrypted->event;
        }
    }

    return event_ptr;
}

std::optional<int>
EventStore::idToIndex(std::string_view id) const
{
    if (this->thread() != QThread::currentThread())
        nhlog::db()->warn("{} called from a different thread!", __func__);

    auto idx = cache::client()->getTimelineIndex(room_id_, id);
    if (idx)
        return toExternalIdx(*idx);
    else
        return std::nullopt;
}
std::optional<std::string>
EventStore::indexToId(int idx) const
{
    if (this->thread() != QThread::currentThread())
        nhlog::db()->warn("{} called from a different thread!", __func__);

    return cache::client()->getTimelineEventId(room_id_, toInternalIdx(idx));
}

olm::DecryptionResult *
EventStore::decryptEvent(const IdIndex &idx,
                         const mtx::events::EncryptedEvent<mtx::events::msg::Encrypted> &e)
{
    if (auto cachedEvent = decryptedEvents_.object(idx))
        return cachedEvent;

    MegolmSessionIndex index(room_id_, e.content);

    auto asCacheEntry = [&idx](olm::DecryptionResult &&event) {
        auto event_ptr = new olm::DecryptionResult(std::move(event));
        decryptedEvents_.insert(idx, event_ptr);
        return event_ptr;
    };

    auto decryptionResult = olm::decryptEvent(index, e);

    if (decryptionResult.error) {
        switch (decryptionResult.error) {
        case olm::DecryptionErrorCode::MissingSession:
        case olm::DecryptionErrorCode::MissingSessionIndex: {
            nhlog::crypto()->info("Could not find inbound megolm session ({}, {}, {})",
                                  index.room_id,
                                  index.session_id,
                                  e.sender);

            requestSession(e, false);
            break;
        }
        case olm::DecryptionErrorCode::DbError:
            nhlog::db()->critical("failed to retrieve megolm session with index ({}, {})",
                                  index.room_id,
                                  index.session_id,
                                  decryptionResult.error_message.value_or(""));
            break;
        case olm::DecryptionErrorCode::DecryptionFailed:
            nhlog::crypto()->critical("failed to decrypt message with index ({},  {}): {}",
                                      index.room_id,
                                      index.session_id,
                                      decryptionResult.error_message.value_or(""));
            break;
        case olm::DecryptionErrorCode::ParsingFailed:
            break;
        case olm::DecryptionErrorCode::ReplayAttack:
            nhlog::crypto()->critical("Reply attack while decryptiong event {} in room {} from {}!",
                                      e.event_id,
                                      room_id_,
                                      e.sender);
            break;
        case olm::DecryptionErrorCode::NoError:
            // unreachable
            break;
        }
        return asCacheEntry(std::move(decryptionResult));
    }

    auto encInfo = mtx::accessors::file(decryptionResult.event.value());
    if (encInfo)
        emit newEncryptedImage(encInfo.value());
    encInfo = mtx::accessors::thumbnail_file(decryptionResult.event.value());
    if (encInfo)
        emit newEncryptedImage(encInfo.value());

    return asCacheEntry(std::move(decryptionResult));
}

void
EventStore::requestSession(const mtx::events::EncryptedEvent<mtx::events::msg::Encrypted> &ev,
                           bool manual)
{
    // we may not want to request keys during initial sync and such
    if (suppressKeyRequests)
        return;

    auto copy    = ev;
    copy.room_id = room_id_;
    if (pending_key_requests.count(ev.content.session_id)) {
        auto &r = pending_key_requests.at(ev.content.session_id);
        r.events.push_back(copy);

        // automatically request once every 10 min, manually every 1 min
        qint64 delay = manual ? 60 : (60 * 10);
        if (r.requested_at + delay < QDateTime::currentSecsSinceEpoch()) {
            r.requested_at = QDateTime::currentSecsSinceEpoch();
            olm::lookup_keybackup(room_id_, ev.content.session_id);
            olm::send_key_request_for(copy, r.request_id);
        }
    } else {
        PendingKeyRequests request;
        request.request_id   = "key_request." + http::client()->generate_txn_id();
        request.requested_at = QDateTime::currentSecsSinceEpoch();
        request.events.push_back(copy);
        olm::lookup_keybackup(room_id_, ev.content.session_id);
        olm::send_key_request_for(copy, request.request_id);
        pending_key_requests[ev.content.session_id] = request;
    }
}

void
EventStore::enableKeyRequests(bool suppressKeyRequests_)
{
    if (!suppressKeyRequests_) {
        auto keys = decryptedEvents_.keys();
        for (const auto &key : qAsConst(keys))
            if (key.room == this->room_id_)
                decryptedEvents_.remove(key);
        suppressKeyRequests = false;
    } else
        suppressKeyRequests = true;
}

mtx::events::collections::TimelineEvents *
EventStore::get(const std::string &id,
                std::string_view related_to,
                bool decrypt,
                bool resolve_edits)
{
    if (this->thread() != QThread::currentThread())
        nhlog::db()->warn("{} called from a different thread!", __func__);

    if (id.empty())
        return nullptr;

    IdIndex index{room_id_, id};
    if (resolve_edits) {
        auto edits_ = edits(index.id);
        if (!edits_.empty()) {
            index.id       = mtx::accessors::event_id(edits_.back());
            auto event_ptr = new mtx::events::collections::TimelineEvents(std::move(edits_.back()));
            events_by_id_.insert(index, event_ptr);
        }
    }

    auto event_ptr = events_by_id_.object(index);
    if (!event_ptr) {
        auto event = cache::client()->getEvent(room_id_, index.id);
        if (!event) {
            http::client()->get_event(room_id_,
                                      index.id,
                                      [this, relatedTo = std::string(related_to), id = index.id](
                                        const mtx::events::collections::TimelineEvents &timeline,
                                        mtx::http::RequestErr err) {
                                          if (err) {
                                              nhlog::net()->error(
                                                "Failed to retrieve event with id {}, which was "
                                                "requested to show the replyTo for event {}",
                                                relatedTo,
                                                id);
                                              return;
                                          }
                                          emit eventFetched(id, relatedTo, timeline);
                                      });
            return nullptr;
        }
        event_ptr = new mtx::events::collections::TimelineEvents(std::move(event->data));
        events_by_id_.insert(index, event_ptr);
    }

    if (decrypt) {
        if (auto encrypted =
              std::get_if<mtx::events::EncryptedEvent<mtx::events::msg::Encrypted>>(event_ptr)) {
            auto decrypted = decryptEvent(index, *encrypted);
            if (decrypted->event)
                return &*decrypted->event;
        }
    }

    return event_ptr;
}

olm::DecryptionErrorCode
EventStore::decryptionError(std::string id)
{
    if (this->thread() != QThread::currentThread())
        nhlog::db()->warn("{} called from a different thread!", __func__);

    if (id.empty())
        return olm::DecryptionErrorCode::NoError;

    IdIndex index{room_id_, std::move(id)};
    auto edits_ = edits(index.id);
    if (!edits_.empty()) {
        index.id       = mtx::accessors::event_id(edits_.back());
        auto event_ptr = new mtx::events::collections::TimelineEvents(std::move(edits_.back()));
        events_by_id_.insert(index, event_ptr);
    }

    auto event_ptr = events_by_id_.object(index);
    if (!event_ptr) {
        auto event = cache::client()->getEvent(room_id_, index.id);
        if (!event) {
            return olm::DecryptionErrorCode::NoError;
        }
        event_ptr = new mtx::events::collections::TimelineEvents(std::move(event->data));
        events_by_id_.insert(index, event_ptr);
    }

    if (auto encrypted =
          std::get_if<mtx::events::EncryptedEvent<mtx::events::msg::Encrypted>>(event_ptr)) {
        auto decrypted = decryptEvent(index, *encrypted);
        return decrypted->error;
    }

    return olm::DecryptionErrorCode::NoError;
}

void
EventStore::fetchMore()
{
    if (noMoreMessages) {
        emit fetchedMore();
        return;
    }

    mtx::http::MessagesOpts opts;
    opts.room_id = room_id_;
    opts.from    = cache::client()->previousBatchToken(room_id_);

    nhlog::ui()->debug("Paginating room {}, token {}", opts.room_id, opts.from);

    http::client()->messages(
      opts, [this, opts](const mtx::responses::Messages &res, mtx::http::RequestErr err) {
          if (cache::client()->previousBatchToken(room_id_) != opts.from) {
              nhlog::net()->warn("Cache cleared while fetching more messages, dropping "
                                 "/messages response");
              if (!opts.to.empty())
                  emit fetchedMore();
              return;
          }
          if (err) {
              nhlog::net()->error("failed to call /messages ({}): {} - {} - {}",
                                  opts.room_id,
                                  mtx::errors::to_string(err->matrix_error.errcode),
                                  err->matrix_error.error,
                                  err->parse_error);
              emit fetchedMore();
              return;
          }

          emit oldMessagesRetrieved(std::move(res));
      });
}
