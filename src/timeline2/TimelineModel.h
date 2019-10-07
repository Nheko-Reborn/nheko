#pragma once

#include <mtx/responses.hpp>

#include <QAbstractListModel>
#include <QColor>
#include <QDate>
#include <QHash>
#include <QSet>

#include "Cache.h"
#include "Logging.h"
#include "MatrixClient.h"

namespace qml_mtx_events {
Q_NAMESPACE

enum EventType
{
        // Unsupported event
        Unsupported,
        /// m.room_key_request
        KeyRequest,
        /// m.room.aliases
        Aliases,
        /// m.room.avatar
        Avatar,
        /// m.room.canonical_alias
        CanonicalAlias,
        /// m.room.create
        Create,
        /// m.room.encrypted.
        Encrypted,
        /// m.room.encryption.
        Encryption,
        /// m.room.guest_access
        GuestAccess,
        /// m.room.history_visibility
        HistoryVisibility,
        /// m.room.join_rules
        JoinRules,
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
        //! When the message failed to send
        Failed,
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

class TimelineModel : public QAbstractListModel
{
        Q_OBJECT
        Q_PROPERTY(
          int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)

public:
        explicit TimelineModel(QString room_id, QObject *parent = 0);

        enum Roles
        {
                Section,
                Type,
                Body,
                FormattedBody,
                UserId,
                UserName,
                Timestamp,
                Url,
                Filename,
                MimeType,
                Height,
                Width,
                ProportionalHeight,
                Id,
                State,
                IsEncrypted,
        };

        QHash<int, QByteArray> roleNames() const override;
        int rowCount(const QModelIndex &parent = QModelIndex()) const override;
        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

        Q_INVOKABLE QColor userColor(QString id, QColor background);
        Q_INVOKABLE QString displayName(QString id) const;
        Q_INVOKABLE QString avatarUrl(QString id) const;
        Q_INVOKABLE QString formatDateSeparator(QDate date) const;
        Q_INVOKABLE QString escapeEmoji(QString str) const;
        Q_INVOKABLE void viewRawMessage(QString id) const;
        Q_INVOKABLE void replyAction(QString id);
        Q_INVOKABLE void readReceiptsAction(QString id) const;
        Q_INVOKABLE void redactEvent(QString id);
        Q_INVOKABLE int idToIndex(QString id) const;
        Q_INVOKABLE QString indexToId(int index) const;

        void addEvents(const mtx::responses::Timeline &events);
        template<class T>
        void sendMessage(const T &msg);

public slots:
        void fetchHistory();
        void setCurrentIndex(int index)
        {
                currentId = indexToId(index);
                emit currentIndexChanged(index);
        }
        int currentIndex() const { return idToIndex(currentId); }
        void markEventsAsRead(const std::vector<QString> &event_ids);

private slots:
        // Add old events at the top of the timeline.
        void addBackwardsEvents(const mtx::responses::Messages &msgs);

signals:
        void oldMessagesRetrieved(const mtx::responses::Messages &res);
        void messageFailed(QString txn_id);
        void messageSent(QString txn_id, QString event_id);
        void currentIndexChanged(int index);
        void redactionFailed(QString id);
        void eventRedacted(QString id);

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

        QHash<QString, mtx::events::collections::TimelineEvents> events;
        QSet<QString> pending, failed, read;
        std::vector<QString> eventOrder;

        QString room_id_;
        QString prev_batch_token_;

        bool isInitialSync        = true;
        bool paginationInProgress = false;

        QHash<QString, QColor> userColors;
        QString currentId;
};

template<class T>
void
TimelineModel::sendMessage(const T &msg)
{
        auto txn_id                       = http::client()->generate_txn_id();
        mtx::events::RoomEvent<T> msgCopy = {};
        msgCopy.content                   = msg;
        msgCopy.type                      = mtx::events::EventType::RoomMessage;
        msgCopy.event_id                  = txn_id;
        msgCopy.sender                    = http::client()->user_id().to_string();
        msgCopy.origin_server_ts          = QDateTime::currentMSecsSinceEpoch();
        internalAddEvents({msgCopy});

        QString txn_id_qstr = QString::fromStdString(txn_id);
        beginInsertRows(QModelIndex(),
                        static_cast<int>(this->eventOrder.size()),
                        static_cast<int>(this->eventOrder.size()));
        pending.insert(txn_id_qstr);
        this->eventOrder.insert(this->eventOrder.end(), txn_id_qstr);
        endInsertRows();

        if (cache::client()->isRoomEncrypted(room_id_.toStdString()))
                sendEncryptedMessage(txn_id, nlohmann::json(msg));
        else
                http::client()->send_room_message<T, mtx::events::EventType::RoomMessage>(
                  room_id_.toStdString(),
                  txn_id,
                  msg,
                  [this, txn_id, txn_id_qstr](const mtx::responses::EventId &res,
                                              mtx::http::RequestErr err) {
                          if (err) {
                                  const int status_code = static_cast<int>(err->status_code);
                                  nhlog::net()->warn("[{}] failed to send message: {} {}",
                                                     txn_id,
                                                     err->matrix_error.error,
                                                     status_code);
                                  emit messageFailed(txn_id_qstr);
                          }
                          emit messageSent(txn_id_qstr,
                                           QString::fromStdString(res.event_id.to_string()));
                  });
}
