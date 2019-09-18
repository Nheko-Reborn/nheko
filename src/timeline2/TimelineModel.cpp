#include "TimelineModel.h"

#include <algorithm>
#include <type_traits>

#include <QRegularExpression>

#include "ChatPage.h"
#include "Logging.h"
#include "MainWindow.h"
#include "Olm.h"
#include "Utils.h"
#include "dialogs/RawMessage.h"

namespace {
template<class T>
QString
eventId(const T &event)
{
        return QString::fromStdString(event.event_id);
}
template<class T>
QString
roomId(const T &event)
{
        return QString::fromStdString(event.room_id);
}
template<class T>
QString
senderId(const T &event)
{
        return QString::fromStdString(event.sender);
}

template<class T>
QDateTime
eventTimestamp(const T &event)
{
        return QDateTime::fromMSecsSinceEpoch(event.origin_server_ts);
}

template<class T>
std::string
eventMsgType(const mtx::events::Event<T> &)
{
        return "";
}
template<class T>
auto
eventMsgType(const mtx::events::RoomEvent<T> &e) -> decltype(e.content.msgtype)
{
        return e.content.msgtype;
}

template<class T>
QString
eventBody(const mtx::events::Event<T> &)
{
        return QString("");
}
template<class T>
auto
eventBody(const mtx::events::RoomEvent<T> &e)
  -> std::enable_if_t<std::is_same<decltype(e.content.body), std::string>::value, QString>
{
        return QString::fromStdString(e.content.body);
}

template<class T>
QString
eventFormattedBody(const mtx::events::Event<T> &)
{
        return QString("");
}
template<class T>
auto
eventFormattedBody(const mtx::events::RoomEvent<T> &e)
  -> std::enable_if_t<std::is_same<decltype(e.content.formatted_body), std::string>::value, QString>
{
        auto temp = e.content.formatted_body;
        if (!temp.empty()) {
                auto pos = temp.find("<mx-reply>");
                if (pos != std::string::npos)
                        temp.erase(pos, std::string("<mx-reply>").size());
                pos = temp.find("</mx-reply>");
                if (pos != std::string::npos)
                        temp.erase(pos, std::string("</mx-reply>").size());
                return QString::fromStdString(temp);
        } else
                return QString::fromStdString(e.content.body);
}

template<class T>
QString
eventUrl(const T &)
{
        return "";
}
template<class T>
auto
eventUrl(const mtx::events::RoomEvent<T> &e)
  -> std::enable_if_t<std::is_same<decltype(e.content.url), std::string>::value, QString>
{
        return QString::fromStdString(e.content.url);
}

template<class T>
qml_mtx_events::EventType
toRoomEventType(const mtx::events::Event<T> &e)
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
        default:
                return qml_mtx_events::EventType::Unsupported;
        }
}
qml_mtx_events::EventType
toRoomEventType(const mtx::events::Event<mtx::events::msg::Audio> &)
{
        return qml_mtx_events::EventType::AudioMessage;
}
qml_mtx_events::EventType
toRoomEventType(const mtx::events::Event<mtx::events::msg::Emote> &)
{
        return qml_mtx_events::EventType::EmoteMessage;
}
qml_mtx_events::EventType
toRoomEventType(const mtx::events::Event<mtx::events::msg::File> &)
{
        return qml_mtx_events::EventType::FileMessage;
}
qml_mtx_events::EventType
toRoomEventType(const mtx::events::Event<mtx::events::msg::Image> &)
{
        return qml_mtx_events::EventType::ImageMessage;
}
qml_mtx_events::EventType
toRoomEventType(const mtx::events::Event<mtx::events::msg::Notice> &)
{
        return qml_mtx_events::EventType::NoticeMessage;
}
qml_mtx_events::EventType
toRoomEventType(const mtx::events::Event<mtx::events::msg::Text> &)
{
        return qml_mtx_events::EventType::TextMessage;
}
qml_mtx_events::EventType
toRoomEventType(const mtx::events::Event<mtx::events::msg::Video> &)
{
        return qml_mtx_events::EventType::VideoMessage;
}

qml_mtx_events::EventType
toRoomEventType(const mtx::events::Event<mtx::events::msg::Redacted> &)
{
        return qml_mtx_events::EventType::Redacted;
}
// ::EventType::Type toRoomEventType(const Event<mtx::events::msg::Location> &e) { return
// ::EventType::LocationMessage; }

