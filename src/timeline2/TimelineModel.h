#pragma once

#include <QAbstractListModel>
#include <QColor>
#include <QDate>
#include <QHash>

#include <mtx/responses.hpp>

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
        UnknownMessage,
};
Q_ENUM_NS(EventType)
}

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

        void addEvents(const mtx::responses::Timeline &events);

public slots:
        void fetchHistory();

private slots:
        // Add old events at the top of the timeline.

        void addBackwardsEvents(const mtx::responses::Messages &msgs);

signals:
        void oldMessagesRetrieved(const mtx::responses::Messages &res);

private:
        QHash<QString, mtx::events::collections::TimelineEvents> events;
        std::vector<QString> eventOrder;

        QString room_id_;
        QString prev_batch_token_;

        bool isInitialSync        = true;
        bool paginationInProgress = false;

        QHash<QString, QColor> userColors;
};
