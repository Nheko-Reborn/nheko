#pragma once

#include <QHash>
#include <QQuickView>
#include <QQuickWidget>
#include <QSharedPointer>
#include <QWidget>

#include <mtx/common.hpp>
#include <mtx/responses.hpp>

#include "Cache.h"
#include "CallManager.h"
#include "DeviceVerificationFlow.h"
#include "Logging.h"
#include "TimelineModel.h"
#include "Utils.h"
#include "WebRTCSession.h"
#include "emoji/EmojiModel.h"
#include "emoji/Provider.h"

class MxcImageProvider;
class BlurhashProvider;
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
        Q_PROPERTY(webrtc::State callState READ callState NOTIFY callStateChanged)
        Q_PROPERTY(bool onVideoCall READ onVideoCall NOTIFY videoCallChanged)
        Q_PROPERTY(QString callPartyName READ callPartyName NOTIFY callPartyChanged)
        Q_PROPERTY(QString callPartyAvatarUrl READ callPartyAvatarUrl NOTIFY callPartyChanged)
        Q_PROPERTY(bool isMicMuted READ isMicMuted NOTIFY micMuteChanged)

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
        webrtc::State callState() const { return WebRTCSession::instance().state(); }
        bool onVideoCall() const { return WebRTCSession::instance().isVideo(); }
        Q_INVOKABLE void setVideoCallItem();
        QString callPartyName() const { return callManager_->callPartyName(); }
        QString callPartyAvatarUrl() const { return callManager_->callPartyAvatarUrl(); }
        bool isMicMuted() const { return WebRTCSession::instance().isMicMuted(); }
        Q_INVOKABLE void toggleMicMute();
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
        Q_INVOKABLE void removeVerificationFlow(DeviceVerificationFlow *flow);

        void verifyUser(QString userid);
        void verifyDevice(QString userid, QString deviceid);

signals:
        void clearRoomMessageCount(QString roomid);
        void updateRoomsLastMessage(QString roomid, const DescInfo &info);
        void activeTimelineChanged(TimelineModel *timeline);
        void initialSyncChanged(bool isInitialSync);
        void replyingEventChanged(QString replyingEvent);
        void replyClosed();
        void newDeviceVerificationRequest(DeviceVerificationFlow *flow);
        void inviteUsers(QStringList users);
        void showRoomList();
        void narrowViewChanged();
        void callStateChanged(webrtc::State);
        void videoCallChanged();
        void callPartyChanged();
        void micMuteChanged();

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

        QHash<QString, QSharedPointer<DeviceVerificationFlow>> dvList;
};
Q_DECLARE_METATYPE(mtx::events::msg::KeyVerificationAccept)
Q_DECLARE_METATYPE(mtx::events::msg::KeyVerificationCancel)
Q_DECLARE_METATYPE(mtx::events::msg::KeyVerificationDone)
Q_DECLARE_METATYPE(mtx::events::msg::KeyVerificationKey)
Q_DECLARE_METATYPE(mtx::events::msg::KeyVerificationMac)
Q_DECLARE_METATYPE(mtx::events::msg::KeyVerificationReady)
Q_DECLARE_METATYPE(mtx::events::msg::KeyVerificationRequest)
Q_DECLARE_METATYPE(mtx::events::msg::KeyVerificationStart)
