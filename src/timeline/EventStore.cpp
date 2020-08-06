#include "EventStore.h"

#include <QThread>
#include <QTimer>

#include "Cache.h"
#include "Cache_p.h"
#include "EventAccessors.h"
#include "Logging.h"
#include "MatrixClient.h"
#include "Olm.h"

Q_DECLARE_METATYPE(Reaction)

QCache<EventStore::IdIndex, mtx::events::collections::TimelineEvents> EventStore::decryptedEvents_{
  1000};
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
          [this](std::string id,
                 std::string relatedTo,
                 mtx::events::collections::TimelineEvents timeline) {
                  cache::client()->storeEvent(room_id_, id, {timeline});

                  if (!relatedTo.empty()) {
                          auto idx = idToIndex(relatedTo);
                          if (idx)
                                  emit dataChanged(*idx, *idx);
                  }
          },
          Qt::QueuedConnection);

        connect(
          this,
          &EventStore::oldMessagesRetrieved,
          this,
          [this](const mtx::responses::Messages &res) {
                  //
                  uint64_t newFirst = cache::client()->saveOldMessages(room_id_, res);
                  if (newFirst == first)
                          fetchMore();
                  else {
                          emit beginInsertRows(toExternalIdx(newFirst),
                                               toExternalIdx(this->first - 1));
                          this->first = newFirst;
                          emit endInsertRows();
                          emit fetchedMore();
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
                  [this](auto e) {
                          auto txn_id       = e.event_id;
                          this->current_txn = txn_id;

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
                                    [this, txn_id](const mtx::responses::EventId &event_id,
                                                   mtx::http::RequestErr err) {
                                            if (err) {
                                                    const int status_code =
                                                      static_cast<int>(err->status_code);
                                                    nhlog::net()->warn(
                                                      "[{}] failed to send message: {} {}",
                                                      txn_id,
                                                      err->matrix_error.error,
                                                      status_code);
                                                    emit messageFailed(txn_id);
                                                    return;
                                            }
                                            emit messageSent(txn_id, event_id.event_id.to_string());
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

                  http::client()->read_event(
                    room_id_, event_id, [this, event_id](mtx::http::RequestErr err) {
                            if (err) {
                                    nhlog::net()->warn(
                                      "failed to read_event ({}, {})", room_id_, event_id);
                            }
                    });

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
EventStore::handleSync(const mtx::responses::Timeline &events)
{
        if (this->thread() != QThread::currentThread())
                nhlog::db()->warn("{} called from a different thread!", __func__);

        auto range = cache::client()->getTimelineRange(room_id_);
        if (!range)
                return;

        if (events.limited) {
                emit beginResetModel();
                this->last  = range->last;
                this->first = range->first;
                emit endResetModel();

        } else if (range->last > this->last) {
                emit beginInsertRows(toExternalIdx(this->last + 1), toExternalIdx(range->last));
                this->last = range->last;
                emit endInsertRows();
        }

        for (const auto &event : events.events) {
                std::string relates_to;
                if (auto redaction =
                      std::get_if<mtx::events::RedactionEvent<mtx::events::msg::Redaction>>(
                        &event)) {
                        // fixup reactions
                        auto redacted = events_by_id_.object({room_id_, redaction->redacts});
                        if (redacted) {
                                auto id = mtx::accessors::relates_to_event_id(*redacted);
                                if (!id.empty()) {
                                        auto idx = idToIndex(id);
                                        if (idx) {
                                                events_by_id_.remove(
                                                  {room_id_, redaction->redacts});
                                                events_.remove({room_id_, toInternalIdx(*idx)});
                                                emit dataChanged(*idx, *idx);
                                        }
                                }
                        }

                        relates_to = redaction->redacts;
                } else if (auto reaction =
                             std::get_if<mtx::events::RoomEvent<mtx::events::msg::Reaction>>(
                               &event)) {
                        relates_to = reaction->content.relates_to.event_id;
                } else {
                        relates_to = mtx::accessors::in_reply_to_event(event);
                }

                if (!relates_to.empty()) {
                        auto idx = cache::client()->getTimelineIndex(room_id_, relates_to);
                        if (idx) {
                                events_by_id_.remove({room_id_, relates_to});
                                decryptedEvents_.remove({room_id_, relates_to});
                                events_.remove({room_id_, *idx});
                                emit dataChanged(toExternalIdx(*idx), toExternalIdx(*idx));
                        }
                }

                if (auto txn_id = mtx::accessors::transaction_id(event); !txn_id.empty()) {
                        auto idx = cache::client()->getTimelineIndex(
                          room_id_, mtx::accessors::event_id(event));
                        if (idx) {
                                Index index{room_id_, *idx};
                                events_.remove(index);
                                emit dataChanged(toExternalIdx(*idx), toExternalIdx(*idx));
                        }
                }
        }
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

                if (auto reaction = std::get_if<mtx::events::RoomEvent<mtx::events::msg::Reaction>>(
                      related_event)) {
                        auto &agg = aggregation[reaction->content.relates_to.key];

                        if (agg.count == 0) {
                                Reaction temp{};
                                temp.key_ =
                                  QString::fromStdString(reaction->content.relates_to.key);
                                reactions.push_back(temp);
                        }

                        agg.count++;
                        agg.users.push_back(cache::displayName(room_id_, reaction->sender));
                        if (reaction->sender == self)
                                agg.reactedBySelf = reaction->event_id;
                }
        }

        QVariantList temp;
        for (auto &reaction : reactions) {
                const auto &agg            = aggregation[reaction.key_.toStdString()];
                reaction.count_            = agg.count;
                reaction.selfReactedEvent_ = QString::fromStdString(agg.reactedBySelf);

                bool firstReaction = true;
                for (const auto &user : agg.users) {
                        if (firstReaction)
                                firstReaction = false;
                        else
                                reaction.users_ += ", ";

                        reaction.users_ += QString::fromStdString(user);
                }

                nhlog::db()->debug("key: {}, count: {}, users: {}",
                                   reaction.key_.toStdString(),
                                   reaction.count_,
                                   reaction.users_.toStdString());
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

                auto event = cache::client()->getEvent(room_id_, *event_id);
                if (!event)
                        return nullptr;
                else
                        event_ptr =
                          new mtx::events::collections::TimelineEvents(std::move(event->data));
                events_.insert(index, event_ptr);
        }

        if (decrypt)
                if (auto encrypted =
                      std::get_if<mtx::events::EncryptedEvent<mtx::events::msg::Encrypted>>(
                        event_ptr))
                        return decryptEvent({room_id_, encrypted->event_id}, *encrypted);

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

mtx::events::collections::TimelineEvents *
EventStore::decryptEvent(const IdIndex &idx,
                         const mtx::events::EncryptedEvent<mtx::events::msg::Encrypted> &e)
{
        if (auto cachedEvent = decryptedEvents_.object(idx))
                return cachedEvent;

        MegolmSessionIndex index;
        index.room_id    = room_id_;
        index.session_id = e.content.session_id;
        index.sender_key = e.content.sender_key;

        auto asCacheEntry = [&idx](mtx::events::collections::TimelineEvents &&event) {
                auto event_ptr = new mtx::events::collections::TimelineEvents(std::move(event));
                decryptedEvents_.insert(idx, event_ptr);
                return event_ptr;
        };

        auto decryptionResult = olm::decryptEvent(index, e);

        if (decryptionResult.error) {
                mtx::events::RoomEvent<mtx::events::msg::Notice> dummy;
                dummy.origin_server_ts = e.origin_server_ts;
                dummy.event_id         = e.event_id;
                dummy.sender           = e.sender;
                switch (*decryptionResult.error) {
                case olm::DecryptionErrorCode::MissingSession:
                        dummy.content.body =
                          tr("-- Encrypted Event (No keys found for decryption) --",
                             "Placeholder, when the message was not decrypted yet or can't be "
                             "decrypted.")
                            .toStdString();
                        nhlog::crypto()->info("Could not find inbound megolm session ({}, {}, {})",
                                              index.room_id,
                                              index.session_id,
                                              e.sender);
                        // TODO: Check if this actually works and look in key backup
                        olm::send_key_request_for(room_id_, e);
                        break;
                case olm::DecryptionErrorCode::DbError:
                        nhlog::db()->critical(
                          "failed to retrieve megolm session with index ({}, {}, {})",
                          index.room_id,
                          index.session_id,
                          index.sender_key,
                          decryptionResult.error_message.value_or(""));
                        dummy.content.body =
                          tr("-- Decryption Error (failed to retrieve megolm keys from db) --",
                             "Placeholder, when the message can't be decrypted, because the DB "
                             "access "
                             "failed.")
                            .toStdString();
                        break;
                case olm::DecryptionErrorCode::DecryptionFailed:
                        nhlog::crypto()->critical(
                          "failed to decrypt message with index ({}, {}, {}): {}",
                          index.room_id,
                          index.session_id,
                          index.sender_key,
                          decryptionResult.error_message.value_or(""));
                        dummy.content.body =
                          tr("-- Decryption Error (%1) --",
                             "Placeholder, when the message can't be decrypted. In this case, the "
                             "Olm "
                             "decrytion returned an error, which is passed as %1.")
                            .arg(
                              QString::fromStdString(decryptionResult.error_message.value_or("")))
                            .toStdString();
                        break;
                case olm::DecryptionErrorCode::ParsingFailed:
                        dummy.content.body =
                          tr("-- Encrypted Event (Unknown event type) --",
                             "Placeholder, when the message was decrypted, but we couldn't parse "
                             "it, because "
                             "Nheko/mtxclient don't support that event type yet.")
                            .toStdString();
                        break;
                case olm::DecryptionErrorCode::ReplayAttack:
                        nhlog::crypto()->critical(
                          "Reply attack while decryptiong event {} in room {} from {}!",
                          e.event_id,
                          room_id_,
                          index.sender_key);
                        dummy.content.body =
                          tr("-- Reply attack! This message index was reused! --").toStdString();
                        break;
                case olm::DecryptionErrorCode::UnknownFingerprint:
                        // TODO: don't fail, just show in UI.
                        nhlog::crypto()->critical("Message by unverified fingerprint {}",
                                                  index.sender_key);
                        dummy.content.body =
                          tr("-- Message by unverified device! --").toStdString();
                        break;
                }
                return asCacheEntry(std::move(dummy));
        }

        auto encInfo = mtx::accessors::file(decryptionResult.event.value());
        if (encInfo)
                emit newEncryptedImage(encInfo.value());

        return asCacheEntry(std::move(decryptionResult.event.value()));
}

mtx::events::collections::TimelineEvents *
EventStore::get(std::string_view id, std::string_view related_to, bool decrypt)
{
        if (this->thread() != QThread::currentThread())
                nhlog::db()->warn("{} called from a different thread!", __func__);

        if (id.empty())
                return nullptr;

        IdIndex index{room_id_, std::string(id.data(), id.size())};

        auto event_ptr = events_by_id_.object(index);
        if (!event_ptr) {
                auto event = cache::client()->getEvent(room_id_, index.id);
                if (!event) {
                        http::client()->get_event(
                          room_id_,
                          index.id,
                          [this,
                           relatedTo = std::string(related_to.data(), related_to.size()),
                           id = index.id](const mtx::events::collections::TimelineEvents &timeline,
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

        if (decrypt)
                if (auto encrypted =
                      std::get_if<mtx::events::EncryptedEvent<mtx::events::msg::Encrypted>>(
                        event_ptr))
                        return decryptEvent(index, *encrypted);

        return event_ptr;
}

void
EventStore::fetchMore()
{
        mtx::http::MessagesOpts opts;
        opts.room_id = room_id_;
        opts.from    = cache::client()->previousBatchToken(room_id_);

        nhlog::ui()->debug("Paginating room {}, token {}", opts.room_id, opts.from);

        http::client()->messages(
          opts, [this, opts](const mtx::responses::Messages &res, mtx::http::RequestErr err) {
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
