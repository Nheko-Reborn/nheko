// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include <QColor>
#include <QDate>
#include <QSet>
#include <QTimer>
#include <QVariant>

#include <mtxclient/http/errors.hpp>

#include "CacheCryptoStructs.h"
#include "CacheStructs.h"
#include "EventStore.h"
#include "InputBar.h"
#include "InviteesModel.h"
#include "MemberList.h"
#include "Permissions.h"
#include "ReadReceiptsModel.h"

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

namespace qml_mtx_events {
Q_NAMESPACE

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
    EmoteMessage,
    FileMessage,
    ImageMessage,
    LocationMessage,
    NoticeMessage,
    TextMessage,
    VideoMessage,
    Redacted,
    UnknownMessage,
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
    //! When the message is loaded from cache or backfill.
    Empty,
};
Q_ENUM_NS(EventState)
}

class StateKeeper
{
public:
    StateKeeper(std::function<void()> &&fn)
      : fn_(std::move(fn))
    {}

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

class TimelineModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(std::vector<QString> typingUsers READ typingUsers WRITE updateTypingUsers NOTIFY
                 typingUsersChanged)
    Q_PROPERTY(QString scrollTarget READ scrollTarget NOTIFY scrollTargetChanged)
    Q_PROPERTY(QString reply READ reply WRITE setReply NOTIFY replyChanged RESET resetReply)
    Q_PROPERTY(QString edit READ edit WRITE setEdit NOTIFY editChanged RESET resetEdit)
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
    Q_PROPERTY(bool isSpace READ isSpace CONSTANT)
    Q_PROPERTY(int trustlevel READ trustlevel NOTIFY trustlevelChanged)
    Q_PROPERTY(bool isDirect READ isDirect NOTIFY isDirectChanged)
    Q_PROPERTY(
      QString directChatOtherUserId READ directChatOtherUserId NOTIFY directChatOtherUserIdChanged)
    Q_PROPERTY(InputBar *input READ input CONSTANT)
    Q_PROPERTY(Permissions *permissions READ permissions NOTIFY permissionsChanged)

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
        PreviousMessageUserId,
        IsSender,
        UserId,
        UserName,
        PreviousMessageDay,
        PreviousMessageIsStateEvent,
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
        EncryptionError,
        ReplyTo,
        Reactions,
        RoomId,
        RoomName,
        RoomTopic,
        CallType,
        Dump,
        RelatedEventCacheBuster,
    };
    Q_ENUM(Roles);

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant data(const mtx::events::collections::TimelineEvents &event, int role) const;
    Q_INVOKABLE QVariant dataById(const QString &id, int role, const QString &relatedTo);

    bool canFetchMore(const QModelIndex &) const override;
    void fetchMore(const QModelIndex &) override;

    static QString getBareRoomLink(const QString &);
    static QString getRoomVias(const QString &);

    Q_INVOKABLE QString displayName(const QString &id) const;
    Q_INVOKABLE QString avatarUrl(const QString &id) const;
    Q_INVOKABLE QString formatDateSeparator(QDate date) const;
    Q_INVOKABLE QString formatTypingUsers(const std::vector<QString> &users, const QColor &bg);
    Q_INVOKABLE bool showAcceptKnockButton(const QString &id);
    Q_INVOKABLE void acceptKnock(const QString &id);
    Q_INVOKABLE QString formatMemberEvent(const QString &id);
    Q_INVOKABLE QString formatJoinRuleEvent(const QString &id);
    Q_INVOKABLE QString formatHistoryVisibilityEvent(const QString &id);
    Q_INVOKABLE QString formatGuestAccessEvent(const QString &id);
    Q_INVOKABLE QString formatPowerLevelEvent(const QString &id);
    Q_INVOKABLE QString formatImagePackEvent(const QString &id);
    Q_INVOKABLE QString formatPolicyRule(const QString &id);
    Q_INVOKABLE QVariantMap formatRedactedEvent(const QString &id);

    Q_INVOKABLE void viewRawMessage(const QString &id);
    Q_INVOKABLE void forwardMessage(const QString &eventId, QString roomId);
    Q_INVOKABLE void viewDecryptedRawMessage(const QString &id);
    Q_INVOKABLE void openUserProfile(QString userid);
    Q_INVOKABLE void editAction(QString id);
    Q_INVOKABLE void replyAction(const QString &id);
    Q_INVOKABLE void unpin(const QString &id);
    Q_INVOKABLE void pin(const QString &id);
    Q_INVOKABLE void showReadReceipts(QString id);
    Q_INVOKABLE void redactEvent(const QString &id, const QString &reason = "");
    Q_INVOKABLE void redactAllFromUser(const QString &userid, const QString &reason = "");
    Q_INVOKABLE int idToIndex(const QString &id) const;
    Q_INVOKABLE QString indexToId(int index) const;
    Q_INVOKABLE void openMedia(const QString &eventId);
    Q_INVOKABLE void cacheMedia(const QString &eventId);
    Q_INVOKABLE bool saveMedia(const QString &eventId) const;
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
        for (const auto &r : list)
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
    crypto::Trust trustlevel() const;
    int roomMemberCount() const;
    bool isDirect() const { return roomMemberCount() <= 2; }
    QString directChatOtherUserId() const;

    std::optional<mtx::events::collections::TimelineEvents> eventById(const QString &id)
    {
        auto e = events.get(id.toStdString(), "");
        if (e)
            return *e;
        else
            return std::nullopt;
    }

