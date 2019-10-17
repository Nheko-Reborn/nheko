#pragma once

#include <QQuickView>
#include <QSharedPointer>
#include <QWidget>

#include <mtx/responses.hpp>

#include "Cache.h"
#include "Logging.h"
#include "TimelineModel.h"
#include "Utils.h"

// temporary for stubs
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

class MxcImageProvider;

class TimelineViewManager : public QObject
{
        Q_OBJECT

        Q_PROPERTY(
          TimelineModel *timeline MEMBER timeline_ READ activeTimeline NOTIFY activeTimelineChanged)

public:
        TimelineViewManager(QWidget *parent = 0);
        QWidget *getWidget() const { return container; }

        void sync(const mtx::responses::Rooms &rooms);
        void addRoom(const QString &room_id);

        void clearAll() { models.clear(); }

        Q_INVOKABLE TimelineModel *activeTimeline() const { return timeline_; }
        void openImageOverlay(QString mxcUrl,
                              QString originalFilename,
                              QString mimeType,
                              qml_mtx_events::EventType eventType) const;
        void saveMedia(QString mxcUrl,
                       QString originalFilename,
                       QString mimeType,
                       qml_mtx_events::EventType eventType) const;
        Q_INVOKABLE void cacheMedia(QString mxcUrl, QString mimeType);
        // Qml can only pass enum as int
        Q_INVOKABLE void openImageOverlay(QString mxcUrl,
                                          QString originalFilename,
                                          QString mimeType,
                                          int eventType) const
        {
                openImageOverlay(
                  mxcUrl, originalFilename, mimeType, (qml_mtx_events::EventType)eventType);
        }
        Q_INVOKABLE void saveMedia(QString mxcUrl,
                                   QString originalFilename,
                                   QString mimeType,
                                   int eventType) const
        {
                saveMedia(mxcUrl, originalFilename, mimeType, (qml_mtx_events::EventType)eventType);
        }

signals:
        void clearRoomMessageCount(QString roomid);
        void updateRoomsLastMessage(QString roomid, const DescInfo &info);
        void activeTimelineChanged(TimelineModel *timeline);
        void mediaCached(QString mxcUrl, QString cacheUrl);

public slots:
        void updateReadReceipts(const QString &room_id, const std::vector<QString> &event_ids);
        void initWithMessages(const std::map<QString, mtx::responses::Timeline> &msgs);

        void setHistoryView(const QString &room_id);
        void updateColorPalette();

        void queueTextMessage(const QString &msg);
        void queueReplyMessage(const QString &reply, const RelatedInfo &related);
        void queueEmoteMessage(const QString &msg);
        void queueImageMessage(const QString &roomid,
                               const QString &filename,
                               const QString &url,
                               const QString &mime,
                               uint64_t dsize,
                               const QSize &dimensions);
        void queueFileMessage(const QString &roomid,
                              const QString &filename,
                              const QString &url,
                              const QString &mime,
                              uint64_t dsize);
        void queueAudioMessage(const QString &roomid,
                               const QString &filename,
                               const QString &url,
                               const QString &mime,
                               uint64_t dsize);
        void queueVideoMessage(const QString &roomid,
                               const QString &filename,
                               const QString &url,
                               const QString &mime,
                               uint64_t dsize);

private:
        QQuickView *view;
        QWidget *container;
        TimelineModel *timeline_ = nullptr;
        MxcImageProvider *imgProvider;

        QHash<QString, QSharedPointer<TimelineModel>> models;
};

#pragma GCC diagnostic pop
