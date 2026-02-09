// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include <QColor>
#include <QDate>
#include <QDateTime>
#include <QSet>
#include <QTimer>
#include <QVariant>

#include <mtx/responses/common.hpp>
#include <mtxclient/http/errors.hpp>

#include "CacheCryptoStructs.h"
#include "CacheStructs.h"
#include "EventStore.h"
#include "InputBar.h"
#include "Permissions.h"
#include "Reaction.h"
#include "ui/RoomSummary.h"

namespace mtx::http {
using RequestErr = const std::optional<mtx::http::ClientError> &;
}
namespace mtx::responses {
struct Timeline;
struct Messages;
struct ClaimKeys;
struct StateEvents;
}
struct RelatedInfo;
class ReadReceiptsProxy;

namespace qml_mtx_events {
Q_NAMESPACE
QML_NAMED_ELEMENT(MtxEvent)

enum EventType
{
    // Unsupported event
    Unsupported,
    /// m.room_key_request
    KeyRequest,
    /// m.reaction,
    Reaction,
    /// m.room.aliases
    Aliases,
    /// m.room.avatar
    Avatar,
    /// m.call.invite
    CallInvite,
    /// m.call.answer
    CallAnswer,
    /// m.call.hangup
    CallHangUp,
    /// m.call.candidates
    CallCandidates,
    /// m.call.select_answer
    CallSelectAnswer,
    /// m.call.reject
    CallReject,
    /// m.call.negotiate
    CallNegotiate,
    /// m.room.canonical_alias
    CanonicalAlias,
    /// m.room.create
    RoomCreate,
    /// m.room.encrypted.
    Encrypted,
    /// m.room.encryption.
    Encryption,
    /// m.room.guest_access
    RoomGuestAccess,
    /// m.room.history_visibility
    RoomHistoryVisibility,
    /// m.room.join_rules
    RoomJoinRules,
    /// m.room.member
    Member,
    /// m.room.name
    Name,
    /// m.room.power_levels
    PowerLevels,
    /// m.room.tombstone
    Tombstone,
    /// m.room.server_acl
    ServerAcl,
    /// m.room.topic
    Topic,
    /// m.room.redaction
    Redaction,
    /// m.room.pinned_events
    PinnedEvents,
    // m.sticker
    Sticker,
    // m.tag
    Tag,
    // m.widget
    Widget,
    /// m.room.message
    AudioMessage,
    ElementEffectMessage,
    EmoteMessage,
    FileMessage,
    ImageMessage,
    LocationMessage,
    NoticeMessage,
    TextMessage,
    UnknownMessage,
    VideoMessage,
    Redacted,
    UnknownEvent,
    KeyVerificationRequest,
    KeyVerificationStart,
    KeyVerificationMac,
    KeyVerificationAccept,
    KeyVerificationCancel,
    KeyVerificationKey,
    KeyVerificationDone,
    KeyVerificationReady,
    //! m.image_pack, currently im.ponies.room_emotes
    ImagePackInRoom,
    //! m.image_pack, currently im.ponies.user_emotes
    ImagePackInAccountData,
    //! m.image_pack.rooms, currently im.ponies.emote_rooms
    ImagePackRooms,
    // m.policy.rule.user
    PolicyRuleUser,
    // m.policy.rule.room
    PolicyRuleRoom,
    // m.policy.rule.server
    PolicyRuleServer,
    // m.space.parent
    SpaceParent,
    // m.space.child
    SpaceChild,
};
Q_ENUM_NS(EventType)
mtx::events::EventType fromRoomEventType(qml_mtx_events::EventType);
qml_mtx_events::EventType
toRoomEventType(mtx::events::EventType e);

enum EventState
{
    //! The plaintext message was received by the server.
    Received,
    //! At least one of the participants has read the message.
    Read,
    //! The client sent the message. Not yet received.
    Sent,
    //! The client sent the message, but it failed.
    Failed,
    //! When the message is loaded from cache or backfill.
    Empty,
};
Q_ENUM_NS(EventState)

enum NotificationLevel
{
    Nothing,
    Notify,
    Highlight,
};
Q_ENUM_NS(NotificationLevel)
}

