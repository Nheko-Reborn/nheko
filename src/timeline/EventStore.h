#pragma once

#include <limits>
#include <string>

#include <QCache>
#include <QObject>
#include <qhashfunctions.h>

#include <mtx/events/collections.hpp>
#include <mtx/responses/sync.hpp>

class EventStore : public QObject
{
        Q_OBJECT

public:
        EventStore(std::string room_id, QObject *parent);

        struct Index
        {
                std::string room;
                int64_t idx;

                friend uint qHash(const Index &i, uint seed = 0) noexcept
                {
                        QtPrivate::QHashCombine hash;
                        seed = hash(seed, QByteArray::fromRawData(i.room.data(), i.room.size()));
                        seed = hash(seed, i.idx);
                        return seed;
                }

                friend bool operator==(const Index &a, const Index &b) noexcept
                {
                        return a.idx == b.idx && a.room == b.room;
                }
        };
        struct IdIndex
        {
                std::string room, id;

                friend uint qHash(const IdIndex &i, uint seed = 0) noexcept
                {
                        QtPrivate::QHashCombine hash;
                        seed = hash(seed, QByteArray::fromRawData(i.room.data(), i.room.size()));
                        seed = hash(seed, QByteArray::fromRawData(i.id.data(), i.id.size()));
                        return seed;
                }

                friend bool operator==(const IdIndex &a, const IdIndex &b) noexcept
                {
                        return a.id == b.id && a.room == b.room;
                }
        };

        void fetchMore();
        void handleSync(const mtx::responses::Timeline &events);

        // optionally returns the event or nullptr and fetches it, after which it emits a
        // relatedFetched event
        mtx::events::collections::TimelineEvents *event(std::string_view id,
                                                        std::string_view related_to,
                                                        bool decrypt = true);
        // always returns a proper event as long as the idx is valid
        mtx::events::collections::TimelineEvents *event(int idx, bool decrypt = true);

        int size() const
        {
                return last != std::numeric_limits<int64_t>::max()
                         ? static_cast<int>(last - first) + 1
                         : 0;
        }
        int toExternalIdx(int64_t idx) const { return static_cast<int>(idx - first); }
        int64_t toInternalIdx(int idx) const { return first + idx; }

        std::optional<int> idToIndex(std::string_view id) const;
        std::optional<std::string> indexToId(int idx) const;

signals:
        void beginInsertRows(int from, int to);
        void endInsertRows();
        void dataChanged(int from, int to);
        void newEncryptedImage(mtx::crypto::EncryptedFile encryptionInfo);

private:
        mtx::events::collections::TimelineEvents *decryptEvent(
          const IdIndex &idx,
          const mtx::events::EncryptedEvent<mtx::events::msg::Encrypted> &e);

        std::string room_id_;

        int64_t first = std::numeric_limits<int64_t>::max(),
                last  = std::numeric_limits<int64_t>::max();

        static QCache<IdIndex, mtx::events::collections::TimelineEvents> decryptedEvents_;
        static QCache<Index, mtx::events::collections::TimelineEvents> events_;
        static QCache<IdIndex, mtx::events::collections::TimelineEvents> events_by_id_;
};
