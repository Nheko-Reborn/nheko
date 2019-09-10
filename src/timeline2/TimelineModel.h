#pragma once

#include <mtx/responses.hpp>

#include <QAbstractListModel>
#include <QColor>
#include <QDate>
#include <QHash>

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
}

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
                Height,
                Width,
                ProportionalHeight,
                Id,
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

        void addEvents(const mtx::responses::Timeline &events);
        template<class T>
        void sendMessage(const T &msg);

public slots:
        void fetchHistory();

private slots:
        // Add old events at the top of the timeline.

        void addBackwardsEvents(const mtx::responses::Messages &msgs);

signals:
        void oldMessagesRetrieved(const mtx::responses::Messages &res);
        void messageFailed(const std::string txn_id);
        void messageSent(const std::string txn_id, std::string event_id);

private:
        DecryptionResult decryptEvent(
          const mtx::events::EncryptedEvent<mtx::events::msg::Encrypted> &e) const;
        std::vector<QString> internalAddEvents(
          const std::vector<mtx::events::collections::TimelineEvents> &timeline);

        QHash<QString, mtx::events::collections::TimelineEvents> events;
        std::vector<QString> eventOrder;

        QString room_id_;
        QString prev_batch_token_;

        bool isInitialSync        = true;
        bool paginationInProgress = false;

        QHash<QString, QColor> userColors;
};

template<class T>
void
TimelineModel::sendMessage(const T &msg)
{
        auto txn_id = http::client()->generate_txn_id();
        http::client()->send_room_message<T, mtx::events::EventType::RoomMessage>(
          room_id_.toStdString(),
          txn_id,
          msg,
          [this, txn_id](const mtx::responses::EventId &res, mtx::http::RequestErr err) {
                  if (err) {
                          const int status_code = static_cast<int>(err->status_code);
                          nhlog::net()->warn("[{}] failed to send message: {} {}",
                                             txn_id,
                                             err->matrix_error.error,
                                             status_code);
                          emit messageFailed(txn_id);
                  }
                  emit messageSent(txn_id, res.event_id.to_string());
          });
}