class StateKeeper
{
public:
    StateKeeper(std::function<void()> &&fn)
      : fn_(std::move(fn))
    {
    }

    ~StateKeeper() { fn_(); }

private:
    std::function<void()> fn_;
};

struct DecryptionResult
{
    //! The decrypted content as a normal plaintext event.
    mtx::events::collections::TimelineEvents event;
    //! Whether or not the decryption was successful.
    bool isDecrypted = false;
};

class TimelineViewManager;

class TimelineModel final : public QAbstractListModel
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Room)
    QML_UNCREATABLE("")

    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(
      QStringList typingUsers READ typingUsers WRITE updateTypingUsers NOTIFY typingUsersChanged)
    Q_PROPERTY(QString scrollTarget READ scrollTarget NOTIFY scrollTargetChanged)
    Q_PROPERTY(QString reply READ reply WRITE setReply NOTIFY replyChanged RESET resetReply)
    Q_PROPERTY(QString edit READ edit WRITE setEdit NOTIFY editChanged RESET resetEdit)
    Q_PROPERTY(QString thread READ thread WRITE setThread NOTIFY threadChanged RESET resetThread)
    Q_PROPERTY(
      bool paginationInProgress READ paginationInProgress NOTIFY paginationInProgressChanged)
    Q_PROPERTY(QString roomId READ roomId CONSTANT)
    Q_PROPERTY(QString roomName READ roomName NOTIFY roomNameChanged)
    Q_PROPERTY(QString plainRoomName READ plainRoomName NOTIFY roomNameChanged)
    Q_PROPERTY(QString roomAvatarUrl READ roomAvatarUrl NOTIFY roomAvatarUrlChanged)
    Q_PROPERTY(QString roomTopic READ roomTopic NOTIFY roomTopicChanged)
    Q_PROPERTY(QStringList pinnedMessages READ pinnedMessages NOTIFY pinnedMessagesChanged)
    Q_PROPERTY(QStringList widgetLinks READ widgetLinks NOTIFY widgetLinksChanged)
    Q_PROPERTY(int roomMemberCount READ roomMemberCount NOTIFY roomMemberCountChanged)
    Q_PROPERTY(bool isEncrypted READ isEncrypted NOTIFY encryptionChanged)
    Q_PROPERTY(QString fullyReadEventId READ fullyReadEventId NOTIFY fullyReadEventIdChanged)
    Q_PROPERTY(bool isSpace READ isSpace CONSTANT)
    Q_PROPERTY(int trustlevel READ trustlevel NOTIFY trustlevelChanged)
    Q_PROPERTY(bool isDirect READ isDirect NOTIFY isDirectChanged)
    Q_PROPERTY(
      QString directChatOtherUserId READ directChatOtherUserId NOTIFY directChatOtherUserIdChanged)
    Q_PROPERTY(InputBar *input READ input CONSTANT)
    Q_PROPERTY(Permissions *permissions READ permissions NOTIFY permissionsChanged)
    Q_PROPERTY(RoomSummary *parentSpace READ parentSpace NOTIFY parentSpaceChanged)