template<class T>
uint64_t
eventHeight(const mtx::events::Event<T> &)
{
        return -1;
}
template<class T>
auto
eventHeight(const mtx::events::RoomEvent<T> &e) -> decltype(e.content.info.h)
{
        return e.content.info.h;
}
template<class T>
uint64_t
eventWidth(const mtx::events::Event<T> &)
{
        return -1;
}
template<class T>
auto
eventWidth(const mtx::events::RoomEvent<T> &e) -> decltype(e.content.info.w)
{
        return e.content.info.w;
}

template<class T>
double
eventPropHeight(const mtx::events::RoomEvent<T> &e)
{
        auto w = eventWidth(e);
        if (w == 0)
                w = 1;
        return eventHeight(e) / (double)w;
}
}

TimelineModel::TimelineModel(QString room_id, QObject *parent)
  : QAbstractListModel(parent)
  , room_id_(room_id)
{
        connect(
          this, &TimelineModel::oldMessagesRetrieved, this, &TimelineModel::addBackwardsEvents);
        connect(this, &TimelineModel::messageFailed, this, [this](QString txn_id) {
                pending.remove(txn_id);
                failed.insert(txn_id);
                int idx = idToIndex(txn_id);
                if (idx < 0) {
                        nhlog::ui()->warn("Failed index out of range");
                        return;
                }
                emit dataChanged(index(idx, 0), index(idx, 0));
        });
        connect(this, &TimelineModel::messageSent, this, [this](QString txn_id, QString event_id) {
                int idx = idToIndex(txn_id);
                if (idx < 0) {
                        nhlog::ui()->warn("Sent index out of range");
                        return;
                }
                eventOrder[idx] = event_id;
                auto ev         = events.value(txn_id);
                ev              = boost::apply_visitor(
                  [event_id](const auto &e) -> mtx::events::collections::TimelineEvents {
                          auto eventCopy     = e;
                          eventCopy.event_id = event_id.toStdString();
                          return eventCopy;
                  },
                  ev);
                events.remove(txn_id);
                events.insert(event_id, ev);
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
          {Height, "height"},
          {Width, "width"},
          {ProportionalHeight, "proportionalHeight"},
          {Id, "id"},
          {State, "state"},
        };
}
int
TimelineModel::rowCount(const QModelIndex &parent) const
{
        Q_UNUSED(parent);
        return (int)this->eventOrder.size();
}

QVariant
TimelineModel::data(const QModelIndex &index, int role) const
{
        if (index.row() < 0 && index.row() >= (int)eventOrder.size())
                return QVariant();

        QString id = eventOrder[index.row()];

        mtx::events::collections::TimelineEvents event = events.value(id);

        if (auto e = boost::get<mtx::events::EncryptedEvent<mtx::events::msg::Encrypted>>(&event)) {
                event = decryptEvent(*e).event;
        }

        switch (role) {
        case Section: {
                QDateTime date = boost::apply_visitor(
                  [](const auto &e) -> QDateTime { return eventTimestamp(e); }, event);
                date.setTime(QTime());

                QString userId =
                  boost::apply_visitor([](const auto &e) -> QString { return senderId(e); }, event);

                for (int r = index.row() - 1; r > 0; r--) {
                        QDateTime prevDate = boost::apply_visitor(
                          [](const auto &e) -> QDateTime { return eventTimestamp(e); },
                          events.value(eventOrder[r]));
                        prevDate.setTime(QTime());
                        if (prevDate != date)
                                return QString("%2 %1").arg(date.toMSecsSinceEpoch()).arg(userId);

                        QString prevUserId =
                          boost::apply_visitor([](const auto &e) -> QString { return senderId(e); },
                                               events.value(eventOrder[r]));
                        if (userId != prevUserId)
                                break;
                }

                return QString("%1").arg(userId);
        }
        case UserId:
                return QVariant(boost::apply_visitor(
                  [](const auto &e) -> QString { return senderId(e); }, event));
        case UserName:
                return QVariant(displayName(boost::apply_visitor(
                  [](const auto &e) -> QString { return senderId(e); }, event)));

        case Timestamp:
                return QVariant(boost::apply_visitor(
                  [](const auto &e) -> QDateTime { return eventTimestamp(e); }, event));
        case Type:
                return QVariant(boost::apply_visitor(
                  [](const auto &e) -> qml_mtx_events::EventType { return toRoomEventType(e); },
                  event));
        case Body:
                return QVariant(utils::replaceEmoji(boost::apply_visitor(
                  [](const auto &e) -> QString { return eventBody(e); }, event)));
        case FormattedBody:
                return QVariant(utils::replaceEmoji(boost::apply_visitor(
                  [](const auto &e) -> QString { return eventFormattedBody(e); }, event)));
        case Url:
                return QVariant(boost::apply_visitor(
                  [](const auto &e) -> QString { return eventUrl(e); }, event));
        case Height:
                return QVariant(boost::apply_visitor(
                  [](const auto &e) -> qulonglong { return eventHeight(e); }, event));
        case Width:
                return QVariant(boost::apply_visitor(
                  [](const auto &e) -> qulonglong { return eventWidth(e); }, event));
        case ProportionalHeight:
                return QVariant(boost::apply_visitor(
                  [](const auto &e) -> double { return eventPropHeight(e); }, event));
        case Id:
                return id;
        case State:
                if (failed.contains(id))
                        return qml_mtx_events::Failed;
                else if (pending.contains(id))
                        return qml_mtx_events::Sent;
                else if (read.contains(id))
                        return qml_mtx_events::Read;
                else
                        return qml_mtx_events::Received;
        default:
                return QVariant();
        }
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

        beginInsertRows(QModelIndex(),
                        static_cast<int>(this->eventOrder.size()),
                        static_cast<int>(this->eventOrder.size() + ids.size() - 1));
        this->eventOrder.insert(this->eventOrder.end(), ids.begin(), ids.end());
        endInsertRows();
}

std::vector<QString>
TimelineModel::internalAddEvents(
  const std::vector<mtx::events::collections::TimelineEvents> &timeline)
{
        std::vector<QString> ids;
        for (const auto &e : timeline) {
                QString id =
                  boost::apply_visitor([](const auto &e) -> QString { return eventId(e); }, e);

                if (this->events.contains(id)) {
                        this->events.insert(id, e);
                        int idx = idToIndex(id);
                        emit dataChanged(index(idx, 0), index(idx, 0));
                        continue;
                }

                if (auto redaction =
                      boost::get<mtx::events::RedactionEvent<mtx::events::msg::Redaction>>(&e)) {
                        QString redacts = QString::fromStdString(redaction->redacts);
                        auto redacted   = std::find(eventOrder.begin(), eventOrder.end(), redacts);

                        if (redacted != eventOrder.end()) {
                                auto redactedEvent = boost::apply_visitor(
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

                this->events.insert(id, e);
                ids.push_back(id);
        }
        return ids;
}

void
TimelineModel::fetchHistory()
{
        if (paginationInProgress) {
                nhlog::ui()->warn("Already loading older messages");
                return;
        }

        paginationInProgress = true;
        mtx::http::MessagesOpts opts;
        opts.room_id = room_id_.toStdString();
        opts.from    = prev_batch_token_.toStdString();

        nhlog::ui()->info("Paginationg room {}", opts.room_id);

        http::client()->messages(
          opts, [this, opts](const mtx::responses::Messages &res, mtx::http::RequestErr err) {
                  if (err) {
                          nhlog::net()->error("failed to call /messages ({}): {} - {}",
                                              opts.room_id,
                                              mtx::errors::to_string(err->matrix_error.errcode),
                                              err->matrix_error.error);
                          return;
                  }

                  emit oldMessagesRetrieved(std::move(res));
          });
}

void
TimelineModel::addBackwardsEvents(const mtx::responses::Messages &msgs)
{
        std::vector<QString> ids = internalAddEvents(msgs.chunk);

        if (!ids.empty()) {
                beginInsertRows(QModelIndex(), 0, static_cast<int>(ids.size() - 1));
                this->eventOrder.insert(this->eventOrder.begin(), ids.rbegin(), ids.rend());
                endInsertRows();
        }

        prev_batch_token_ = QString::fromStdString(msgs.end);

        paginationInProgress = false;
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
        return Cache::displayName(room_id_, id);
}

QString
TimelineModel::avatarUrl(QString id) const
{
        return Cache::avatarUrl(room_id_, id);
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
                if (!cache::client()->inboundMegolmSessionExists(index)) {
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
                auto session = cache::client()->getInboundMegolmSession(index);
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

        nhlog::crypto()->debug("decrypted event: {}", e.event_id);

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
        auto event          = events.value(id);
        RelatedInfo related = boost::apply_visitor(
          [](const auto &ev) -> RelatedInfo {
                  RelatedInfo related_   = {};
                  related_.quoted_user   = QString::fromStdString(ev.sender);
                  related_.related_event = ev.event_id;
                  return related_;
          },
          event);
        related.type = mtx::events::getMessageType(boost::apply_visitor(
          [](const auto &e) -> std::string { return eventMsgType(e); }, event));
        related.quoted_body =
          boost::apply_visitor([](const auto &e) -> QString { return eventBody(e); }, event);

        if (related.quoted_body.isEmpty())
                return;

        ChatPage::instance()->messageReply(related);
}

void
TimelineModel::readReceiptsAction(QString id) const
{
        MainWindow::instance()->openReadReceiptsDialog(id);
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
