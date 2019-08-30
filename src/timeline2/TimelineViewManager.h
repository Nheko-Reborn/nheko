#pragma once

#include <QQuickView>
#include <QSharedPointer>
#include <QWidget>

#include <mtx/responses.hpp>

#include "Cache.h"
#include "TimelineModel.h"
#include "Utils.h"

// temporary for stubs
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

class TimelineViewManager : public QObject
{
        Q_OBJECT
public:
        TimelineViewManager(QWidget *parent = 0);
        QWidget *getWidget() const { return container; }

        void initialize(const mtx::responses::Rooms &rooms);
        void addRoom(const QString &room_id);

        void sync(const mtx::responses::Rooms &rooms) {}
        void clearAll() { models.clear(); }

signals:
        void clearRoomMessageCount(QString roomid);
        void updateRoomsLastMessage(const QString &user, const DescInfo &info);

public slots:
        void updateReadReceipts(const QString &room_id, const std::vector<QString> &event_ids) {}
        void removeTimelineEvent(const QString &room_id, const QString &event_id) {}
        void initWithMessages(const std::map<QString, mtx::responses::Timeline> &msgs);

        void setHistoryView(const QString &room_id);

        void queueTextMessage(const QString &msg) {}
        void queueReplyMessage(const QString &reply, const RelatedInfo &related) {}
        void queueEmoteMessage(const QString &msg) {}
        void queueImageMessage(const QString &roomid,
                               const QString &filename,
                               const QString &url,
                               const QString &mime,
                               uint64_t dsize,
                               const QSize &dimensions)
        {}
        void queueFileMessage(const QString &roomid,
                              const QString &filename,
                              const QString &url,
                              const QString &mime,
                              uint64_t dsize)
        {}
        void queueAudioMessage(const QString &roomid,
                               const QString &filename,
                               const QString &url,
                               const QString &mime,
                               uint64_t dsize)
        {}
        void queueVideoMessage(const QString &roomid,
                               const QString &filename,
                               const QString &url,
                               const QString &mime,
                               uint64_t dsize)
        {}

private:
        QQuickView *view;
        QWidget *container;

        QHash<QString, QSharedPointer<TimelineModel>> models;
};

#pragma GCC diagnostic pop