public:
    explicit TimelineModel(TimelineViewManager *manager,
                           QString room_id,
                           QObject *parent = nullptr);

    enum Roles
    {
        Type,
        TypeString,
        IsOnlyEmoji,
        Body,
        FormattedBody,
        FormattedStateEvent,
        IsSender,
        UserId,
        UserName,
        UserPowerlevel,
        Day,
        Timestamp,
        Url,
        ThumbnailUrl,
        Duration,
        Blurhash,
        Filename,
        Filesize,
        MimeType,
        OriginalHeight,
        OriginalWidth,
        ProportionalHeight,
        EventId,
        State,
        IsEdited,
        IsEditable,
        IsEncrypted,
        IsStateEvent,
        Trustlevel,
        Notificationlevel,
        EncryptionError,
        ReplyTo,
        ThreadId,
        Reactions,
        Room,
        RoomId,
        RoomName,
        RoomTopic,
        CallType,
        Dump,
        RelatedEventCacheBuster,
    };
    Q_ENUM(Roles);

    enum SpecialEffect
    {
        Confetti,
        Rainfall,
    };
    Q_DECLARE_FLAGS(SpecialEffects, SpecialEffect)

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    void multiData(const QModelIndex &index, QModelRoleDataSpan roleDataSpan) const override;
    void
    multiData(const QString &id, const QString &relatedTo, QModelRoleDataSpan roleDataSpan) const;
    QVariant data(const mtx::events::collections::TimelineEvents &event, int role) const;
    Q_INVOKABLE QVariant dataById(const QString &id, int role, const QString &relatedTo);
    Q_INVOKABLE QVariant dataByIndex(int i, int role = Qt::DisplayRole) const
    {
        return data(index(i), role);
    }

    bool canFetchMore(const QModelIndex &) const override;
    void fetchMore(const QModelIndex &) override;

    static QString getBareRoomLink(const QString &);
    static QString getRoomVias(const QString &);

    Q_INVOKABLE QString displayName(const QString &id) const;
    Q_INVOKABLE QString avatarUrl(const QString &id) const;
    Q_INVOKABLE QString formatDateSeparator(QDateTime date) const;
    Q_INVOKABLE QString formatLaterSeparator(QDateTime prevDate, QDateTime date) const;
    Q_INVOKABLE QString formatTypingUsers(const QStringList &users, const QColor &bg);
    Q_INVOKABLE bool showAcceptKnockButton(const QString &id);
    Q_INVOKABLE void acceptKnock(const QString &id);
    Q_INVOKABLE void joinReplacementRoom(const QString &id);
    Q_INVOKABLE QString
    formatMemberEvent(const mtx::events::StateEvent<mtx::events::state::Member> &event) const;
    QString
    formatJoinRuleEvent(const mtx::events::StateEvent<mtx::events::state::JoinRules> &event) const;
    QString formatHistoryVisibilityEvent(
      const mtx::events::StateEvent<mtx::events::state::HistoryVisibility> &event) const;
    QString
    formatGuestAccessEvent(const mtx::events::StateEvent<mtx::events::state::GuestAccess> &) const;
    QString formatPowerLevelEvent(
      const mtx::events::StateEvent<mtx::events::state::PowerLevels> &event) const;
    QString formatImagePackEvent(
      const mtx::events::StateEvent<mtx::events::msc2545::ImagePack> &event) const;
    Q_INVOKABLE QString formatPolicyRule(const QString &id) const;
    Q_INVOKABLE QVariantMap formatRedactedEvent(const QString &id);
    Q_INVOKABLE bool showImage() const;

    Q_INVOKABLE void viewRawMessage(const QString &id);
    Q_INVOKABLE void forwardMessage(const QString &eventId, QString roomId);
    Q_INVOKABLE void viewDecryptedRawMessage(const QString &id);
    Q_INVOKABLE void openUserProfile(QString userid);
    Q_INVOKABLE void unpin(const QString &id);
    Q_INVOKABLE void pin(const QString &id);
    Q_INVOKABLE void showReadReceipts(const QString &id);
    Q_INVOKABLE void redactEvent(const QString &id, const QString &reason = "");
    Q_INVOKABLE void redactAllFromUser(const QString &userid, const QString &reason = "");
    Q_INVOKABLE void
    reportEvent(const QString &eventId, const QString &reason = {}, const int score = -50);
    Q_INVOKABLE int idToIndex(const QString &id) const;
    Q_INVOKABLE QString indexToId(int index) const;
    Q_INVOKABLE void openMedia(const QString &eventId);
    Q_INVOKABLE void cacheMedia(const QString &eventId);
    Q_INVOKABLE bool saveMedia(const QString &eventId) const;
    Q_INVOKABLE bool copyMedia(const QString &eventId) const;
    Q_INVOKABLE void showEvent(QString eventId);
    Q_INVOKABLE void copyLinkToEvent(const QString &eventId) const;

    void
    cacheMedia(const QString &eventId, const std::function<void(const QString filename)> &callback);
    Q_INVOKABLE void sendReset()
    {
        beginResetModel();
        endResetModel();
    }

    Q_INVOKABLE void requestKeyForEvent(const QString &id);

    std::vector<::Reaction> reactions(const std::string &event_id)
    {
        auto list = events.reactions(event_id);
        std::vector<::Reaction> vec;
        vec.reserve(list.size());
        for (const auto &r : std::as_const(list))
            vec.push_back(r.value<Reaction>());
        return vec;
    }

    void updateLastMessage();
    void sync(const mtx::responses::JoinedRoom &room);
    void addEvents(const mtx::responses::Timeline &events);
    void syncState(const mtx::responses::State &state);
    template<class T>
    void sendMessageEvent(const T &content, mtx::events::EventType eventType);
    RelatedInfo relatedInfo(const QString &id);

    DescInfo lastMessage() const;
    uint64_t lastMessageTimestamp() const { return lastMessage_.timestamp; }

    bool isSpace() const { return isSpace_; }
    bool isEncrypted() const { return isEncrypted_; }
    QString fullyReadEventId() const { return QString::fromStdString(fullyReadEventId_); }
    crypto::Trust trustlevel() const;
    int roomMemberCount() const;
    bool isDirect() const { return roomMemberCount() <= 2; }
    QString directChatOtherUserId() const;

    mtx::pushrules::PushRuleEvaluator::RoomContext pushrulesRoomContext() const;

    std::optional<mtx::events::collections::TimelineEvents> eventById(const QString &id)
    {
        auto e = events.get(id.toStdString(), "");
        if (e)
            return *e;
        else
            return std::nullopt;
    }

    void refetchOnlineKeyBackupKeys() { events.refetchOnlineKeyBackupKeys(); };

