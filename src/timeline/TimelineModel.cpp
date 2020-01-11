#include "TimelineModel.h"

#include <algorithm>
#include <type_traits>

#include <QFileDialog>
#include <QMimeDatabase>
#include <QRegularExpression>
#include <QStandardPaths>

#include "ChatPage.h"
#include "EventAccessors.h"
#include "Logging.h"
#include "MainWindow.h"
#include "MatrixClient.h"
#include "MxcImageProvider.h"
#include "Olm.h"
#include "TimelineViewManager.h"
#include "Utils.h"
#include "dialogs/RawMessage.h"

Q_DECLARE_METATYPE(QModelIndex)

namespace {
struct RoomEventType
{
        template<class T>
        qml_mtx_events::EventType operator()(const mtx::events::Event<T> &e)
        {
                using mtx::events::EventType;
                switch (e.type) {
                case EventType::RoomKeyRequest:
                        return qml_mtx_events::EventType::KeyRequest;
                case EventType::RoomAliases:
                        return qml_mtx_events::EventType::Aliases;
                case EventType::RoomAvatar:
                        return qml_mtx_events::EventType::Avatar;
                case EventType::RoomCanonicalAlias:
                        return qml_mtx_events::EventType::CanonicalAlias;
                case EventType::RoomCreate:
                        return qml_mtx_events::EventType::Create;
                case EventType::RoomEncrypted:
                        return qml_mtx_events::EventType::Encrypted;
                case EventType::RoomEncryption:
                        return qml_mtx_events::EventType::Encryption;
                case EventType::RoomGuestAccess:
                        return qml_mtx_events::EventType::GuestAccess;
                case EventType::RoomHistoryVisibility:
                        return qml_mtx_events::EventType::HistoryVisibility;
                case EventType::RoomJoinRules:
                        return qml_mtx_events::EventType::JoinRules;
                case EventType::RoomMember:
                        return qml_mtx_events::EventType::Member;
                case EventType::RoomMessage:
                        return qml_mtx_events::EventType::UnknownMessage;
                case EventType::RoomName:
                        return qml_mtx_events::EventType::Name;
                case EventType::RoomPowerLevels:
                        return qml_mtx_events::EventType::PowerLevels;
                case EventType::RoomTopic:
                        return qml_mtx_events::EventType::Topic;
                case EventType::RoomTombstone:
                        return qml_mtx_events::EventType::Tombstone;
                case EventType::RoomRedaction:
                        return qml_mtx_events::EventType::Redaction;
                case EventType::RoomPinnedEvents:
                        return qml_mtx_events::EventType::PinnedEvents;
                case EventType::Sticker:
                        return qml_mtx_events::EventType::Sticker;
                case EventType::Tag:
                        return qml_mtx_events::EventType::Tag;
                case EventType::Unsupported:
                        return qml_mtx_events::EventType::Unsupported;
                default:
                        return qml_mtx_events::EventType::UnknownMessage;
                }
        }
        qml_mtx_events::EventType operator()(const mtx::events::Event<mtx::events::msg::Audio> &)
        {
                return qml_mtx_events::EventType::AudioMessage;
        }
        qml_mtx_events::EventType operator()(const mtx::events::Event<mtx::events::msg::Emote> &)
        {
                return qml_mtx_events::EventType::EmoteMessage;
        }
        qml_mtx_events::EventType operator()(const mtx::events::Event<mtx::events::msg::File> &)
        {
                return qml_mtx_events::EventType::FileMessage;
        }
        qml_mtx_events::EventType operator()(const mtx::events::Event<mtx::events::msg::Image> &)
        {
                return qml_mtx_events::EventType::ImageMessage;
        }
        qml_mtx_events::EventType operator()(const mtx::events::Event<mtx::events::msg::Notice> &)
        {
                return qml_mtx_events::EventType::NoticeMessage;
        }
        qml_mtx_events::EventType operator()(const mtx::events::Event<mtx::events::msg::Text> &)
        {
                return qml_mtx_events::EventType::TextMessage;
        }
        qml_mtx_events::EventType operator()(const mtx::events::Event<mtx::events::msg::Video> &)
        {
                return qml_mtx_events::EventType::VideoMessage;
        }

        qml_mtx_events::EventType operator()(const mtx::events::Event<mtx::events::msg::Redacted> &)
        {
                return qml_mtx_events::EventType::Redacted;
        }
        // ::EventType::Type operator()(const Event<mtx::events::msg::Location> &e) { return
        // ::EventType::LocationMessage; }
};
}

qml_mtx_events::EventType
toRoomEventType(const mtx::events::collections::TimelineEvents &event)
{
        return std::visit(RoomEventType{}, event);
}

