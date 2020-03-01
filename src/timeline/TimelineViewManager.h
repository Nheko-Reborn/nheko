#pragma once

#include <QQuickView>
#include <QQuickWidget>
#include <QSharedPointer>
#include <QWidget>

#include <mtx/common.hpp>
#include <mtx/responses.hpp>

#include "Cache.h"
#include "Logging.h"
#include "TimelineModel.h"
#include "Utils.h"

class MxcImageProvider;
class BlurhashProvider;
class ColorImageProvider;
class UserSettings;

class TimelineViewManager : public QObject
{
        Q_OBJECT

        Q_PROPERTY(
          TimelineModel *timeline MEMBER timeline_ READ activeTimeline NOTIFY activeTimelineChanged)
        Q_PROPERTY(
          bool isInitialSync MEMBER isInitialSync_ READ isInitialSync NOTIFY initialSyncChanged)
        Q_PROPERTY(QString replyingEvent READ getReplyingEvent WRITE updateReplyingEvent NOTIFY
                     replyingEventChanged)

public:
        TimelineViewManager(QSharedPointer<UserSettings> userSettings, QWidget *parent = nullptr);
        QWidget *getWidget() const { return container; }

        void sync(const mtx::responses::Rooms &rooms);
        void addRoom(const QString &room_id);

        void clearAll() { models.clear(); }

        Q_INVOKABLE TimelineModel *activeTimeline() const { return timeline_; }
        Q_INVOKABLE bool isInitialSync() const { return isInitialSync_; }
        Q_INVOKABLE void openImageOverlay(QString mxcUrl, QString eventId) const;
        Q_INVOKABLE QColor userColor(QString id, QColor background);

signals:
        void clearRoomMessageCount(QString roomid);
        void updateRoomsLastMessage(QString roomid, const DescInfo &info);
        void activeTimelineChanged(TimelineModel *timeline);
        void initialSyncChanged(bool isInitialSync);
        void replyingEventChanged(QString replyingEvent);
        void replyClosed();

public slots:
        void updateReplyingEvent(const QString &replyingEvent)
        {
                if (this->replyingEvent_ != replyingEvent) {
                        this->replyingEvent_ = replyingEvent;
                        emit replyingEventChanged(replyingEvent_);
                }
        }
        void closeReply()
        {
                this->updateReplyingEvent(nullptr);
                emit replyClosed();
        }
        QString getReplyingEvent() const { return replyingEvent_; }
        void updateReadReceipts(const QString &room_id, const std::vector<QString> &event_ids);
        void initWithMessages(const std::map<QString, mtx::responses::Timeline> &msgs);

        void setHistoryView(const QString &room_id);
        void updateColorPalette();

        void queueTextMessage(const QString &msg, const std::optional<RelatedInfo> &related);
        void queueEmoteMessage(const QString &msg);
        void queueImageMessage(const QString &roomid,
                               const QString &filename,
                               const std::optional<mtx::crypto::EncryptedFile> &file,
                               const QString &url,
                               const QString &mime,
                               uint64_t dsize,
                               const QSize &dimensions,
                               const QString &blurhash,
                               const std::optional<RelatedInfo> &related);
        void queueFileMessage(const QString &roomid,
                              const QString &filename,
                              const std::optional<mtx::crypto::EncryptedFile> &file,
                              const QString &url,
                              const QString &mime,
                              uint64_t dsize,
                              const std::optional<RelatedInfo> &related);
        void queueAudioMessage(const QString &roomid,
                               const QString &filename,
                               const std::optional<mtx::crypto::EncryptedFile> &file,
                               const QString &url,
                               const QString &mime,
                               uint64_t dsize,
                               const std::optional<RelatedInfo> &related);
        void queueVideoMessage(const QString &roomid,
                               const QString &filename,
                               const std::optional<mtx::crypto::EncryptedFile> &file,
                               const QString &url,
                               const QString &mime,
                               uint64_t dsize,
                               const std::optional<RelatedInfo> &related);

private:
#ifdef USE_QUICK_VIEW
        QQuickView *view;
#else
        QQuickWidget *view;
#endif
        QWidget *container;

        MxcImageProvider *imgProvider;
        ColorImageProvider *colorImgProvider;
        BlurhashProvider *blurhashProvider;

        QHash<QString, QSharedPointer<TimelineModel>> models;
        TimelineModel *timeline_ = nullptr;
        bool isInitialSync_      = true;
        QString replyingEvent_;

        QSharedPointer<UserSettings> settings;
        QHash<QString, QColor> userColors;
};