public slots:
    void setCurrentIndex(int index);
    void setCurrentIndex(int index, bool ignoreInactiveState);
    int currentIndex() const { return idToIndex(currentId); }
    void eventShown();
    void markEventsAsRead(const std::vector<QString> &event_ids);
    void markRoomAsRead();
    void updateLastReadId(const QString &currentRoomId);
    void lastReadIdOnWindowFocus();
    void checkAfterFetch();
    QVariantMap getDump(const QString &eventId, const QString &relatedTo) const;
    void updateTypingUsers(const QStringList &users)
    {
        if (this->typingUsers_ != users) {
            this->typingUsers_ = users;
            emit typingUsersChanged(typingUsers_);
        }
    }
    QStringList typingUsers() const { return typingUsers_; }
    bool paginationInProgress() const { return m_paginationInProgress; }
    QString reply() const { return reply_; }
    void setReply(const QString &newReply);
    void resetReply()
    {
        if (!reply_.isEmpty()) {
            reply_ = QLatin1String("");
            emit replyChanged(reply_);
        }
    }
    QString edit() const { return edit_; }
    void setEdit(const QString &newEdit);
    void resetEdit();
    QString thread() const { return thread_; }
    void setThread(const QString &newThread);
    void resetThread();
    void setDecryptDescription(bool decrypt) { decryptDescription = decrypt; }
    void clearTimeline()
    {
        events.clearTimeline();
        setPaginationInProgress(false);
    }
    void resetState();
    void receivedSessionKey(const std::string &session_key)
    {
        events.receivedSessionKey(session_key);
    }

    QString roomName() const;
    QString plainRoomName() const;
    QString roomTopic() const;
    QStringList pinnedMessages() const;
    QStringList widgetLinks() const;
    InputBar *input() { return &input_; }
    Permissions *permissions() { return &permissions_; }
    QString roomAvatarUrl() const;
    QString roomId() const { return room_id_; }
    RoomSummary *parentSpace();

    bool hasMentions() const { return highlight_count > 0; }
    int notificationCount() const { return static_cast<int>(notification_count); }

    QString scrollTarget() const;

    void triggerSpecialEffects();
    void markSpecialEffectsDone();