TimelineModel::TimelineModel(TimelineViewManager *manager, QString room_id, QObject *parent)
  : QAbstractListModel(parent)
  , room_id_(room_id)
  , manager_(manager)
{
        connect(
          this, &TimelineModel::oldMessagesRetrieved, this, &TimelineModel::addBackwardsEvents);
        connect(this, &TimelineModel::messageFailed, this, [this](QString txn_id) {
                pending.removeOne(txn_id);
                failed.insert(txn_id);
                int idx = idToIndex(txn_id);
                if (idx < 0) {
                        nhlog::ui()->warn("Failed index out of range");
                        return;
                }
                isProcessingPending = false;
                emit dataChanged(index(idx, 0), index(idx, 0));
        });
        connect(this, &TimelineModel::messageSent, this, [this](QString txn_id, QString event_id) {
                pending.removeOne(txn_id);
                int idx = idToIndex(txn_id);
                if (idx < 0) {
                        nhlog::ui()->warn("Sent index out of range");
                        return;
                }
                eventOrder[idx] = event_id;
                auto ev         = events.value(txn_id);
                ev              = std::visit(
                  [event_id](const auto &e) -> mtx::events::collections::TimelineEvents {
                          auto eventCopy     = e;
                          eventCopy.event_id = event_id.toStdString();
                          return eventCopy;
                  },
                  ev);

                events.remove(txn_id);
                events.insert(event_id, ev);

                // mark our messages as read
                readEvent(event_id.toStdString());

                // ask to be notified for read receipts
                cache::addPendingReceipt(room_id_, event_id);

                isProcessingPending = false;
                emit dataChanged(index(idx, 0), index(idx, 0));

                if (pending.size() > 0)
                        emit nextPendingMessage();
        });
        connect(this, &TimelineModel::redactionFailed, this, [](const QString &msg) {
                emit ChatPage::instance()->showNotification(msg);
        });

        connect(
          this, &TimelineModel::nextPendingMessage, this, &TimelineModel::processOnePendingMessage);
        connect(this, &TimelineModel::newMessageToSend, this, &TimelineModel::addPendingMessage);

        connect(this,
                &TimelineModel::replyFetched,
                this,
                [this](QString requestingEvent, mtx::events::collections::TimelineEvents event) {
                        events.insert(QString::fromStdString(mtx::accessors::event_id(event)),
                                      event);
                        auto idx = idToIndex(requestingEvent);
                        if (idx >= 0)
                                emit dataChanged(index(idx, 0), index(idx, 0));
                });
}

QHash<int, QByteArray>
TimelineModel::roleNames() const
{
        return {
          {Section, "section"},
          {Type, "type"},
          {Body, "body"},
          {FormattedBody, "formattedBody"},
          {UserId, "userId"},
          {UserName, "userName"},
          {Timestamp, "timestamp"},
          {Url, "url"},
          {ThumbnailUrl, "thumbnailUrl"},
          {Filename, "filename"},
          {Filesize, "filesize"},
          {MimeType, "mimetype"},
          {Height, "height"},
          {Width, "width"},
          {ProportionalHeight, "proportionalHeight"},
          {Id, "id"},
          {State, "state"},
          {IsEncrypted, "isEncrypted"},
          {ReplyTo, "replyTo"},
          {RoomName, "roomName"},
          {RoomTopic, "roomTopic"},
          {Dump, "dump"},
        };
}
int
TimelineModel::rowCount(const QModelIndex &parent) const
{
        Q_UNUSED(parent);
        return (int)this->eventOrder.size();
}

QVariantMap
TimelineModel::getDump(QString eventId) const
{
        if (events.contains(eventId))
                return data(eventId, Dump).toMap();
        return {};
}

QVariant
TimelineModel::data(const QString &id, int role) const
{
        using namespace mtx::accessors;
        namespace acc                                  = mtx::accessors;
        mtx::events::collections::TimelineEvents event = events.value(id);

        if (auto e =
              std::get_if<mtx::events::EncryptedEvent<mtx::events::msg::Encrypted>>(&event)) {
                event = decryptEvent(*e).event;
        }

        switch (role) {
        case UserId:
                return QVariant(QString::fromStdString(acc::sender(event)));
        case UserName:
                return QVariant(displayName(QString::fromStdString(acc::sender(event))));

        case Timestamp:
                return QVariant(origin_server_ts(event));
        case Type:
                return QVariant(toRoomEventType(event));
        case Body:
                return QVariant(utils::replaceEmoji(QString::fromStdString(body(event))));
        case FormattedBody: {
                const static QRegularExpression replyFallback(
                  "<mx-reply>.*</mx-reply>", QRegularExpression::DotMatchesEverythingOption);
                return QVariant(
                  utils::replaceEmoji(utils::linkifyMessage(formattedBodyWithFallback(event)))
                    .remove(replyFallback));
        }
        case Url:
                return QVariant(QString::fromStdString(url(event)));
        case ThumbnailUrl:
                return QVariant(QString::fromStdString(thumbnail_url(event)));
        case Filename:
                return QVariant(QString::fromStdString(filename(event)));
        case Filesize:
                return QVariant(utils::humanReadableFileSize(filesize(event)));
        case MimeType:
                return QVariant(QString::fromStdString(mimetype(event)));
        case Height:
                return QVariant(qulonglong{media_height(event)});
        case Width:
                return QVariant(qulonglong{media_width(event)});
        case ProportionalHeight: {
                auto w = media_width(event);
                if (w == 0)
                        w = 1;

                double prop = media_height(event) / (double)w;

                return QVariant(prop > 0 ? prop : 1.);
        }
        case Id:
                return id;
        case State:
                // only show read receipts for messages not from us
                if (acc::sender(event) != http::client()->user_id().to_string())
                        return qml_mtx_events::Empty;
                else if (failed.contains(id))
                        return qml_mtx_events::Failed;
                else if (pending.contains(id))
                        return qml_mtx_events::Sent;
                else if (read.contains(id) || cache::readReceipts(id, room_id_).size() > 1)
                        return qml_mtx_events::Read;
                else
                        return qml_mtx_events::Received;
        case IsEncrypted: {
                return std::holds_alternative<
                  mtx::events::EncryptedEvent<mtx::events::msg::Encrypted>>(events[id]);
        }
        case ReplyTo:
                return QVariant(QString::fromStdString(in_reply_to_event(event)));
        case RoomName:
                return QVariant(QString::fromStdString(room_name(event)));
        case RoomTopic:
                return QVariant(QString::fromStdString(room_topic(event)));
        case Dump: {
                QVariantMap m;
                auto names = roleNames();

                // m.insert(names[Section], data(id, static_cast<int>(Section)));
                m.insert(names[Type], data(id, static_cast<int>(Type)));
                m.insert(names[Body], data(id, static_cast<int>(Body)));
                m.insert(names[FormattedBody], data(id, static_cast<int>(FormattedBody)));
                m.insert(names[UserId], data(id, static_cast<int>(UserId)));
                m.insert(names[UserName], data(id, static_cast<int>(UserName)));
                m.insert(names[Timestamp], data(id, static_cast<int>(Timestamp)));
                m.insert(names[Url], data(id, static_cast<int>(Url)));
                m.insert(names[ThumbnailUrl], data(id, static_cast<int>(ThumbnailUrl)));
                m.insert(names[Filename], data(id, static_cast<int>(Filename)));
                m.insert(names[Filesize], data(id, static_cast<int>(Filesize)));
                m.insert(names[MimeType], data(id, static_cast<int>(MimeType)));
                m.insert(names[Height], data(id, static_cast<int>(Height)));
                m.insert(names[Width], data(id, static_cast<int>(Width)));
                m.insert(names[ProportionalHeight], data(id, static_cast<int>(ProportionalHeight)));
                m.insert(names[Id], data(id, static_cast<int>(Id)));
                m.insert(names[State], data(id, static_cast<int>(State)));
                m.insert(names[IsEncrypted], data(id, static_cast<int>(IsEncrypted)));
                m.insert(names[ReplyTo], data(id, static_cast<int>(ReplyTo)));
                m.insert(names[RoomName], data(id, static_cast<int>(RoomName)));
                m.insert(names[RoomTopic], data(id, static_cast<int>(RoomTopic)));

                return QVariant(m);
        }
        default:
                return QVariant();
        }
}

