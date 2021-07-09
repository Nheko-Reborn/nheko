// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include <QColor>
#include <QDate>
#include <QHash>
#include <QSet>
#include <QTimer>

#include <mtxclient/http/errors.hpp>

#include "CacheCryptoStructs.h"
#include "CacheStructs.h"
#include "EventStore.h"
#include "InputBar.h"
#include "Permissions.h"
#include "ui/RoomSettings.h"
#include "ui/UserProfile.h"

namespace mtx::http {
using RequestErr = const std::optional<mtx::http::ClientError> &;
}
namespace mtx::responses {
struct Timeline;
struct Messages;
struct ClaimKeys;
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
        KeyVerificationReady
};
Q_ENUM_NS(EventType)
mtx::events::EventType fromRoomEventType(qml_mtx_events::EventType);

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
        Q_PROPERTY(
          int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
        Q_PROPERTY(std::vector<QString> typingUsers READ typingUsers WRITE updateTypingUsers NOTIFY
                     typingUsersChanged)
        Q_PROPERTY(QString scrollTarget READ scrollTarget NOTIFY scrollTargetChanged)
        Q_PROPERTY(QString reply READ reply WRITE setReply NOTIFY replyChanged RESET resetReply)
        Q_PROPERTY(QString edit READ edit WRITE setEdit NOTIFY editChanged RESET resetEdit)
        Q_PROPERTY(
          bool paginationInProgress READ paginationInProgress NOTIFY paginationInProgressChanged)
        Q_PROPERTY(QString roomName READ roomName NOTIFY roomNameChanged)
        Q_PROPERTY(QString roomAvatarUrl READ roomAvatarUrl NOTIFY roomAvatarUrlChanged)
        Q_PROPERTY(QString roomTopic READ roomTopic NOTIFY roomTopicChanged)
        Q_PROPERTY(int roomMemberCount READ roomMemberCount NOTIFY roomMemberCountChanged)
        Q_PROPERTY(bool isSpace READ isSpace CONSTANT)
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
                Day,
                Timestamp,
                Url,
                ThumbnailUrl,
                Blurhash,
                Filename,
                Filesize,
                MimeType,
                Height,
                Width,
                ProportionalHeight,
                Id,
                State,
                IsEdited,
                IsEditable,
                IsEncrypted,
                Trustlevel,
                IsRoomEncrypted,
                ReplyTo,
                Reactions,
                RoomId,
                RoomName,
                RoomTopic,
                CallType,
                Dump,
        };

        QHash<int, QByteArray> roleNames() const override;
        int rowCount(const QModelIndex &parent = QModelIndex()) const override;
        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
        QVariant data(const mtx::events::collections::TimelineEvents &event, int role) const;

        bool canFetchMore(const QModelIndex &) const override;
        void fetchMore(const QModelIndex &) override;

        Q_INVOKABLE QString displayName(QString id) const;
        Q_INVOKABLE QString avatarUrl(QString id) const;
        Q_INVOKABLE QString formatDateSeparator(QDate date) const;
        Q_INVOKABLE QString formatTypingUsers(const std::vector<QString> &users, QColor bg);
        Q_INVOKABLE QString formatMemberEvent(QString id);
        Q_INVOKABLE QString formatJoinRuleEvent(QString id);
        Q_INVOKABLE QString formatHistoryVisibilityEvent(QString id);
        Q_INVOKABLE QString formatGuestAccessEvent(QString id);
        Q_INVOKABLE QString formatPowerLevelEvent(QString id);

        Q_INVOKABLE void viewRawMessage(QString id) const;
        Q_INVOKABLE void forwardMessage(QString eventId, QString roomId);
        Q_INVOKABLE void viewDecryptedRawMessage(QString id) const;
        Q_INVOKABLE void openUserProfile(QString userid);
        Q_INVOKABLE void openRoomSettings();
        Q_INVOKABLE void editAction(QString id);
        Q_INVOKABLE void replyAction(QString id);
        Q_INVOKABLE void readReceiptsAction(QString id) const;
        Q_INVOKABLE void redactEvent(QString id);
        Q_INVOKABLE int idToIndex(QString id) const;
        Q_INVOKABLE QString indexToId(int index) const;
        Q_INVOKABLE void openMedia(QString eventId);
        Q_INVOKABLE void cacheMedia(QString eventId);
        Q_INVOKABLE bool saveMedia(QString eventId) const;
        Q_INVOKABLE void showEvent(QString eventId);
        Q_INVOKABLE void copyLinkToEvent(QString eventId) const;
        void cacheMedia(QString eventId, std::function<void(const QString filename)> callback);

        std::vector<::Reaction> reactions(const std::string &event_id)
        {
                auto list = events.reactions(event_id);
                std::vector<::Reaction> vec;
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
        RelatedInfo relatedInfo(QString id);

        DescInfo lastMessage() const { return lastMessage_; }
        bool isSpace() const { return isSpace_; }
        int roomMemberCount() const;

public slots:
        void setCurrentIndex(int index);
        int currentIndex() const { return idToIndex(currentId); }
        void eventShown();
        void markEventsAsRead(const std::vector<QString> &event_ids);
        QVariantMap getDump(QString eventId, QString relatedTo) const;
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
        void setReply(QString newReply)
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
                        reply_ = "";
                        emit replyChanged(reply_);
                }
        }
        QString edit() const { return edit_; }
        void setEdit(QString newEdit);
        void resetEdit();
        void setDecryptDescription(bool decrypt) { decryptDescription = decrypt; }
        void clearTimeline() { events.clearTimeline(); }
        void receivedSessionKey(const std::string &session_key)
        {
                events.receivedSessionKey(session_key);
        }

        QString roomName() const;
        QString plainRoomName() const;
        QString roomTopic() const;
        InputBar *input() { return &input_; }
        Permissions *permissions() { return &permissions_; }
        QString roomAvatarUrl() const;
        QString roomId() const { return room_id_; }

        bool hasMentions() { return highlight_count > 0; }
        int notificationCount() { return notification_count; }

        QString scrollTarget() const;

