#pragma once

#include <QAbstractListModel>
#include <QColor>
#include <QDate>
#include <QHash>
#include <QSet>

#include <mtxclient/http/errors.hpp>

#include "CacheCryptoStructs.h"
#include "ReactionsModel.h"
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
};
Q_ENUM_NS(EventType)

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
        Q_PROPERTY(QString reply READ reply WRITE setReply NOTIFY replyChanged RESET resetReply)
        Q_PROPERTY(
          bool paginationInProgress READ paginationInProgress NOTIFY paginationInProgressChanged)

public:
        explicit TimelineModel(TimelineViewManager *manager,
                               QString room_id,
                               QObject *parent = nullptr);

        enum Roles
        {
                Section,
                Type,
                TypeString,
                IsOnlyEmoji,
                Body,
                FormattedBody,
                UserId,
                UserName,
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
                IsEncrypted,
                IsRoomEncrypted,
                ReplyTo,
                Reactions,
                RoomId,
                RoomName,
                RoomTopic,
                Dump,
        };

        QHash<int, QByteArray> roleNames() const override;
        int rowCount(const QModelIndex &parent = QModelIndex()) const override;
        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
        QVariant data(const QString &id, int role) const;

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

        Q_INVOKABLE QString escapeEmoji(QString str) const;
        Q_INVOKABLE void viewRawMessage(QString id) const;
        Q_INVOKABLE void viewDecryptedRawMessage(QString id) const;
        Q_INVOKABLE void openUserProfile(QString userid);
        Q_INVOKABLE void replyAction(QString id);
        Q_INVOKABLE void readReceiptsAction(QString id) const;
        Q_INVOKABLE void redactEvent(QString id);
        Q_INVOKABLE int idToIndex(QString id) const;
        Q_INVOKABLE QString indexToId(int index) const;
        Q_INVOKABLE void cacheMedia(QString eventId);
        Q_INVOKABLE bool saveMedia(QString eventId) const;

        void updateLastMessage();
        void addEvents(const mtx::responses::Timeline &events);
        template<class T>
        void sendMessage(const T &msg);
        RelatedInfo relatedInfo(QString id);

public slots:
        void setCurrentIndex(int index);
        int currentIndex() const { return idToIndex(currentId); }
        void markEventsAsRead(const std::vector<QString> &event_ids);
        QVariantMap getDump(QString eventId) const;
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
        void setDecryptDescription(bool decrypt) { decryptDescription = decrypt; }

private slots:
        // Add old events at the top of the timeline.
        void addBackwardsEvents(const mtx::responses::Messages &msgs);
        void processOnePendingMessage();
        void addPendingMessage(mtx::events::collections::TimelineEvents event);

signals:
        void oldMessagesRetrieved(const mtx::responses::Messages &res);
        void messageFailed(QString txn_id);
        void messageSent(QString txn_id, QString event_id);
        void currentIndexChanged(int index);
        void redactionFailed(QString id);
        void eventRedacted(QString id);
        void nextPendingMessage();
        void newMessageToSend(mtx::events::collections::TimelineEvents event);
        void mediaCached(QString mxcUrl, QString cacheUrl);
        void newEncryptedImage(mtx::crypto::EncryptedFile encryptionInfo);
        void eventFetched(QString requestingEvent, mtx::events::collections::TimelineEvents event);
        void typingUsersChanged(std::vector<QString> users);
        void replyChanged(QString reply);
        void paginationInProgressChanged(const bool);

        void openProfile(UserProfile *profile);

private:
        DecryptionResult decryptEvent(
          const mtx::events::EncryptedEvent<mtx::events::msg::Encrypted> &e) const;
        std::vector<QString> internalAddEvents(
          const std::vector<mtx::events::collections::TimelineEvents> &timeline);
        void sendEncryptedMessage(const std::string &txn_id, nlohmann::json content);
        void handleClaimedKeys(std::shared_ptr<StateKeeper> keeper,
                               const std::map<std::string, std::string> &room_key,
                               const std::map<std::string, DevicePublicKeys> &pks,
                               const std::string &user_id,
                               const mtx::responses::ClaimKeys &res,
                               mtx::http::RequestErr err);
        void readEvent(const std::string &id);

        void setPaginationInProgress(const bool paginationInProgress);

        QHash<QString, mtx::events::collections::TimelineEvents> events;
        QSet<QString> read;
        QList<QString> pending;
        std::vector<QString> eventOrder;
        std::map<QString, ReactionsModel> reactions;

        QString room_id_;
        QString prev_batch_token_;

        bool isInitialSync          = true;
        bool decryptDescription     = true;
        bool m_paginationInProgress = false;

        QString currentId;
        QString reply_;
        std::vector<QString> typingUsers_;

        TimelineViewManager *manager_;

        friend struct SendMessageVisitor;
};

template<class T>
void
TimelineModel::sendMessage(const T &msg)
{
        mtx::events::RoomEvent<T> msgCopy = {};
        msgCopy.content                   = msg;
        emit newMessageToSend(msgCopy);
}