QVariant
TimelineModel::data(const QModelIndex &index, int role) const
{
        using namespace mtx::accessors;
        namespace acc = mtx::accessors;
        if (index.row() < 0 && index.row() >= (int)eventOrder.size())
                return QVariant();

        QString id = eventOrder[index.row()];

        mtx::events::collections::TimelineEvents event = events.value(id);

        if (role == Section) {
                QDateTime date = origin_server_ts(event);
                date.setTime(QTime());

                std::string userId = acc::sender(event);

                for (size_t r = index.row() + 1; r < eventOrder.size(); r++) {
                        auto tempEv        = events.value(eventOrder[r]);
                        QDateTime prevDate = origin_server_ts(tempEv);
                        prevDate.setTime(QTime());
                        if (prevDate != date)
                                return QString("%2 %1")
                                  .arg(date.toMSecsSinceEpoch())
                                  .arg(QString::fromStdString(userId));

                        std::string prevUserId = acc::sender(tempEv);
                        if (userId != prevUserId)
                                break;
                }

                return QString("%1").arg(QString::fromStdString(userId));
        }

        return data(id, role);
}

bool
TimelineModel::canFetchMore(const QModelIndex &) const
{
        if (eventOrder.empty())
                return true;
        if (!std::holds_alternative<mtx::events::StateEvent<mtx::events::state::Create>>(
              events[eventOrder.back()]))
                return true;
        else

                return false;
}

void
TimelineModel::fetchMore(const QModelIndex &)
{
        if (paginationInProgress) {
                nhlog::ui()->warn("Already loading older messages");
                return;
        }

        paginationInProgress = true;
        mtx::http::MessagesOpts opts;
        opts.room_id = room_id_.toStdString();
        opts.from    = prev_batch_token_.toStdString();

        nhlog::ui()->debug("Paginationg room {}", opts.room_id);

        http::client()->messages(
          opts, [this, opts](const mtx::responses::Messages &res, mtx::http::RequestErr err) {
                  if (err) {
                          nhlog::net()->error("failed to call /messages ({}): {} - {}",
                                              opts.room_id,
                                              mtx::errors::to_string(err->matrix_error.errcode),
                                              err->matrix_error.error);
                          paginationInProgress = false;
                          return;
                  }

                  emit oldMessagesRetrieved(std::move(res));
                  paginationInProgress = false;
          });
}

void
TimelineModel::addEvents(const mtx::responses::Timeline &timeline)
{
        if (isInitialSync) {
                prev_batch_token_ = QString::fromStdString(timeline.prev_batch);
                isInitialSync     = false;
        }

        if (timeline.events.empty())
                return;

        std::vector<QString> ids = internalAddEvents(timeline.events);

        if (ids.empty())
                return;

        beginInsertRows(QModelIndex(), 0, static_cast<int>(ids.size() - 1));
        this->eventOrder.insert(this->eventOrder.begin(), ids.rbegin(), ids.rend());
        endInsertRows();

        updateLastMessage();
}

template<typename T>
auto
isMessage(const mtx::events::RoomEvent<T> &e)
  -> std::enable_if_t<std::is_same<decltype(e.content.msgtype), std::string>::value, bool>
{
        return true;
}

template<typename T>
auto
isMessage(const mtx::events::Event<T> &)
{
        return false;
}

void
TimelineModel::updateLastMessage()
{
        for (auto it = eventOrder.begin(); it != eventOrder.end(); ++it) {
                auto event = events.value(*it);
                if (auto e = std::get_if<mtx::events::EncryptedEvent<mtx::events::msg::Encrypted>>(
                      &event)) {
                        event = decryptEvent(*e).event;
                }

                if (!std::visit([](const auto &e) -> bool { return isMessage(e); }, event))
                        continue;

                auto description = utils::getMessageDescription(
                  event, QString::fromStdString(http::client()->user_id().to_string()), room_id_);
                emit manager_->updateRoomsLastMessage(room_id_, description);
                return;
        }
}

