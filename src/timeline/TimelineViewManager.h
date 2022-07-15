// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QHash>
#include <QQuickItem>
#include <QQuickTextDocument>
#include <QWidget>

#include <mtx/common.hpp>
#include <mtx/responses/messages.hpp>

#include "ReadReceiptsModel.h"
#include "timeline/CommunitiesModel.h"
#include "timeline/PresenceEmitter.h"
#include "timeline/RoomlistModel.h"
#include "ui/RoomSettings.h"
#include "ui/UserProfile.h"

class MxcImageProvider;
class BlurhashProvider;
class ColorImageProvider;
class UserSettings;
class ChatPage;
class ImagePackListModel;
class TimelineModel;
class CallManager;
class VerificationManager;

namespace mtx::responses {
struct Sync;
}

class TimelineViewManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(
      bool isInitialSync MEMBER isInitialSync_ READ isInitialSync NOTIFY initialSyncChanged)
    Q_PROPERTY(bool isConnected READ isConnected NOTIFY isConnectedChanged)

public:
    TimelineViewManager(CallManager *callManager, ChatPage *parent = nullptr);

    void sync(const mtx::responses::Sync &sync_);

    VerificationManager *verificationManager() { return verificationManager_; }

    void clearAll() { rooms_->clear(); }

    Q_INVOKABLE bool isInitialSync() const { return isInitialSync_; }
    bool isConnected() const { return isConnected_; }
    Q_INVOKABLE void openImageOverlay(TimelineModel *room,
                                      QString mxcUrl,
                                      QString eventId,
                                      double originalWidth,
                                      double proportionalHeight);
    Q_INVOKABLE void openImagePackSettings(QString roomid);
    Q_INVOKABLE void saveMedia(QString mxcUrl);
    Q_INVOKABLE QColor userColor(QString id, QColor background);
    Q_INVOKABLE QString escapeEmoji(QString str) const;
    Q_INVOKABLE QString htmlEscape(QString str) const { return str.toHtmlEscaped(); }

    Q_INVOKABLE void openRoomMembers(TimelineModel *room);
    Q_INVOKABLE void openRoomSettings(QString room_id);
    Q_INVOKABLE void openInviteUsers(QString roomId);
    Q_INVOKABLE void openGlobalUserProfile(QString userId);
    Q_INVOKABLE UserProfile *getGlobalUserProfile(QString userId);

    Q_INVOKABLE void focusMessageInput();

    Q_INVOKABLE void fixImageRendering(QQuickTextDocument *t, QQuickItem *i);

signals:
    void activeTimelineChanged(TimelineModel *timeline);
    void initialSyncChanged(bool isInitialSync);
    void isConnectedChanged(bool state);
    void replyingEventChanged(QString replyingEvent);
    void replyClosed();
    void inviteUsers(QString roomId, QStringList users);
    void showRoomList();
    void narrowViewChanged();
    void focusInput();
    void openRoomMembersDialog(MemberList *members, TimelineModel *room);
    void openRoomSettingsDialog(RoomSettings *settings);
    void openInviteUsersDialog(InviteesModel *invitees);
    void openProfile(UserProfile *profile);
    void showImagePackSettings(TimelineModel *room, ImagePackListModel *packlist);
    void openLeaveRoomDialog(QString roomid, QString reason = "");
    void showImageOverlay(TimelineModel *room,
                          QString eventId,
                          QString url,
                          double originalWidth,
                          double proportionalHeight);

public slots:
    void updateReadReceipts(const QString &room_id, const std::vector<QString> &event_ids);
    void receivedSessionKey(const std::string &room_id, const std::string &session_id);
    void initializeRoomlist();

    void showEvent(const QString &room_id, const QString &event_id);

    void updateColorPalette();
    void queueReply(const QString &roomid, const QString &repliedToEvent, const QString &replyBody);
    void queueCallMessage(const QString &roomid, const mtx::events::voip::CallInvite &);
    void queueCallMessage(const QString &roomid, const mtx::events::voip::CallCandidates &);
    void queueCallMessage(const QString &roomid, const mtx::events::voip::CallAnswer &);
    void queueCallMessage(const QString &roomid, const mtx::events::voip::CallHangUp &);

    void setVideoCallItem();

    QObject *completerFor(QString completerName, QString roomId = QLatin1String(QLatin1String("")));
    void forwardMessageToRoom(mtx::events::collections::TimelineEvents *e, QString roomId);

    RoomlistModel *rooms() { return rooms_; }

private:
    bool isInitialSync_ = true;
    bool isConnected_   = true;

    RoomlistModel *rooms_          = nullptr;
    CommunitiesModel *communities_ = nullptr;

    // don't move this above the rooms_
    VerificationManager *verificationManager_ = nullptr;
    PresenceEmitter *presenceEmitter          = nullptr;

    QHash<QPair<QString, quint64>, QColor> userColors;
};
Q_DECLARE_METATYPE(mtx::events::msg::KeyVerificationAccept)
Q_DECLARE_METATYPE(mtx::events::msg::KeyVerificationCancel)
Q_DECLARE_METATYPE(mtx::events::msg::KeyVerificationDone)
Q_DECLARE_METATYPE(mtx::events::msg::KeyVerificationKey)
Q_DECLARE_METATYPE(mtx::events::msg::KeyVerificationMac)
Q_DECLARE_METATYPE(mtx::events::msg::KeyVerificationReady)
Q_DECLARE_METATYPE(mtx::events::msg::KeyVerificationRequest)
Q_DECLARE_METATYPE(mtx::events::msg::KeyVerificationStart)
