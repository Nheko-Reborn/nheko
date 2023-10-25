// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QHash>
#include <QQuickItem>
#include <QQuickTextDocument>

#include <mtx/common.hpp>
#include <mtx/responses/messages.hpp>

#include "InviteesModel.h"
#include "MemberList.h"
#include "timeline/CommunitiesModel.h"
#include "timeline/PresenceEmitter.h"
#include "timeline/RoomlistModel.h"
#include "ui/RoomSettings.h"
#include "ui/UserProfile.h"

class UserSettings;
class ChatPage;
class ImagePackListModel;
class TimelineModel;
class CallManager;
class VerificationManager;

namespace mtx::responses {
struct Sync;
}

class TimelineViewManager final : public QObject
{
    Q_OBJECT

    QML_NAMED_ELEMENT(TimelineManager)
    QML_SINGLETON

    Q_PROPERTY(
      bool isInitialSync MEMBER isInitialSync_ READ isInitialSync NOTIFY initialSyncChanged)
    Q_PROPERTY(bool isConnected READ isConnected NOTIFY isConnectedChanged)
    Q_PROPERTY(QVector<QString> ignoredUsers READ getIgnoredUsers NOTIFY ignoredUsersChanged)

public:
    TimelineViewManager(CallManager *callManager, ChatPage *parent = nullptr);

    static TimelineViewManager *create(QQmlEngine *qmlEngine, QJSEngine *)
    {
        // The instance has to exist before it is used. We cannot replace it.
        Q_ASSERT(instance_);

        // The engine has to have the same thread affinity as the singleton.
        Q_ASSERT(qmlEngine->thread() == instance_->thread());

        // There can only be one engine accessing the singleton.
        static QJSEngine *s_engine = nullptr;
        if (s_engine)
            Q_ASSERT(qmlEngine == s_engine);
        else
            s_engine = qmlEngine;

        QJSEngine::setObjectOwnership(instance_, QJSEngine::CppOwnership);
        return instance_;
    }

    static TimelineViewManager *instance() { return TimelineViewManager::instance_; }

    QVector<QString> getIgnoredUsers();

    void sync(const mtx::responses::Sync &sync_);

    VerificationManager *verificationManager() { return verificationManager_; }

    void clearAll() { rooms_->clear(); }

    Q_INVOKABLE bool isInitialSync() const { return isInitialSync_; }
    bool isConnected() const { return isConnected_; }
    Q_INVOKABLE void openImageOverlay(TimelineModel *room,
                                      const QString &mxcUrl,
                                      const QString &eventId,
                                      double originalWidth,
                                      double proportionalHeight);
    Q_INVOKABLE void openImagePackSettings(QString roomid);
    Q_INVOKABLE void saveMedia(QString mxcUrl);
    Q_INVOKABLE void copyImage(const QString &mxcUrl) const;
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
    void ignoredUsersChanged(const QVector<QString> &ignoredUsers);

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
    void queueCallMessage(const QString &roomid, const mtx::events::voip::CallSelectAnswer &);
    void queueCallMessage(const QString &roomid, const mtx::events::voip::CallReject &);
    void queueCallMessage(const QString &roomid, const mtx::events::voip::CallNegotiate &);

    void setVideoCallItem();

    QAbstractItemModel *completerFor(const QString &completerName,
                                     const QString &roomId = QLatin1String(QLatin1String("")));
    void forwardMessageToRoom(mtx::events::collections::TimelineEvents const *e, QString roomId);

    RoomlistModel *rooms() { return rooms_; }

private:
    bool isInitialSync_ = true;
    bool isConnected_   = true;

    RoomlistModel *rooms_          = nullptr;
    FilteredRoomlistModel *frooms_ = nullptr;
    CommunitiesModel *communities_ = nullptr;

    // don't move this above the rooms_
    VerificationManager *verificationManager_ = nullptr;
    PresenceEmitter *presenceEmitter          = nullptr;

    QHash<QPair<QString, quint64>, QColor> userColors;

    inline static TimelineViewManager *instance_ = nullptr;

    void processIgnoredUsers(const mtx::responses::AccountData &data);
};