std::vector<QString>
TimelineModel::internalAddEvents(
  const std::vector<mtx::events::collections::TimelineEvents> &timeline)
{
        std::vector<QString> ids;
        for (auto e : timeline) {
                QString id = QString::fromStdString(mtx::accessors::event_id(e));

                if (this->events.contains(id)) {
                        this->events.insert(id, e);
                        int idx = idToIndex(id);
                        emit dataChanged(index(idx, 0), index(idx, 0));
                        continue;
                }

                if (auto redaction =
                      std::get_if<mtx::events::RedactionEvent<mtx::events::msg::Redaction>>(&e)) {
                        QString redacts = QString::fromStdString(redaction->redacts);
                        auto redacted   = std::find(eventOrder.begin(), eventOrder.end(), redacts);

                        if (redacted != eventOrder.end()) {
                                auto redactedEvent = std::visit(
                                  [](const auto &ev)
                                    -> mtx::events::RoomEvent<mtx::events::msg::Redacted> {
                                          mtx::events::RoomEvent<mtx::events::msg::Redacted>
                                            replacement                = {};
                                          replacement.event_id         = ev.event_id;
                                          replacement.room_id          = ev.room_id;
                                          replacement.sender           = ev.sender;
                                          replacement.origin_server_ts = ev.origin_server_ts;
                                          replacement.type             = ev.type;
                                          return replacement;
                                  },
                                  e);
                                events.insert(redacts, redactedEvent);

                                int row = (int)std::distance(eventOrder.begin(), redacted);
                                emit dataChanged(index(row, 0), index(row, 0));
                        }

                        continue; // don't insert redaction into timeline
                }

                if (auto event =
                      std::get_if<mtx::events::EncryptedEvent<mtx::events::msg::Encrypted>>(&e)) {
                        e = decryptEvent(*event).event;
                }
                auto encInfo = mtx::accessors::file(e);

                if (encInfo)
                        emit newEncryptedImage(encInfo.value());

                this->events.insert(id, e);
                ids.push_back(id);

                auto replyTo  = mtx::accessors::in_reply_to_event(e);
                auto qReplyTo = QString::fromStdString(replyTo);
                if (!replyTo.empty() && !events.contains(qReplyTo)) {
                        http::client()->get_event(
                          this->room_id_.toStdString(),
                          replyTo,
                          [this, id, replyTo](
                            const mtx::events::collections::TimelineEvents &timeline,
                            mtx::http::RequestErr err) {
                                  if (err) {
                                          nhlog::net()->error(
                                            "Failed to retrieve event with id {}, which was "
                                            "requested to show the replyTo for event {}",
                                            replyTo,
                                            id.toStdString());
                                          return;
                                  }
                                  emit replyFetched(id, timeline);
                          });
                }
        }
        return ids;
}

void
TimelineModel::setCurrentIndex(int index)
{
        auto oldIndex = idToIndex(currentId);
        currentId     = indexToId(index);
        emit currentIndexChanged(index);

        if ((oldIndex > index || oldIndex == -1) && !pending.contains(currentId) &&
            ChatPage::instance()->isActiveWindow()) {
                readEvent(currentId.toStdString());
        }
}

void
TimelineModel::readEvent(const std::string &id)
{
        http::client()->read_event(room_id_.toStdString(), id, [this](mtx::http::RequestErr err) {
                if (err) {
                        nhlog::net()->warn("failed to read_event ({}, {})",
                                           room_id_.toStdString(),
                                           currentId.toStdString());
                }
        });
}

void
TimelineModel::addBackwardsEvents(const mtx::responses::Messages &msgs)
{
        std::vector<QString> ids = internalAddEvents(msgs.chunk);

        if (!ids.empty()) {
                beginInsertRows(QModelIndex(),
                                static_cast<int>(this->eventOrder.size()),
                                static_cast<int>(this->eventOrder.size() + ids.size() - 1));
                this->eventOrder.insert(this->eventOrder.end(), ids.begin(), ids.end());
                endInsertRows();
        }

        prev_batch_token_ = QString::fromStdString(msgs.end);
}

QColor
TimelineModel::userColor(QString id, QColor background)
{
        if (!userColors.contains(id))
                userColors.insert(
                  id, QColor(utils::generateContrastingHexColor(id, background.name())));
        return userColors.value(id);
}

QString
TimelineModel::displayName(QString id) const
{
        return cache::displayName(room_id_, id);
}

QString
TimelineModel::avatarUrl(QString id) const
{
        return cache::avatarUrl(room_id_, id);
}

QString
TimelineModel::formatDateSeparator(QDate date) const
{
        auto now = QDateTime::currentDateTime();

        QString fmt = QLocale::system().dateFormat(QLocale::LongFormat);

        if (now.date().year() == date.year()) {
                QRegularExpression rx("[^a-zA-Z]*y+[^a-zA-Z]*");
                fmt = fmt.remove(rx);
        }

        return date.toString(fmt);
}

QString
TimelineModel::escapeEmoji(QString str) const
{
        return utils::replaceEmoji(str);
}

void
TimelineModel::viewRawMessage(QString id) const
{
        std::string ev = utils::serialize_event(events.value(id)).dump(4);
        auto dialog    = new dialogs::RawMessage(QString::fromStdString(ev));
        Q_UNUSED(dialog);
}

void

TimelineModel::openUserProfile(QString userid) const
{
        MainWindow::instance()->openUserProfile(userid, room_id_);
}