public slots:
    void setCurrentIndex(int index);
    int currentIndex() const { return idToIndex(currentId); }
    void eventShown();
    void markEventsAsRead(const std::vector<QString> &event_ids);
    QVariantMap getDump(const QString &eventId, const QString &relatedTo) const;
    void updateTypingUsers(const std::vector<QString> &users)
    {
        if (this->typingUsers_ != users) {
            this->typingUsers_ = users;
            emit typingUsersChanged(typingUsers_);
        }
    }
    std::vector<QString> typingUsers() const { return typingUsers_; }
    bool paginationInProgress() const { return m_paginationInProgress; }
    QString reply() const { return reply_; }
    void setReply(const QString &newReply)
    {
        if (edit_.startsWith('m'))
            return;

        if (reply_ != newReply) {
            reply_ = newReply;
            emit replyChanged(reply_);
        }
    }
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
    void setDecryptDescription(bool decrypt) { decryptDescription = decrypt; }
    void clearTimeline() { events.clearTimeline(); }
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

    bool hasMentions() const { return highlight_count > 0; }
    int notificationCount() const { return notification_count; }

    QString scrollTarget() const;

private slots:
    void addPendingMessage(mtx::events::collections::TimelineEvents event);
    void scrollTimerEvent();

signals:
    void dataAtIdChanged(QString id);
    void currentIndexChanged(int index);
    void redactionFailed(QString id);
    void mediaCached(QString mxcUrl, QString cacheUrl);
    void newEncryptedImage(mtx::crypto::EncryptedFile encryptionInfo);
    void typingUsersChanged(std::vector<QString> users);
    void replyChanged(QString reply);
    void editChanged(QString reply);
    void openReadReceiptsDialog(ReadReceiptsProxy *rr);
    void showRawMessageDialog(QString rawMessage);
    void paginationInProgressChanged(const bool);
    void newCallEvent(const mtx::events::collections::TimelineEvents &event);
    void scrollToIndex(int index);

    void lastMessageChanged();
    void notificationsChanged();

    void newState(const mtx::responses::StateEvents &events);

    void newMessageToSend(mtx::events::collections::TimelineEvents event);
    void addPendingMessageToStore(mtx::events::collections::TimelineEvents event);
    void updateFlowEventId(std::string event_id);

    void encryptionChanged();
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
    void forwardToRoom(mtx::events::collections::TimelineEvents *e, QString roomId);

    void scrollTargetChanged();

private:
    template<typename T>
    void sendEncryptedMessage(mtx::events::RoomEvent<T> msg, mtx::events::EventType eventType);
    void readEvent(const std::string &id);

    void setPaginationInProgress(const bool paginationInProgress);

    QString room_id_;

    QSet<QString> read;

    mutable EventStore events;

    QString currentId, currentReadId;
    QString reply_, edit_;
    QString textBeforeEdit, replyBeforeEdit;
    std::vector<QString> typingUsers_;

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
};

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
