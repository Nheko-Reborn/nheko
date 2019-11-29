#pragma once

#include <QQuickView>
#include <QQuickWidget>
#include <QSharedPointer>
#include <QWidget>

#include <mtx/responses.hpp>

#include "Cache.h"
#include "Logging.h"
#include "TimelineModel.h"
#include "Utils.h"

class MxcImageProvider;
class ColorImageProvider;

class TimelineViewManager : public QObject
{
        Q_OBJECT

        Q_PROPERTY(
          TimelineModel *timeline MEMBER timeline_ READ activeTimeline NOTIFY activeTimelineChanged)
        Q_PROPERTY(
          bool isInitialSync MEMBER isInitialSync_ READ isInitialSync NOTIFY initialSyncChanged)

public:
        TimelineViewManager(QWidget *parent = 0);
        QWidget *getWidget() const { return container; }

        void sync(const mtx::responses::Rooms &rooms);
        void addRoom(const QString &room_id);

        void clearAll() { models.clear(); }

        Q_INVOKABLE TimelineModel *activeTimeline() const { return timeline_; }
        Q_INVOKABLE bool isInitialSync() const { return isInitialSync_; }
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
        void initialSyncChanged(bool isInitialSync);
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
#ifdef USE_QUICK_VIEW
        QQuickView *view;
#else
        QQuickWidget *view;
#endif
        QWidget *container;

        MxcImageProvider *imgProvider;
        ColorImageProvider *colorImgProvider;

        QHash<QString, QSharedPointer<TimelineModel>> models;
        TimelineModel *timeline_ = nullptr;
        bool isInitialSync_      = true;
};