DecryptionResult
TimelineModel::decryptEvent(const mtx::events::EncryptedEvent<mtx::events::msg::Encrypted> &e) const
{
        MegolmSessionIndex index;
        index.room_id    = room_id_.toStdString();
        index.session_id = e.content.session_id;
        index.sender_key = e.content.sender_key;

        mtx::events::RoomEvent<mtx::events::msg::Notice> dummy;
        dummy.origin_server_ts = e.origin_server_ts;
        dummy.event_id         = e.event_id;
        dummy.sender           = e.sender;
        dummy.content.body =
          tr("-- Encrypted Event (No keys found for decryption) --",
             "Placeholder, when the message was not decrypted yet or can't be decrypted")
            .toStdString();

        try {
                if (!cache::inboundMegolmSessionExists(index)) {
                        nhlog::crypto()->info("Could not find inbound megolm session ({}, {}, {})",
                                              index.room_id,
                                              index.session_id,
                                              e.sender);
                        // TODO: request megolm session_id & session_key from the sender.
                        return {dummy, false};
                }
        } catch (const lmdb::error &e) {
                nhlog::db()->critical("failed to check megolm session's existence: {}", e.what());
                dummy.content.body = tr("-- Decryption Error (failed to communicate with DB) --",
                                        "Placeholder, when the message can't be decrypted, because "
                                        "the DB access failed when trying to lookup the session.")
                                       .toStdString();
                return {dummy, false};
        }

        std::string msg_str;
        try {
                auto session = cache::getInboundMegolmSession(index);
                auto res     = olm::client()->decrypt_group_message(session, e.content.ciphertext);
                msg_str      = std::string((char *)res.data.data(), res.data.size());
        } catch (const lmdb::error &e) {
                nhlog::db()->critical("failed to retrieve megolm session with index ({}, {}, {})",
                                      index.room_id,
                                      index.session_id,
                                      index.sender_key,
                                      e.what());
                dummy.content.body =
                  tr("-- Decryption Error (failed to retrieve megolm keys from db) --",
                     "Placeholder, when the message can't be decrypted, because the DB access "
                     "failed.")
                    .toStdString();
                return {dummy, false};
        } catch (const mtx::crypto::olm_exception &e) {
                nhlog::crypto()->critical("failed to decrypt message with index ({}, {}, {}): {}",
                                          index.room_id,
                                          index.session_id,
                                          index.sender_key,
                                          e.what());
                dummy.content.body =
                  tr("-- Decryption Error (%1) --",
                     "Placeholder, when the message can't be decrypted. In this case, the Olm "
                     "decrytion returned an error, which is passed ad %1")
                    .arg(e.what())
                    .toStdString();
                return {dummy, false};
        }

        // Add missing fields for the event.
        json body                = json::parse(msg_str);
        body["event_id"]         = e.event_id;
        body["sender"]           = e.sender;
        body["origin_server_ts"] = e.origin_server_ts;
        body["unsigned"]         = e.unsigned_data;

        json event_array = json::array();
        event_array.push_back(body);

        std::vector<mtx::events::collections::TimelineEvents> temp_events;
        mtx::responses::utils::parse_timeline_events(event_array, temp_events);

        if (temp_events.size() == 1)
                return {temp_events.at(0), true};

        dummy.content.body =
          tr("-- Encrypted Event (Unknown event type) --",
             "Placeholder, when the message was decrypted, but we couldn't parse it, because "
             "Nheko/mtxclient don't support that event type yet")
            .toStdString();
        return {dummy, false};
}

void
TimelineModel::replyAction(QString id)
{
        auto event = events.value(id);
        if (auto e =
              std::get_if<mtx::events::EncryptedEvent<mtx::events::msg::Encrypted>>(&event)) {
                event = decryptEvent(*e).event;
        }

        RelatedInfo related   = {};
        related.quoted_user   = QString::fromStdString(mtx::accessors::sender(event));
        related.related_event = mtx::accessors::event_id(event);
        related.type          = mtx::accessors::msg_type(event);
        related.quoted_body   = mtx::accessors::formattedBodyWithFallback(event);
        related.quoted_body.remove(QRegularExpression(
          "<mx-reply>.*</mx-reply>", QRegularExpression::DotMatchesEverythingOption));
        nhlog::ui()->debug("after replacement: {}", related.quoted_body.toStdString());
        related.room = room_id_;

        // if (related.quoted_body.isEmpty())
        //        return;

        ChatPage::instance()->messageReply(related);
}

void
TimelineModel::readReceiptsAction(QString id) const
{
        MainWindow::instance()->openReadReceiptsDialog(id);
}

void
TimelineModel::redactEvent(QString id)
{
        if (!id.isEmpty())
                http::client()->redact_event(
                  room_id_.toStdString(),
                  id.toStdString(),
                  [this, id](const mtx::responses::EventId &, mtx::http::RequestErr err) {
                          if (err) {
                                  emit redactionFailed(
                                    tr("Message redaction failed: %1")
                                      .arg(QString::fromStdString(err->matrix_error.error)));
                                  return;
                          }

                          emit eventRedacted(id);
                  });
}

int
TimelineModel::idToIndex(QString id) const
{
        if (id.isEmpty())
                return -1;
        for (int i = 0; i < (int)eventOrder.size(); i++)
                if (id == eventOrder[i])
                        return i;
        return -1;
}

QString
TimelineModel::indexToId(int index) const
{
        if (index < 0 || index >= (int)eventOrder.size())
                return "";
        return eventOrder[index];
}

// Note: this will only be called for our messages
void
TimelineModel::markEventsAsRead(const std::vector<QString> &event_ids)
{
        for (const auto &id : event_ids) {
                read.insert(id);
                int idx = idToIndex(id);
                if (idx < 0) {
                        nhlog::ui()->warn("Read index out of range");
                        return;
                }
                emit dataChanged(index(idx, 0), index(idx, 0));
        }
}

