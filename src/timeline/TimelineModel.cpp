#include "TimelineModel.h"

#include <algorithm>
#include <thread>
#include <type_traits>

#include <QCache>
#include <QFileDialog>
#include <QMimeDatabase>
#include <QRegularExpression>
#include <QSettings>
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

namespace std {
inline uint
qHash(const std::string &key, uint seed = 0)
{
        return qHash(QByteArray::fromRawData(key.data(), key.length()), seed);
}
}

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
                case EventType::Reaction:
                        return qml_mtx_events::EventType::Reaction;
                case EventType::RoomAliases:
                        return qml_mtx_events::EventType::Aliases;
                case EventType::RoomAvatar:
                        return qml_mtx_events::EventType::Avatar;
                case EventType::RoomCanonicalAlias:
                        return qml_mtx_events::EventType::CanonicalAlias;
                case EventType::RoomCreate:
                        return qml_mtx_events::EventType::RoomCreate;
                case EventType::RoomEncrypted:
                        return qml_mtx_events::EventType::Encrypted;
                case EventType::RoomEncryption:
                        return qml_mtx_events::EventType::Encryption;
                case EventType::RoomGuestAccess:
                        return qml_mtx_events::EventType::RoomGuestAccess;
                case EventType::RoomHistoryVisibility:
                        return qml_mtx_events::EventType::RoomHistoryVisibility;
                case EventType::RoomJoinRules:
                        return qml_mtx_events::EventType::RoomJoinRules;
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
        qml_mtx_events::EventType operator()(
          const mtx::events::Event<mtx::events::msg::CallInvite> &)
        {
                return qml_mtx_events::EventType::CallInvite;
        }
        qml_mtx_events::EventType operator()(
          const mtx::events::Event<mtx::events::msg::CallAnswer> &)
        {
                return qml_mtx_events::EventType::CallAnswer;
        }
        qml_mtx_events::EventType operator()(
          const mtx::events::Event<mtx::events::msg::CallHangUp> &)
        {
                return qml_mtx_events::EventType::CallHangUp;
        }
        qml_mtx_events::EventType operator()(
          const mtx::events::Event<mtx::events::msg::CallCandidates> &)
        {
                return qml_mtx_events::EventType::CallCandidates;
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

QString
toRoomEventTypeString(const mtx::events::collections::TimelineEvents &event)
{
        return std::visit([](const auto &e) { return QString::fromStdString(to_string(e.type)); },
                          event);
}

TimelineModel::TimelineModel(TimelineViewManager *manager, QString room_id, QObject *parent)
  : QAbstractListModel(parent)
  , events(room_id.toStdString(), this)
  , room_id_(room_id)
  , manager_(manager)
{
        connect(
          this,
          &TimelineModel::redactionFailed,
          this,
          [](const QString &msg) { emit ChatPage::instance()->showNotification(msg); },
          Qt::QueuedConnection);

        connect(this,
                &TimelineModel::newMessageToSend,
                this,
                &TimelineModel::addPendingMessage,
                Qt::QueuedConnection);
        connect(this, &TimelineModel::addPendingMessageToStore, &events, &EventStore::addPending);

        connect(
          &events,
          &EventStore::dataChanged,
          this,
          [this](int from, int to) {
                  nhlog::ui()->debug(
                    "data changed {} to {}", events.size() - to - 1, events.size() - from - 1);
                  emit dataChanged(index(events.size() - to - 1, 0),
                                   index(events.size() - from - 1, 0));
          },
          Qt::QueuedConnection);

        connect(&events, &EventStore::beginInsertRows, this, [this](int from, int to) {
                int first = events.size() - to;
                int last  = events.size() - from;
                if (from >= events.size()) {
                        int batch_size = to - from;
                        first += batch_size;
                        last += batch_size;
                } else {
                        first -= 1;
                        last -= 1;
                }
                nhlog::ui()->debug("begin insert from {} to {}", first, last);
                beginInsertRows(QModelIndex(), first, last);
        });
        connect(&events, &EventStore::endInsertRows, this, [this]() { endInsertRows(); });
        connect(&events, &EventStore::beginResetModel, this, [this]() { beginResetModel(); });
        connect(&events, &EventStore::endResetModel, this, [this]() { endResetModel(); });
        connect(&events, &EventStore::newEncryptedImage, this, &TimelineModel::newEncryptedImage);
        connect(
          &events, &EventStore::fetchedMore, this, [this]() { setPaginationInProgress(false); });
}

QHash<int, QByteArray>
TimelineModel::roleNames() const
{
        return {
          {Section, "section"},
          {Type, "type"},
          {TypeString, "typeString"},
          {IsOnlyEmoji, "isOnlyEmoji"},
          {Body, "body"},
          {FormattedBody, "formattedBody"},
          {UserId, "userId"},
          {UserName, "userName"},
          {Timestamp, "timestamp"},
          {Url, "url"},
          {ThumbnailUrl, "thumbnailUrl"},
          {Blurhash, "blurhash"},
          {Filename, "filename"},
          {Filesize, "filesize"},
          {MimeType, "mimetype"},
          {Height, "height"},
          {Width, "width"},
          {ProportionalHeight, "proportionalHeight"},
          {Id, "id"},
          {State, "state"},
          {IsEncrypted, "isEncrypted"},
          {IsRoomEncrypted, "isRoomEncrypted"},
          {ReplyTo, "replyTo"},
          {Reactions, "reactions"},
          {RoomId, "roomId"},
          {RoomName, "roomName"},
          {RoomTopic, "roomTopic"},
          {CallType, "callType"},
          {Dump, "dump"},
        };
}
int
TimelineModel::rowCount(const QModelIndex &parent) const
{
        Q_UNUSED(parent);
        return this->events.size();
}

QVariantMap
TimelineModel::getDump(QString eventId, QString relatedTo) const
{
        if (auto event = events.get(eventId.toStdString(), relatedTo.toStdString()))
                return data(*event, Dump).toMap();
        return {};
}

QVariant
TimelineModel::data(const mtx::events::collections::TimelineEvents &event, int role) const
{
        using namespace mtx::accessors;
        namespace acc = mtx::accessors;

        switch (role) {
        case UserId:
                return QVariant(QString::fromStdString(acc::sender(event)));
        case UserName:
                return QVariant(displayName(QString::fromStdString(acc::sender(event))));

        case Timestamp:
                return QVariant(origin_server_ts(event));
        case Type:
                return QVariant(toRoomEventType(event));
        case TypeString:
                return QVariant(toRoomEventTypeString(event));
        case IsOnlyEmoji: {
                QString qBody = QString::fromStdString(body(event));

                QVector<uint> utf32_string = qBody.toUcs4();
                int emojiCount             = 0;

                for (auto &code : utf32_string) {
                        if (utils::codepointIsEmoji(code)) {
                                emojiCount++;
                        } else {
                                return QVariant(0);
                        }
                }

                return QVariant(emojiCount);
        }
        case Body:
                return QVariant(utils::replaceEmoji(QString::fromStdString(body(event))));
        case FormattedBody: {
                const static QRegularExpression replyFallback(
                  "<mx-reply>.*</mx-reply>", QRegularExpression::DotMatchesEverythingOption);

                bool isReply = !in_reply_to_event(event).empty();

                auto formattedBody_ = QString::fromStdString(formatted_body(event));
                if (formattedBody_.isEmpty()) {
                        auto body_ = QString::fromStdString(body(event));

                        if (isReply) {
                                while (body_.startsWith("> "))
                                        body_ = body_.right(body_.size() - body_.indexOf('\n') - 1);
                                if (body_.startsWith('\n'))
                                        body_ = body_.right(body_.size() - 1);
                        }
                        formattedBody_ = body_.toHtmlEscaped().replace('\n', "<br>");
                } else {
                        if (isReply)
                                formattedBody_ = formattedBody_.remove(replyFallback);
                }

                formattedBody_.replace("<img src=\"mxc:&#47;&#47;", "<img src=\"image://mxcImage/");
                formattedBody_.replace("<img src=\"mxc://", "<img src=\"image://mxcImage/");

                return QVariant(utils::replaceEmoji(
                  utils::linkifyMessage(utils::escapeBlacklistedHtml(formattedBody_))));
        }
        case Url:
                return QVariant(QString::fromStdString(url(event)));
        case ThumbnailUrl:
                return QVariant(QString::fromStdString(thumbnail_url(event)));
        case Blurhash:
                return QVariant(QString::fromStdString(blurhash(event)));
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
                return QVariant(QString::fromStdString(event_id(event)));
        case State: {
                auto id             = QString::fromStdString(event_id(event));
                auto containsOthers = [](const auto &vec) {
                        for (const auto &e : vec)
                                if (e.second != http::client()->user_id().to_string())
                                        return true;
                        return false;
                };

                // only show read receipts for messages not from us
                if (acc::sender(event) != http::client()->user_id().to_string())
                        return qml_mtx_events::Empty;
                else if (!id.isEmpty() && id[0] == "m")
                        return qml_mtx_events::Sent;
                else if (read.contains(id) || containsOthers(cache::readReceipts(id, room_id_)))
                        return qml_mtx_events::Read;
                else
                        return qml_mtx_events::Received;
        }
        case IsEncrypted: {
                auto id              = event_id(event);
                auto encrypted_event = events.get(id, id, false);
                return encrypted_event &&
                       std::holds_alternative<
                         mtx::events::EncryptedEvent<mtx::events::msg::Encrypted>>(
                         *encrypted_event);
        }
        case IsRoomEncrypted: {
                return cache::isRoomEncrypted(room_id_.toStdString());
        }
        case ReplyTo:
                return QVariant(QString::fromStdString(in_reply_to_event(event)));
        case Reactions: {
                auto id = event_id(event);
                return QVariant::fromValue(events.reactions(id));
        }
        case RoomId:
                return QVariant(room_id_);
        case RoomName:
                return QVariant(QString::fromStdString(room_name(event)));
        case RoomTopic:
                return QVariant(QString::fromStdString(room_topic(event)));
        case CallType:
                return QVariant(QString::fromStdString(call_type(event)));
        case Dump: {
                QVariantMap m;
                auto names = roleNames();

                // m.insert(names[Section], data(id, static_cast<int>(Section)));
                m.insert(names[Type], data(event, static_cast<int>(Type)));
                m.insert(names[TypeString], data(event, static_cast<int>(TypeString)));
                m.insert(names[IsOnlyEmoji], data(event, static_cast<int>(IsOnlyEmoji)));
                m.insert(names[Body], data(event, static_cast<int>(Body)));
                m.insert(names[FormattedBody], data(event, static_cast<int>(FormattedBody)));
                m.insert(names[UserId], data(event, static_cast<int>(UserId)));
                m.insert(names[UserName], data(event, static_cast<int>(UserName)));
                m.insert(names[Timestamp], data(event, static_cast<int>(Timestamp)));
                m.insert(names[Url], data(event, static_cast<int>(Url)));
                m.insert(names[ThumbnailUrl], data(event, static_cast<int>(ThumbnailUrl)));
                m.insert(names[Blurhash], data(event, static_cast<int>(Blurhash)));
                m.insert(names[Filename], data(event, static_cast<int>(Filename)));
                m.insert(names[Filesize], data(event, static_cast<int>(Filesize)));
                m.insert(names[MimeType], data(event, static_cast<int>(MimeType)));
                m.insert(names[Height], data(event, static_cast<int>(Height)));
                m.insert(names[Width], data(event, static_cast<int>(Width)));
                m.insert(names[ProportionalHeight],
                         data(event, static_cast<int>(ProportionalHeight)));
                m.insert(names[Id], data(event, static_cast<int>(Id)));
                m.insert(names[State], data(event, static_cast<int>(State)));
                m.insert(names[IsEncrypted], data(event, static_cast<int>(IsEncrypted)));
                m.insert(names[IsRoomEncrypted], data(event, static_cast<int>(IsRoomEncrypted)));
                m.insert(names[ReplyTo], data(event, static_cast<int>(ReplyTo)));
                m.insert(names[RoomName], data(event, static_cast<int>(RoomName)));
                m.insert(names[RoomTopic], data(event, static_cast<int>(RoomTopic)));
                m.insert(names[CallType], data(event, static_cast<int>(CallType)));

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
        if (index.row() < 0 && index.row() >= rowCount())
                return QVariant();

        auto event = events.get(rowCount() - index.row() - 1);

        if (!event)
                return "";

        if (role == Section) {
                QDateTime date = origin_server_ts(*event);
                date.setTime(QTime());

                std::string userId = acc::sender(*event);

                for (int r = rowCount() - index.row(); r < events.size(); r++) {
                        auto tempEv = events.get(r);
                        if (!tempEv)
                                break;

                        QDateTime prevDate = origin_server_ts(*tempEv);
                        prevDate.setTime(QTime());
                        if (prevDate != date)
                                return QString("%2 %1")
                                  .arg(date.toMSecsSinceEpoch())
                                  .arg(QString::fromStdString(userId));

                        std::string prevUserId = acc::sender(*tempEv);
                        if (userId != prevUserId)
                                break;
                }

                return QString("%1").arg(QString::fromStdString(userId));
        }

        return data(*event, role);
}

bool
TimelineModel::canFetchMore(const QModelIndex &) const
{
        if (!events.size())
                return true;
        if (auto first = events.get(0);
            first &&
            !std::holds_alternative<mtx::events::StateEvent<mtx::events::state::Create>>(*first))
                return true;
        else

                return false;
}

void
TimelineModel::setPaginationInProgress(const bool paginationInProgress)
{
        if (m_paginationInProgress == paginationInProgress) {
                return;
        }

        m_paginationInProgress = paginationInProgress;
        emit paginationInProgressChanged(m_paginationInProgress);
}

void
TimelineModel::fetchMore(const QModelIndex &)
{
        if (m_paginationInProgress) {
                nhlog::ui()->warn("Already loading older messages");
                return;
        }

        setPaginationInProgress(true);

        events.fetchMore();
}

void
TimelineModel::syncState(const mtx::responses::State &s)
{
        using namespace mtx::events;

        for (const auto &e : s.events) {
                if (std::holds_alternative<StateEvent<state::Avatar>>(e))
                        emit roomAvatarUrlChanged();
                else if (std::holds_alternative<StateEvent<state::Name>>(e))
                        emit roomNameChanged();
                else if (std::holds_alternative<StateEvent<state::Topic>>(e))
                        emit roomTopicChanged();
                else if (std::holds_alternative<StateEvent<state::Member>>(e)) {
                        emit roomAvatarUrlChanged();
                        emit roomNameChanged();
                }
        }
}

void
TimelineModel::addEvents(const mtx::responses::Timeline &timeline)
{
        if (timeline.events.empty())
                return;

        events.handleSync(timeline);

        using namespace mtx::events;

        for (auto e : timeline.events) {
                if (auto encryptedEvent = std::get_if<EncryptedEvent<msg::Encrypted>>(&e)) {
                        MegolmSessionIndex index;
                        index.room_id    = room_id_.toStdString();
                        index.session_id = encryptedEvent->content.session_id;
                        index.sender_key = encryptedEvent->content.sender_key;

                        auto result = olm::decryptEvent(index, *encryptedEvent);
                        if (result.event)
                                e = result.event.value();
                }

                if (std::holds_alternative<RoomEvent<msg::CallCandidates>>(e) ||
                    std::holds_alternative<RoomEvent<msg::CallInvite>>(e) ||
                    std::holds_alternative<RoomEvent<msg::CallAnswer>>(e) ||
                    std::holds_alternative<RoomEvent<msg::CallHangUp>>(e))
                        std::visit(
                          [this](auto &event) {
                                  event.room_id = room_id_.toStdString();
                                  if (event.sender != http::client()->user_id().to_string())
                                          emit newCallEvent(event);
                          },
                          e);
                else if (std::holds_alternative<StateEvent<state::Avatar>>(e))
                        emit roomAvatarUrlChanged();
                else if (std::holds_alternative<StateEvent<state::Name>>(e))
                        emit roomNameChanged();
                else if (std::holds_alternative<StateEvent<state::Topic>>(e))
                        emit roomTopicChanged();
                else if (std::holds_alternative<StateEvent<state::Member>>(e)) {
                        emit roomAvatarUrlChanged();
                        emit roomNameChanged();
                }
        }
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

template<typename T>
auto
isMessage(const mtx::events::EncryptedEvent<T> &)
{
        return true;
}

auto
isMessage(const mtx::events::RoomEvent<mtx::events::msg::CallInvite> &)
{
        return true;
}

auto
isMessage(const mtx::events::RoomEvent<mtx::events::msg::CallAnswer> &)
{
        return true;
}
auto
isMessage(const mtx::events::RoomEvent<mtx::events::msg::CallHangUp> &)
{
        return true;
}

// Workaround. We also want to see a room at the top, if we just joined it
auto
isYourJoin(const mtx::events::StateEvent<mtx::events::state::Member> &e)
{
        return e.content.membership == mtx::events::state::Membership::Join &&
               e.state_key == http::client()->user_id().to_string();
}
template<typename T>
auto
isYourJoin(const mtx::events::Event<T> &)
{
        return false;
}

void
TimelineModel::updateLastMessage()
{
        for (auto it = events.size() - 1; it >= 0; --it) {
                auto event = events.get(it, decryptDescription);
                if (!event)
                        continue;

                if (std::visit([](const auto &e) -> bool { return isYourJoin(e); }, *event)) {
                        auto time   = mtx::accessors::origin_server_ts(*event);
                        uint64_t ts = time.toMSecsSinceEpoch();
                        emit manager_->updateRoomsLastMessage(
                          room_id_,
                          DescInfo{QString::fromStdString(mtx::accessors::event_id(*event)),
                                   QString::fromStdString(http::client()->user_id().to_string()),
                                   tr("You joined this room."),
                                   utils::descriptiveTime(time),
                                   ts,
                                   time});
                        return;
                }
                if (!std::visit([](const auto &e) -> bool { return isMessage(e); }, *event))
                        continue;

                auto description = utils::getMessageDescription(
                  *event, QString::fromStdString(http::client()->user_id().to_string()), room_id_);
                emit manager_->updateRoomsLastMessage(room_id_, description);
                return;
        }
}

void
TimelineModel::setCurrentIndex(int index)
{
        auto oldIndex = idToIndex(currentId);
        currentId     = indexToId(index);
        emit currentIndexChanged(index);

        if ((oldIndex > index || oldIndex == -1) && !currentId.startsWith("m") &&
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

QString
TimelineModel::displayName(QString id) const
{
        return cache::displayName(room_id_, id).toHtmlEscaped();
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
        auto e = events.get(id.toStdString(), "", false);
        if (!e)
                return;
        std::string ev = mtx::accessors::serialize_event(*e).dump(4);
        auto dialog    = new dialogs::RawMessage(QString::fromStdString(ev));
        Q_UNUSED(dialog);
}

void
TimelineModel::viewDecryptedRawMessage(QString id) const
{
        auto e = events.get(id.toStdString(), "");
        if (!e)
                return;

        std::string ev = mtx::accessors::serialize_event(*e).dump(4);
        auto dialog    = new dialogs::RawMessage(QString::fromStdString(ev));
        Q_UNUSED(dialog);
}

void
TimelineModel::openUserProfile(QString userid) const
{
        MainWindow::instance()->openUserProfile(userid, room_id_);
}

void
TimelineModel::replyAction(QString id)
{
        setReply(id);
        ChatPage::instance()->focusMessageInput();
}

RelatedInfo
TimelineModel::relatedInfo(QString id)
{
        auto event = events.get(id.toStdString(), "");
        if (!event)
                return {};

        RelatedInfo related   = {};
        related.quoted_user   = QString::fromStdString(mtx::accessors::sender(*event));
        related.related_event = mtx::accessors::event_id(*event);
        related.type          = mtx::accessors::msg_type(*event);

        // get body, strip reply fallback, then transform the event to text, if it is a media event
        // etc
        related.quoted_body = QString::fromStdString(mtx::accessors::body(*event));
        QRegularExpression plainQuote("^>.*?$\n?", QRegularExpression::MultilineOption);
        while (related.quoted_body.startsWith(">"))
                related.quoted_body.remove(plainQuote);
        if (related.quoted_body.startsWith("\n"))
                related.quoted_body.remove(0, 1);
        related.quoted_body = utils::getQuoteBody(related);

        // get quoted body and strip reply fallback
        related.quoted_formatted_body = mtx::accessors::formattedBodyWithFallback(*event);
        related.quoted_formatted_body.remove(QRegularExpression(
          "<mx-reply>.*</mx-reply>", QRegularExpression::DotMatchesEverythingOption));
        related.room = room_id_;

        return related;
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

        auto idx = events.idToIndex(id.toStdString());
        if (idx)
                return events.size() - *idx - 1;
        else
                return -1;
}

QString
TimelineModel::indexToId(int index) const
{
        auto id = events.indexToId(events.size() - index - 1);
        return id ? QString::fromStdString(*id) : "";
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
TimelineModel::sendEncryptedMessageEvent(const std::string &txn_id,
                                         nlohmann::json content,
                                         mtx::events::EventType eventType)
{
        const auto room_id = room_id_.toStdString();

        using namespace mtx::events;
        using namespace mtx::identifiers;

        json doc = {
          {"type", mtx::events::to_string(eventType)}, {"content", content}, {"room_id", room_id}};

        try {
                // Check if we have already an outbound megolm session then we can use.
                if (cache::outboundMegolmSessionExists(room_id)) {
                        mtx::events::EncryptedEvent<mtx::events::msg::Encrypted> event;
                        event.content =
                          olm::encrypt_group_message(room_id, http::client()->device_id(), doc);
                        event.event_id         = txn_id;
                        event.room_id          = room_id;
                        event.sender           = http::client()->user_id().to_string();
                        event.type             = mtx::events::EventType::RoomEncrypted;
                        event.origin_server_ts = QDateTime::currentMSecsSinceEpoch();

                        emit this->addPendingMessageToStore(event);
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

                auto keeper = std::make_shared<StateKeeper>([room_id, doc, txn_id, this]() {
                        try {
                                mtx::events::EncryptedEvent<mtx::events::msg::Encrypted> event;
                                event.content = olm::encrypt_group_message(
                                  room_id, http::client()->device_id(), doc);
                                event.event_id         = txn_id;
                                event.room_id          = room_id;
                                event.sender           = http::client()->user_id().to_string();
                                event.type             = mtx::events::EventType::RoomEncrypted;
                                event.origin_server_ts = QDateTime::currentMSecsSinceEpoch();

                                emit this->addPendingMessageToStore(event);
                        } catch (const lmdb::error &e) {
                                nhlog::db()->critical("failed to save megolm outbound session: {}",
                                                      e.what());
                                emit ChatPage::instance()->showNotification(
                                  tr("Failed to encrypt event, sending aborted!"));
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
                                  emit ChatPage::instance()->showNotification(
                                    tr("Failed to encrypt event, sending aborted!"));
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
                emit ChatPage::instance()->showNotification(
                  tr("Failed to encrypt event, sending aborted!"));
        } catch (const mtx::crypto::olm_exception &e) {
                nhlog::crypto()->critical(
                  "failed to open outbound megolm session ({}): {}", room_id, e.what());
                emit ChatPage::instance()->showNotification(
                  tr("Failed to encrypt event, sending aborted!"));
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
        explicit SendMessageVisitor(TimelineModel *model)
          : model_(model)
        {}

        template<typename T, mtx::events::EventType Event>
        void sendRoomEvent(mtx::events::RoomEvent<T> msg)
        {
                if (cache::isRoomEncrypted(model_->room_id_.toStdString())) {
                        auto encInfo = mtx::accessors::file(msg);
                        if (encInfo)
                                emit model_->newEncryptedImage(encInfo.value());

                        model_->sendEncryptedMessageEvent(
                          msg.event_id, nlohmann::json(msg.content), Event);
                } else {
                        msg.type = Event;
                        emit model_->addPendingMessageToStore(msg);
                }
        }

        // Do-nothing operator for all unhandled events
        template<typename T>
        void operator()(const mtx::events::Event<T> &)
        {}

        // Operator for m.room.message events that contain a msgtype in their content
        template<typename T,
                 std::enable_if_t<std::is_same<decltype(T::msgtype), std::string>::value, int> = 0>
        void operator()(mtx::events::RoomEvent<T> msg)
        {
                sendRoomEvent<T, mtx::events::EventType::RoomMessage>(msg);
        }

        // Special operator for reactions, which are a type of m.room.message, but need to be
        // handled distinctly for their differences from normal room messages.  Specifically,
        // reactions need to have the relation outside of ciphertext, or synapse / the homeserver
        // cannot handle it correctly.  See the MSC for more details:
        // https://github.com/matrix-org/matrix-doc/blob/matthew/msc1849/proposals/1849-aggregations.md#end-to-end-encryption
        void operator()(mtx::events::RoomEvent<mtx::events::msg::Reaction> msg)
        {
                msg.type = mtx::events::EventType::Reaction;
                emit model_->addPendingMessageToStore(msg);
        }

        void operator()(const mtx::events::RoomEvent<mtx::events::msg::CallInvite> &event)
        {
                sendRoomEvent<mtx::events::msg::CallInvite, mtx::events::EventType::CallInvite>(
                  event);
        }

        void operator()(const mtx::events::RoomEvent<mtx::events::msg::CallCandidates> &event)
        {
                sendRoomEvent<mtx::events::msg::CallCandidates,
                              mtx::events::EventType::CallCandidates>(event);
        }

        void operator()(const mtx::events::RoomEvent<mtx::events::msg::CallAnswer> &event)
        {
                sendRoomEvent<mtx::events::msg::CallAnswer, mtx::events::EventType::CallAnswer>(
                  event);
        }

        void operator()(const mtx::events::RoomEvent<mtx::events::msg::CallHangUp> &event)
        {
                sendRoomEvent<mtx::events::msg::CallHangUp, mtx::events::EventType::CallHangUp>(
                  event);
        }

        TimelineModel *model_;
};

void
TimelineModel::addPendingMessage(mtx::events::collections::TimelineEvents event)
{
        std::visit(
          [](auto &msg) {
                  msg.type             = mtx::events::EventType::RoomMessage;
                  msg.event_id         = "m" + http::client()->generate_txn_id();
                  msg.sender           = http::client()->user_id().to_string();
                  msg.origin_server_ts = QDateTime::currentMSecsSinceEpoch();
          },
          event);

        std::visit(SendMessageVisitor{this}, event);
}

bool
TimelineModel::saveMedia(QString eventId) const
{
        mtx::events::collections::TimelineEvents *event = events.get(eventId.toStdString(), "");
        if (!event)
                return false;

        QString mxcUrl           = QString::fromStdString(mtx::accessors::url(*event));
        QString originalFilename = QString::fromStdString(mtx::accessors::filename(*event));
        QString mimeType         = QString::fromStdString(mtx::accessors::mimetype(*event));

        auto encryptionInfo = mtx::accessors::file(*event);

        qml_mtx_events::EventType eventType = toRoomEventType(*event);

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

        const QString filterString = QMimeDatabase().mimeTypeForName(mimeType).filterString();
        const QString downloadsFolder =
          QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        const QString openLocation = downloadsFolder + "/" + originalFilename;

        const QString filename = QFileDialog::getSaveFileName(
          manager_->getWidget(), dialogTitle, openLocation, filterString);

        if (filename.isEmpty())
                return false;

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

                          return;
                  } catch (const std::exception &e) {
                          nhlog::ui()->warn("Error while saving file to: {}", e.what());
                  }
          });
        return true;
}

void
TimelineModel::cacheMedia(QString eventId)
{
        mtx::events::collections::TimelineEvents *event = events.get(eventId.toStdString(), "");
        if (!event)
                return;

        QString mxcUrl           = QString::fromStdString(mtx::accessors::url(*event));
        QString originalFilename = QString::fromStdString(mtx::accessors::filename(*event));
        QString mimeType         = QString::fromStdString(mtx::accessors::mimetype(*event));

        auto encryptionInfo = mtx::accessors::file(*event);

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

QString
TimelineModel::formatTypingUsers(const std::vector<QString> &users, QColor bg)
{
        QString temp =
          tr("%1 and %2 are typing.",
             "Multiple users are typing. First argument is a comma separated list of potentially "
             "multiple users. Second argument is the last user of that list. (If only one user is "
             "typing, %1 is empty. You should still use it in your string though to silence Qt "
             "warnings.)",
             users.size());

        if (users.empty()) {
                return "";
        }

        QStringList uidWithoutLast;

        auto formatUser = [this, bg](const QString &user_id) -> QString {
                auto uncoloredUsername = escapeEmoji(displayName(user_id));
                QString prefix =
                  QString("<font color=\"%1\">").arg(manager_->userColor(user_id, bg).name());

                // color only parts that don't have a font already specified
                QString coloredUsername;
                int index = 0;
                do {
                        auto startIndex = uncoloredUsername.indexOf("<font", index);

                        if (startIndex - index != 0)
                                coloredUsername +=
                                  prefix +
                                  uncoloredUsername.midRef(
                                    index, startIndex > 0 ? startIndex - index : -1) +
                                  "</font>";

                        auto endIndex = uncoloredUsername.indexOf("</font>", startIndex);
                        if (endIndex > 0)
                                endIndex += sizeof("</font>") - 1;

                        if (endIndex - startIndex != 0)
                                coloredUsername +=
                                  uncoloredUsername.midRef(startIndex, endIndex - startIndex);

                        index = endIndex;
                } while (index > 0 && index < uncoloredUsername.size());

                return coloredUsername;
        };

        for (size_t i = 0; i + 1 < users.size(); i++) {
                uidWithoutLast.append(formatUser(users[i]));
        }

        return temp.arg(uidWithoutLast.join(", ")).arg(formatUser(users.back()));
}

QString
TimelineModel::formatJoinRuleEvent(QString id)
{
        mtx::events::collections::TimelineEvents *e = events.get(id.toStdString(), "");
        if (!e)
                return "";

        auto event = std::get_if<mtx::events::StateEvent<mtx::events::state::JoinRules>>(e);
        if (!event)
                return "";

        QString user = QString::fromStdString(event->sender);
        QString name = escapeEmoji(displayName(user));

        switch (event->content.join_rule) {
        case mtx::events::state::JoinRule::Public:
                return tr("%1 opened the room to the public.").arg(name);
        case mtx::events::state::JoinRule::Invite:
                return tr("%1 made this room require and invitation to join.").arg(name);
        default:
                // Currently, knock and private are reserved keywords and not implemented in Matrix.
                return "";
        }
}

QString
TimelineModel::formatGuestAccessEvent(QString id)
{
        mtx::events::collections::TimelineEvents *e = events.get(id.toStdString(), "");
        if (!e)
                return "";

        auto event = std::get_if<mtx::events::StateEvent<mtx::events::state::GuestAccess>>(e);
        if (!event)
                return "";

        QString user = QString::fromStdString(event->sender);
        QString name = escapeEmoji(displayName(user));

        switch (event->content.guest_access) {
        case mtx::events::state::AccessState::CanJoin:
                return tr("%1 made the room open to guests.").arg(name);
        case mtx::events::state::AccessState::Forbidden:
                return tr("%1 has closed the room to guest access.").arg(name);
        default:
                return "";
        }
}

QString
TimelineModel::formatHistoryVisibilityEvent(QString id)
{
        mtx::events::collections::TimelineEvents *e = events.get(id.toStdString(), "");
        if (!e)
                return "";

        auto event = std::get_if<mtx::events::StateEvent<mtx::events::state::HistoryVisibility>>(e);

        if (!event)
                return "";

        QString user = QString::fromStdString(event->sender);
        QString name = escapeEmoji(displayName(user));

        switch (event->content.history_visibility) {
        case mtx::events::state::Visibility::WorldReadable:
                return tr("%1 made the room history world readable. Events may be now read by "
                          "non-joined people.")
                  .arg(name);
        case mtx::events::state::Visibility::Shared:
                return tr("%1 set the room history visible to members from this point on.")
                  .arg(name);
        case mtx::events::state::Visibility::Invited:
                return tr("%1 set the room history visible to members since they were invited.")
                  .arg(name);
        case mtx::events::state::Visibility::Joined:
                return tr("%1 set the room history visible to members since they joined the room.")
                  .arg(name);
        default:
                return "";
        }
}

QString
TimelineModel::formatPowerLevelEvent(QString id)
{
        mtx::events::collections::TimelineEvents *e = events.get(id.toStdString(), "");
        if (!e)
                return "";

        auto event = std::get_if<mtx::events::StateEvent<mtx::events::state::PowerLevels>>(e);
        if (!event)
                return "";

        QString user = QString::fromStdString(event->sender);
        QString name = escapeEmoji(displayName(user));

        // TODO: power levels rendering is actually a bit complex. work on this later.
        return tr("%1 has changed the room's permissions.").arg(name);
}

QString
TimelineModel::formatMemberEvent(QString id)
{
        mtx::events::collections::TimelineEvents *e = events.get(id.toStdString(), "");
        if (!e)
                return "";

        auto event = std::get_if<mtx::events::StateEvent<mtx::events::state::Member>>(e);
        if (!event)
                return "";

        mtx::events::StateEvent<mtx::events::state::Member> *prevEvent = nullptr;
        if (!event->unsigned_data.replaces_state.empty()) {
                auto tempPrevEvent =
                  events.get(event->unsigned_data.replaces_state, event->event_id);
                if (tempPrevEvent) {
                        prevEvent =
                          std::get_if<mtx::events::StateEvent<mtx::events::state::Member>>(
                            tempPrevEvent);
                }
        }

        QString user = QString::fromStdString(event->state_key);
        QString name = escapeEmoji(displayName(user));
        QString rendered;

        // see table https://matrix.org/docs/spec/client_server/latest#m-room-member
        using namespace mtx::events::state;
        switch (event->content.membership) {
        case Membership::Invite:
                rendered = tr("%1 was invited.").arg(name);
                break;
        case Membership::Join:
                if (prevEvent && prevEvent->content.membership == Membership::Join) {
                        bool displayNameChanged =
                          prevEvent->content.display_name != event->content.display_name;
                        bool avatarChanged =
                          prevEvent->content.avatar_url != event->content.avatar_url;

                        if (displayNameChanged && avatarChanged)
                                rendered =
                                  tr("%1 changed their display name and avatar.").arg(name);
                        else if (displayNameChanged)
                                rendered = tr("%1 changed their display name.").arg(name);
                        else if (avatarChanged)
                                rendered = tr("%1 changed their avatar.").arg(name);
                        else
                                rendered = tr("%1 changed some profile info.").arg(name);
                        // the case of nothing changed but join follows join shouldn't happen, so
                        // just show it as join
                } else {
                        rendered = tr("%1 joined.").arg(name);
                }
                break;
        case Membership::Leave:
                if (!prevEvent) // Should only ever happen temporarily
                        return "";

                if (prevEvent->content.membership == Membership::Invite) {
                        if (event->state_key == event->sender)
                                rendered = tr("%1 rejected their invite.").arg(name);
                        else
                                rendered = tr("Revoked the invite to %1.").arg(name);
                } else if (prevEvent->content.membership == Membership::Join) {
                        if (event->state_key == event->sender)
                                rendered = tr("%1 left the room.").arg(name);
                        else
                                rendered = tr("Kicked %1.").arg(name);
                } else if (prevEvent->content.membership == Membership::Ban) {
                        rendered = tr("Unbanned %1.").arg(name);
                } else if (prevEvent->content.membership == Membership::Knock) {
                        if (event->state_key == event->sender)
                                rendered = tr("%1 redacted their knock.").arg(name);
                        else
                                rendered = tr("Rejected the knock from %1.").arg(name);
                } else
                        return tr("%1 left after having already left!",
                                  "This is a leave event after the user already left and shouldn't "
                                  "happen apart from state resets")
                          .arg(name);
                break;

        case Membership::Ban:
                rendered = tr("%1 was banned.").arg(name);
                break;
        case Membership::Knock:
                rendered = tr("%1 knocked.").arg(name);
                break;
        }

        if (event->content.reason != "") {
                rendered += tr(" Reason: %1").arg(QString::fromStdString(event->content.reason));
        }

        return rendered;
}

QString
TimelineModel::roomName() const
{
        auto info = cache::getRoomInfo({room_id_.toStdString()});

        if (!info.count(room_id_))
                return "";
        else
                return QString::fromStdString(info[room_id_].name);
}

QString
TimelineModel::roomAvatarUrl() const
{
        auto info = cache::getRoomInfo({room_id_.toStdString()});

        if (!info.count(room_id_))
                return "";
        else
                return QString::fromStdString(info[room_id_].avatar_url);
}

QString
TimelineModel::roomTopic() const
{
        auto info = cache::getRoomInfo({room_id_.toStdString()});

        if (!info.count(room_id_))
                return "";
        else
                return utils::replaceEmoji(utils::linkifyMessage(
                  utils::escapeBlacklistedHtml(QString::fromStdString(info[room_id_].topic))));
}
