#pragma once

#include <QHash>
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
#include "emoji/EmojiModel.h"
#include "emoji/Provider.h"

class MxcImageProvider;
class BlurhashProvider;
class CallManager;
class ColorImageProvider;
class UserSettings;
class ChatPage;

class TimelineViewManager : public QObject
{
        Q_OBJECT

        Q_PROPERTY(
          TimelineModel *timeline MEMBER timeline_ READ activeTimeline NOTIFY activeTimelineChanged)
        Q_PROPERTY(
          bool isInitialSync MEMBER isInitialSync_ READ isInitialSync NOTIFY initialSyncChanged)
        Q_PROPERTY(
          bool isNarrowView MEMBER isNarrowView_ READ isNarrowView NOTIFY narrowViewChanged)

public:
        TimelineViewManager(QSharedPointer<UserSettings> userSettings,
                            CallManager *callManager,
                            ChatPage *parent = nullptr);
        QWidget *getWidget() const { return container; }

        void sync(const mtx::responses::Rooms &rooms);
        void addRoom(const QString &room_id);

        void clearAll() { models.clear(); }

        Q_INVOKABLE TimelineModel *activeTimeline() const { return timeline_; }
        Q_INVOKABLE bool isInitialSync() const { return isInitialSync_; }
        bool isNarrowView() const { return isNarrowView_; }
        Q_INVOKABLE void openImageOverlay(QString mxcUrl, QString eventId) const;
        Q_INVOKABLE QColor userColor(QString id, QColor background);
        Q_INVOKABLE QString escapeEmoji(QString str) const;

        Q_INVOKABLE QString userPresence(QString id) const;
        Q_INVOKABLE QString userStatus(QString id) const;

        Q_INVOKABLE void openLink(QString link) const;

        Q_INVOKABLE void openInviteUsersDialog();
        Q_INVOKABLE void openMemberListDialog() const;
        Q_INVOKABLE void openLeaveRoomDialog() const;
        Q_INVOKABLE void openRoomSettings() const;

signals:
        void clearRoomMessageCount(QString roomid);
        void updateRoomsLastMessage(QString roomid, const DescInfo &info);
        void activeTimelineChanged(TimelineModel *timeline);
        void initialSyncChanged(bool isInitialSync);
        void replyingEventChanged(QString replyingEvent);
        void replyClosed();
        void inviteUsers(QStringList users);
        void showRoomList();
        void narrowViewChanged();

public slots:
        void updateReadReceipts(const QString &room_id, const std::vector<QString> &event_ids);
        void initWithMessages(const std::map<QString, mtx::responses::Timeline> &msgs);

        void setHistoryView(const QString &room_id);
        void updateColorPalette();
        void queueReactionMessage(const QString &reactedEvent, const QString &reactionKey);
        void queueTextMessage(const QString &msg);
        void queueEmoteMessage(const QString &msg);
        void queueImageMessage(const QString &roomid,
                               const QString &filename,
                               const std::optional<mtx::crypto::EncryptedFile> &file,
                               const QString &url,
                               const QString &mime,
                               uint64_t dsize,
                               const QSize &dimensions,
                               const QString &blurhash);
        void queueFileMessage(const QString &roomid,
                              const QString &filename,
                              const std::optional<mtx::crypto::EncryptedFile> &file,
                              const QString &url,
                              const QString &mime,
                              uint64_t dsize);
        void queueAudioMessage(const QString &roomid,
                               const QString &filename,
                               const std::optional<mtx::crypto::EncryptedFile> &file,
                               const QString &url,
                               const QString &mime,
                               uint64_t dsize);
        void queueVideoMessage(const QString &roomid,
                               const QString &filename,
                               const std::optional<mtx::crypto::EncryptedFile> &file,
                               const QString &url,
                               const QString &mime,
                               uint64_t dsize);
        void queueCallMessage(const QString &roomid, const mtx::events::msg::CallInvite &);
        void queueCallMessage(const QString &roomid, const mtx::events::msg::CallCandidates &);
        void queueCallMessage(const QString &roomid, const mtx::events::msg::CallAnswer &);
        void queueCallMessage(const QString &roomid, const mtx::events::msg::CallHangUp &);

        void updateEncryptedDescriptions();

        void clearCurrentRoomTimeline()
        {
                if (timeline_)
                        timeline_->clearTimeline();
        }

        void enableBackButton()
        {
                if (isNarrowView_)
                        return;
                isNarrowView_ = true;
                emit narrowViewChanged();
        }
        void disableBackButton()
        {
                if (!isNarrowView_)
                        return;
                isNarrowView_ = false;
                emit narrowViewChanged();
        }

        void backToRooms() { emit showRoomList(); }

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
        TimelineModel *timeline_  = nullptr;
        CallManager *callManager_ = nullptr;

        bool isInitialSync_ = true;
        bool isNarrowView_  = false;

        QSharedPointer<UserSettings> settings;
        QHash<QString, QColor> userColors;
};