void
TimelineModel::sendEncryptedMessage(const std::string &txn_id, nlohmann::json content)
{
        const auto room_id = room_id_.toStdString();

        using namespace mtx::events;
        using namespace mtx::identifiers;

        json doc{{"type", "m.room.message"}, {"content", content}, {"room_id", room_id}};

        try {
                // Check if we have already an outbound megolm session then we can use.
                if (cache::outboundMegolmSessionExists(room_id)) {
                        auto data = olm::encrypt_group_message(
                          room_id, http::client()->device_id(), doc.dump());

                        http::client()->send_room_message<msg::Encrypted, EventType::RoomEncrypted>(
                          room_id,
                          txn_id,
                          data,
                          [this, txn_id](const mtx::responses::EventId &res,
                                         mtx::http::RequestErr err) {
                                  if (err) {
                                          const int status_code =
                                            static_cast<int>(err->status_code);
                                          nhlog::net()->warn("[{}] failed to send message: {} {}",
                                                             txn_id,
                                                             err->matrix_error.error,
                                                             status_code);
                                          emit messageFailed(QString::fromStdString(txn_id));
                                  }
                                  emit messageSent(
                                    QString::fromStdString(txn_id),
                                    QString::fromStdString(res.event_id.to_string()));
                          });
                        return;
                }

                nhlog::ui()->debug("creating new outbound megolm session");

                // Create a new outbound megolm session.
                auto outbound_session  = olm::client()->init_outbound_group_session();
                const auto session_id  = mtx::crypto::session_id(outbound_session.get());
                const auto session_key = mtx::crypto::session_key(outbound_session.get());

                // TODO: needs to be moved in the lib.
                auto megolm_payload = json{{"algorithm", "m.megolm.v1.aes-sha2"},
                                           {"room_id", room_id},
                                           {"session_id", session_id},
                                           {"session_key", session_key}};

                // Saving the new megolm session.
                // TODO: Maybe it's too early to save.
                OutboundGroupSessionData session_data;
                session_data.session_id    = session_id;
                session_data.session_key   = session_key;
                session_data.message_index = 0; // TODO Update me
                cache::saveOutboundMegolmSession(
                  room_id, session_data, std::move(outbound_session));

                const auto members = cache::roomMembers(room_id);
                nhlog::ui()->info("retrieved {} members for {}", members.size(), room_id);

                auto keeper =
                  std::make_shared<StateKeeper>([megolm_payload, room_id, doc, txn_id, this]() {
                          try {
                                  auto data = olm::encrypt_group_message(
                                    room_id, http::client()->device_id(), doc.dump());

                                  http::client()
                                    ->send_room_message<msg::Encrypted, EventType::RoomEncrypted>(
                                      room_id,
                                      txn_id,
                                      data,
                                      [this, txn_id](const mtx::responses::EventId &res,
                                                     mtx::http::RequestErr err) {
                                              if (err) {
                                                      const int status_code =
                                                        static_cast<int>(err->status_code);
                                                      nhlog::net()->warn(
                                                        "[{}] failed to send message: {} {}",
                                                        txn_id,
                                                        err->matrix_error.error,
                                                        status_code);
                                                      emit messageFailed(
                                                        QString::fromStdString(txn_id));
                                              }
                                              emit messageSent(
                                                QString::fromStdString(txn_id),
                                                QString::fromStdString(res.event_id.to_string()));
                                      });
                          } catch (const lmdb::error &e) {
                                  nhlog::db()->critical(
                                    "failed to save megolm outbound session: {}", e.what());
                                  emit messageFailed(QString::fromStdString(txn_id));
                          }
                  });

                mtx::requests::QueryKeys req;
                for (const auto &member : members)
                        req.device_keys[member] = {};

                http::client()->query_keys(
                  req,
                  [keeper = std::move(keeper), megolm_payload, txn_id, this](
                    const mtx::responses::QueryKeys &res, mtx::http::RequestErr err) {
                          if (err) {
                                  nhlog::net()->warn("failed to query device keys: {} {}",
                                                     err->matrix_error.error,
                                                     static_cast<int>(err->status_code));
                                  // TODO: Mark the event as failed. Communicate with the UI.
                                  emit messageFailed(QString::fromStdString(txn_id));
                                  return;
                          }

                          for (const auto &user : res.device_keys) {
                                  // Mapping from a device_id with valid identity keys to the
                                  // generated room_key event used for sharing the megolm session.
                                  std::map<std::string, std::string> room_key_msgs;
                                  std::map<std::string, DevicePublicKeys> deviceKeys;

                                  room_key_msgs.clear();
                                  deviceKeys.clear();

                                  for (const auto &dev : user.second) {
                                          const auto user_id   = ::UserId(dev.second.user_id);
                                          const auto device_id = DeviceId(dev.second.device_id);

                                          const auto device_keys = dev.second.keys;
                                          const auto curveKey    = "curve25519:" + device_id.get();
                                          const auto edKey       = "ed25519:" + device_id.get();

                                          if ((device_keys.find(curveKey) == device_keys.end()) ||
                                              (device_keys.find(edKey) == device_keys.end())) {
                                                  nhlog::net()->debug(
                                                    "ignoring malformed keys for device {}",
                                                    device_id.get());
                                                  continue;
                                          }

                                          DevicePublicKeys pks;
                                          pks.ed25519    = device_keys.at(edKey);
                                          pks.curve25519 = device_keys.at(curveKey);

                                          try {
                                                  if (!mtx::crypto::verify_identity_signature(
                                                        json(dev.second), device_id, user_id)) {
                                                          nhlog::crypto()->warn(
                                                            "failed to verify identity keys: {}",
                                                            json(dev.second).dump(2));
                                                          continue;
                                                  }
                                          } catch (const json::exception &e) {
                                                  nhlog::crypto()->warn(
                                                    "failed to parse device key json: {}",
                                                    e.what());
                                                  continue;
                                          } catch (const mtx::crypto::olm_exception &e) {
                                                  nhlog::crypto()->warn(
                                                    "failed to verify device key json: {}",
                                                    e.what());
                                                  continue;
                                          }

                                          auto room_key = olm::client()
                                                            ->create_room_key_event(
                                                              user_id, pks.ed25519, megolm_payload)
                                                            .dump();

                                          room_key_msgs.emplace(device_id, room_key);
                                          deviceKeys.emplace(device_id, pks);
                                  }

                                  std::vector<std::string> valid_devices;
                                  valid_devices.reserve(room_key_msgs.size());
                                  for (auto const &d : room_key_msgs) {
                                          valid_devices.push_back(d.first);

                                          nhlog::net()->info("{}", d.first);
                                          nhlog::net()->info("  curve25519 {}",
                                                             deviceKeys.at(d.first).curve25519);
                                          nhlog::net()->info("  ed25519 {}",
                                                             deviceKeys.at(d.first).ed25519);
                                  }

                                  nhlog::net()->info(
                                    "sending claim request for user {} with {} devices",
                                    user.first,
                                    valid_devices.size());

                                  http::client()->claim_keys(
                                    user.first,
                                    valid_devices,
                                    std::bind(&TimelineModel::handleClaimedKeys,
                                              this,
                                              keeper,
                                              room_key_msgs,
                                              deviceKeys,
                                              user.first,
                                              std::placeholders::_1,
                                              std::placeholders::_2));

                                  // TODO: Wait before sending the next batch of requests.
                                  std::this_thread::sleep_for(std::chrono::milliseconds(500));
                          }
                  });

                // TODO: Let the user know about the errors.
        } catch (const lmdb::error &e) {
                nhlog::db()->critical(
                  "failed to open outbound megolm session ({}): {}", room_id, e.what());
                emit messageFailed(QString::fromStdString(txn_id));
        } catch (const mtx::crypto::olm_exception &e) {
                nhlog::crypto()->critical(
                  "failed to open outbound megolm session ({}): {}", room_id, e.what());
                emit messageFailed(QString::fromStdString(txn_id));
        }
}