private slots:
    void addPendingMessage(mtx::events::collections::TimelineEvents event);
    void scrollTimerEvent();

signals:
    void dataAtIdChanged(QString id);
    void currentIndexChanged(int index);
    void redactionFailed(QString id);
    void mediaCached(QString mxcUrl, QString cacheUrl);
    void newEncryptedImage(mtx::crypto::EncryptedFile encryptionInfo);
    void typingUsersChanged(QStringList users);
    void replyChanged(QString reply);
    void editChanged(QString reply);
    void threadChanged(QString id);
    void openReadReceiptsDialog(ReadReceiptsProxy *rr);
    void showRawMessageDialog(QString rawMessage);
    void paginationInProgressChanged(const bool);
    void newCallEvent(const mtx::events::collections::TimelineEvents &event);
    void scrollToIndex(int index);
    void confetti();
    void confettiDone();
    void rainfall();
    void rainfallDone();

    void lastMessageChanged();
    void notificationsChanged();

    void newState(const mtx::responses::StateEvents &events);

    void newMessageToSend(mtx::events::collections::TimelineEvents event);
    void addPendingMessageToStore(mtx::events::collections::TimelineEvents event);
    void updateFlowEventId(std::string event_id);

    void parentSpaceChanged();
    void encryptionChanged();
    void fullyReadEventIdChanged();
    void trustlevelChanged();
    void roomNameChanged();
    void roomTopicChanged();
    void pinnedMessagesChanged();
    void widgetLinksChanged();
    void roomAvatarUrlChanged();
    void roomMemberCountChanged();
    void isDirectChanged();
    void directChatOtherUserIdChanged();
    void permissionsChanged();
    void forwardToRoom(mtx::events::collections::TimelineEvents const *e, QString roomId);

    void scrollTargetChanged();

    void fetchedMore();

private:
    template<typename T>
    void
    sendEncryptedMessage(const mtx::events::RoomEvent<T> &msg, mtx::events::EventType eventType);
    void readEvent(const std::string &id);

    void setPaginationInProgress(const bool paginationInProgress);

    QString room_id_;

    QSet<QString> read;

    mutable EventStore events;

    QString currentId, currentReadId;
    QString reply_, edit_, thread_;
    QString replyBeforeEdit;
    QStringList typingUsers_;

    TimelineViewManager *manager_;

    InputBar input_{this};
    Permissions permissions_;

    QTimer showEventTimer{this};
    QString eventIdToShow;
    int showEventTimerCounter = 0;

    DescInfo lastMessage_{};

    friend struct SendMessageVisitor;

    uint64_t notification_count = 0, highlight_count = 0;

    unsigned int relatedEventCacheBuster = 0;

    bool decryptDescription     = true;
    bool m_paginationInProgress = false;
    bool isSpace_               = false;
    bool isEncrypted_           = false;
    std::string last_event_id;
    std::string fullyReadEventId_;

    bool needsSpecialEffects_ = false;
    QFlags<SpecialEffect> specialEffects_;

    std::unique_ptr<RoomSummary, DeleteLaterDeleter> parentSummary = nullptr;
    bool parentChecked                                             = false;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(TimelineModel::SpecialEffects)

template<class T>
void
TimelineModel::sendMessageEvent(const T &content, mtx::events::EventType eventType)
{
    if constexpr (std::is_same_v<T, mtx::events::msg::StickerImage>) {
        mtx::events::Sticker msgCopy = {};
        msgCopy.content              = content;
        msgCopy.type                 = eventType;
        emit newMessageToSend(msgCopy);
    } else {
        mtx::events::RoomEvent<T> msgCopy = {};
        msgCopy.content                   = content;
        msgCopy.type                      = eventType;
        emit newMessageToSend(msgCopy);
    }

    resetReply();
    resetEdit();
}
