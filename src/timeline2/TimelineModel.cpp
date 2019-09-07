#include "TimelineModel.h"

#include <type_traits>

#include <QRegularExpression>

#include "Logging.h"
#include "Utils.h"

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
// ::EventType::Type toRoomEventType(const Event<mtx::events::msg::Location> &e) { return
// ::EventType::LocationMessage; }
}

TimelineModel::TimelineModel(QString room_id, QObject *parent)
  : QAbstractListModel(parent)
  , room_id_(room_id)
{
        connect(
          this, &TimelineModel::oldMessagesRetrieved, this, &TimelineModel::addBackwardsEvents);
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

        switch (role) {
        case Section: {
                QDateTime date = boost::apply_visitor(
                  [](const auto &e) -> QDateTime { return eventTimestamp(e); }, events.value(id));
                date.setTime(QTime());

                QString userId = boost::apply_visitor(
                  [](const auto &e) -> QString { return senderId(e); }, events.value(id));

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
                  [](const auto &e) -> QString { return senderId(e); }, events.value(id)));
        case UserName:
                return QVariant(displayName(boost::apply_visitor(
                  [](const auto &e) -> QString { return senderId(e); }, events.value(id))));

        case Timestamp:
                return QVariant(boost::apply_visitor(
                  [](const auto &e) -> QDateTime { return eventTimestamp(e); }, events.value(id)));
        case Type:
                return QVariant(boost::apply_visitor(
                  [](const auto &e) -> qml_mtx_events::EventType { return toRoomEventType(e); },
                  events.value(id)));
        case FormattedBody:
                return QVariant(utils::replaceEmoji(boost::apply_visitor(
                  [](const auto &e) -> QString { return eventFormattedBody(e); },
                  events.value(id))));
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

        std::vector<QString> ids;
        for (const auto &e : timeline.events) {
                QString id =
                  boost::apply_visitor([](const auto &e) -> QString { return eventId(e); }, e);

                this->events.insert(id, e);
                ids.push_back(id);
        }

        beginInsertRows(QModelIndex(),
                        static_cast<int>(this->eventOrder.size()),
                        static_cast<int>(this->eventOrder.size() + ids.size() - 1));
        this->eventOrder.insert(this->eventOrder.end(), ids.begin(), ids.end());
        endInsertRows();
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
        std::vector<QString> ids;
        for (const auto &e : msgs.chunk) {
                QString id =
                  boost::apply_visitor([](const auto &e) -> QString { return eventId(e); }, e);

                this->events.insert(id, e);
                ids.push_back(id);
        }

        beginInsertRows(QModelIndex(), 0, static_cast<int>(ids.size() - 1));
        this->eventOrder.insert(this->eventOrder.begin(), ids.rbegin(), ids.rend());
        endInsertRows();

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