void
TimelineModel::handleClaimedKeys(std::shared_ptr<StateKeeper> keeper,
                                 const std::map<std::string, std::string> &room_keys,
                                 const std::map<std::string, DevicePublicKeys> &pks,
                                 const std::string &user_id,
                                 const mtx::responses::ClaimKeys &res,
                                 mtx::http::RequestErr err)
{
        if (err) {
                nhlog::net()->warn("claim keys error: {} {} {}",
                                   err->matrix_error.error,
                                   err->parse_error,
                                   static_cast<int>(err->status_code));
                return;
        }

        nhlog::net()->debug("claimed keys for {}", user_id);

        if (res.one_time_keys.size() == 0) {
                nhlog::net()->debug("no one-time keys found for user_id: {}", user_id);
                return;
        }

        if (res.one_time_keys.find(user_id) == res.one_time_keys.end()) {
                nhlog::net()->debug("no one-time keys found for user_id: {}", user_id);
                return;
        }

        auto retrieved_devices = res.one_time_keys.at(user_id);

        // Payload with all the to_device message to be sent.
        json body;
        body["messages"][user_id] = json::object();

        for (const auto &rd : retrieved_devices) {
                const auto device_id = rd.first;
                nhlog::net()->debug("{} : \n {}", device_id, rd.second.dump(2));

                // TODO: Verify signatures
                auto otk = rd.second.begin()->at("key");

                if (pks.find(device_id) == pks.end()) {
                        nhlog::net()->critical("couldn't find public key for device: {}",
                                               device_id);
                        continue;
                }

                auto id_key = pks.at(device_id).curve25519;
                auto s      = olm::client()->create_outbound_session(id_key, otk);

                if (room_keys.find(device_id) == room_keys.end()) {
                        nhlog::net()->critical("couldn't find m.room_key for device: {}",
                                               device_id);
                        continue;
                }

                auto device_msg = olm::client()->create_olm_encrypted_content(
                  s.get(), room_keys.at(device_id), pks.at(device_id).curve25519);

                try {
                        cache::saveOlmSession(id_key, std::move(s));
                } catch (const lmdb::error &e) {
                        nhlog::db()->critical("failed to save outbound olm session: {}", e.what());
                } catch (const mtx::crypto::olm_exception &e) {
                        nhlog::crypto()->critical("failed to pickle outbound olm session: {}",
                                                  e.what());
                }

                body["messages"][user_id][device_id] = device_msg;
        }

        nhlog::net()->info("send_to_device: {}", user_id);

        http::client()->send_to_device(
          "m.room.encrypted", body, [keeper](mtx::http::RequestErr err) {
                  if (err) {
                          nhlog::net()->warn("failed to send "
                                             "send_to_device "
                                             "message: {}",
                                             err->matrix_error.error);
                  }

                  (void)keeper;
          });
}

struct SendMessageVisitor
{
        SendMessageVisitor(const QString &txn_id, TimelineModel *model)
          : txn_id_qstr_(txn_id)
          , model_(model)
        {}

        template<typename T>
        void operator()(const mtx::events::Event<T> &)
        {}

        template<typename T,
                 std::enable_if_t<std::is_same<decltype(T::msgtype), std::string>::value, int> = 0>
        void operator()(const mtx::events::RoomEvent<T> &msg)

        {
                if (cache::isRoomEncrypted(model_->room_id_.toStdString())) {
                        model_->sendEncryptedMessage(txn_id_qstr_.toStdString(),
                                                     nlohmann::json(msg.content));
                } else {
                        QString txn_id_qstr  = txn_id_qstr_;
                        TimelineModel *model = model_;
                        http::client()->send_room_message<T, mtx::events::EventType::RoomMessage>(
                          model->room_id_.toStdString(),
                          txn_id_qstr.toStdString(),
                          msg.content,
                          [txn_id_qstr, model](const mtx::responses::EventId &res,
                                               mtx::http::RequestErr err) {
                                  if (err) {
                                          const int status_code =
                                            static_cast<int>(err->status_code);
                                          nhlog::net()->warn("[{}] failed to send message: {} {}",
                                                             txn_id_qstr.toStdString(),
                                                             err->matrix_error.error,
                                                             status_code);
                                          emit model->messageFailed(txn_id_qstr);
                                  }
                                  emit model->messageSent(
                                    txn_id_qstr, QString::fromStdString(res.event_id.to_string()));
                          });
                }
        }

        QString txn_id_qstr_;
        TimelineModel *model_;
};

void
TimelineModel::processOnePendingMessage()
{
        if (isProcessingPending || pending.isEmpty())
                return;

        isProcessingPending = true;

        QString txn_id_qstr = pending.first();

        auto event = events.value(txn_id_qstr);
        std::visit(SendMessageVisitor{txn_id_qstr, this}, event);
}