private slots:
        void addPendingMessage(mtx::events::collections::TimelineEvents event);
        void scrollTimerEvent();

signals:
        void currentIndexChanged(int index);
        void redactionFailed(QString id);
        void eventRedacted(QString id);
        void mediaCached(QString mxcUrl, QString cacheUrl);
        void newEncryptedImage(mtx::crypto::EncryptedFile encryptionInfo);
        void typingUsersChanged(std::vector<QString> users);
        void replyChanged(QString reply);
        void editChanged(QString reply);
        void paginationInProgressChanged(const bool);
        void newCallEvent(const mtx::events::collections::TimelineEvents &event);
        void scrollToIndex(int index);

        void lastMessageChanged();
        void notificationsChanged();

        void openRoomSettingsDialog(RoomSettings *settings);

        void newMessageToSend(mtx::events::collections::TimelineEvents event);
        void addPendingMessageToStore(mtx::events::collections::TimelineEvents event);
        void updateFlowEventId(std::string event_id);

        void roomNameChanged();
        void roomTopicChanged();
        void roomAvatarUrlChanged();
        void roomMemberCountChanged();
        void permissionsChanged();
        void forwardToRoom(mtx::events::collections::TimelineEvents *e, QString roomId);

        void scrollTargetChanged();

private:
        template<typename T>
        void sendEncryptedMessage(mtx::events::RoomEvent<T> msg, mtx::events::EventType eventType);
        void readEvent(const std::string &id);

        void setPaginationInProgress(const bool paginationInProgress);

        QSet<QString> read;

        mutable EventStore events;

        QString room_id_;

        QString currentId, currentReadId;
        QString reply_, edit_;
        QString textBeforeEdit, replyBeforeEdit;
        std::vector<QString> typingUsers_;

        TimelineViewManager *manager_;

        InputBar input_{this};
        Permissions permissions_{this};

        QTimer showEventTimer{this};
        QString eventIdToShow;
        int showEventTimerCounter = 0;

        DescInfo lastMessage_{};

        friend struct SendMessageVisitor;

        int notification_count = 0, highlight_count = 0;

        bool decryptDescription     = true;
        bool m_paginationInProgress = false;
        bool isSpace_               = false;
};

template<class T>
void
TimelineModel::sendMessageEvent(const T &content, mtx::events::EventType eventType)
{
        mtx::events::RoomEvent<T> msgCopy = {};
        msgCopy.content                   = content;
        msgCopy.type                      = eventType;
        emit newMessageToSend(msgCopy);
        resetReply();
        resetEdit();
}