void
TimelineModel::addPendingMessage(mtx::events::collections::TimelineEvents event)
{
        std::visit(
          [](auto &msg) {
                  msg.type             = mtx::events::EventType::RoomMessage;
                  msg.event_id         = http::client()->generate_txn_id();
                  msg.sender           = http::client()->user_id().to_string();
                  msg.origin_server_ts = QDateTime::currentMSecsSinceEpoch();
          },
          event);

        internalAddEvents({event});

        QString txn_id_qstr = QString::fromStdString(mtx::accessors::event_id(event));
        beginInsertRows(QModelIndex(), 0, 0);
        pending.push_back(txn_id_qstr);
        this->eventOrder.insert(this->eventOrder.begin(), txn_id_qstr);
        endInsertRows();
        updateLastMessage();

        if (!isProcessingPending)
                emit nextPendingMessage();
}

void
TimelineModel::saveMedia(QString eventId) const
{
        mtx::events::collections::TimelineEvents event = events.value(eventId);

        if (auto e =
              std::get_if<mtx::events::EncryptedEvent<mtx::events::msg::Encrypted>>(&event)) {
                event = decryptEvent(*e).event;
        }

        QString mxcUrl           = QString::fromStdString(mtx::accessors::url(event));
        QString originalFilename = QString::fromStdString(mtx::accessors::filename(event));
        QString mimeType         = QString::fromStdString(mtx::accessors::mimetype(event));

        auto encryptionInfo = mtx::accessors::file(event);

        qml_mtx_events::EventType eventType = toRoomEventType(event);

        QString dialogTitle;
        if (eventType == qml_mtx_events::EventType::ImageMessage) {
                dialogTitle = tr("Save image");
        } else if (eventType == qml_mtx_events::EventType::VideoMessage) {
                dialogTitle = tr("Save video");
        } else if (eventType == qml_mtx_events::EventType::AudioMessage) {
                dialogTitle = tr("Save audio");
        } else {
                dialogTitle = tr("Save file");
        }

        QString filterString = QMimeDatabase().mimeTypeForName(mimeType).filterString();

        auto filename = QFileDialog::getSaveFileName(
          manager_->getWidget(), dialogTitle, originalFilename, filterString);

        if (filename.isEmpty())
                return;

        const auto url = mxcUrl.toStdString();

        http::client()->download(
          url,
          [filename, url, encryptionInfo](const std::string &data,
                                          const std::string &,
                                          const std::string &,
                                          mtx::http::RequestErr err) {
                  if (err) {
                          nhlog::net()->warn("failed to retrieve image {}: {} {}",
                                             url,
                                             err->matrix_error.error,
                                             static_cast<int>(err->status_code));
                          return;
                  }

                  try {
                          auto temp = data;
                          if (encryptionInfo)
                                  temp = mtx::crypto::to_string(
                                    mtx::crypto::decrypt_file(temp, encryptionInfo.value()));

                          QFile file(filename);

                          if (!file.open(QIODevice::WriteOnly))
                                  return;

                          file.write(QByteArray(temp.data(), (int)temp.size()));
                          file.close();
                  } catch (const std::exception &e) {
                          nhlog::ui()->warn("Error while saving file to: {}", e.what());
                  }
          });
}

void
TimelineModel::cacheMedia(QString eventId)
{
        mtx::events::collections::TimelineEvents event = events.value(eventId);

        if (auto e =
              std::get_if<mtx::events::EncryptedEvent<mtx::events::msg::Encrypted>>(&event)) {
                event = decryptEvent(*e).event;
        }

        QString mxcUrl           = QString::fromStdString(mtx::accessors::url(event));
        QString originalFilename = QString::fromStdString(mtx::accessors::filename(event));
        QString mimeType         = QString::fromStdString(mtx::accessors::mimetype(event));

        auto encryptionInfo = mtx::accessors::file(event);

        // If the message is a link to a non mxcUrl, don't download it
        if (!mxcUrl.startsWith("mxc://")) {
                emit mediaCached(mxcUrl, mxcUrl);
                return;
        }

        QString suffix = QMimeDatabase().mimeTypeForName(mimeType).preferredSuffix();

        const auto url = mxcUrl.toStdString();
        QFileInfo filename(QString("%1/media_cache/%2.%3")
                             .arg(QStandardPaths::writableLocation(QStandardPaths::CacheLocation))
                             .arg(QString(mxcUrl).remove("mxc://"))
                             .arg(suffix));
        if (QDir::cleanPath(filename.path()) != filename.path()) {
                nhlog::net()->warn("mxcUrl '{}' is not safe, not downloading file", url);
                return;
        }

        QDir().mkpath(filename.path());

        if (filename.isReadable()) {
                emit mediaCached(mxcUrl, filename.filePath());
                return;
        }

        http::client()->download(
          url,
          [this, mxcUrl, filename, url, encryptionInfo](const std::string &data,
                                                        const std::string &,
                                                        const std::string &,
                                                        mtx::http::RequestErr err) {
                  if (err) {
                          nhlog::net()->warn("failed to retrieve image {}: {} {}",
                                             url,
                                             err->matrix_error.error,
                                             static_cast<int>(err->status_code));
                          return;
                  }

                  try {
                          auto temp = data;
                          if (encryptionInfo)
                                  temp = mtx::crypto::to_string(
                                    mtx::crypto::decrypt_file(temp, encryptionInfo.value()));

                          QFile file(filename.filePath());

                          if (!file.open(QIODevice::WriteOnly))
                                  return;

                          file.write(QByteArray(temp.data(), temp.size()));
                          file.close();
                  } catch (const std::exception &e) {
                          nhlog::ui()->warn("Error while saving file to: {}", e.what());
                  }

                  emit mediaCached(mxcUrl, filename.filePath());
          });
}
