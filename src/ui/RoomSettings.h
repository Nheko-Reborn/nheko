#pragma once

#include <QObject>
#include <QString>

#include <mtx/events/guest_access.hpp>

#include "CacheStructs.h"

/// Convenience class which connects events emmited from threads
/// outside of main with the UI code.
class ThreadProxy : public QObject
{
        Q_OBJECT

signals:
        void error(const QString &msg);
        void nameEventSent(const QString &);
        void topicEventSent();
        void stopLoading();
};

class RoomSettings : public QObject
{
        Q_OBJECT
        Q_PROPERTY(QString roomName READ roomName CONSTANT)
        Q_PROPERTY(QString roomId READ roomId CONSTANT)
        Q_PROPERTY(QString roomVersion READ roomVersion CONSTANT)
        Q_PROPERTY(QString roomAvatarUrl READ roomAvatarUrl NOTIFY avatarUrlChanged)
        Q_PROPERTY(int memberCount READ memberCount CONSTANT)
        Q_PROPERTY(int notifications READ notifications NOTIFY notificationsChanged)
        Q_PROPERTY(int accessJoinRules READ accessJoinRules NOTIFY accessJoinRulesChanged)
        Q_PROPERTY(bool isLoading READ isLoading NOTIFY loadingChanged)
        Q_PROPERTY(bool canChangeJoinRules READ canChangeJoinRules CONSTANT)
        Q_PROPERTY(bool canChangeNameAndTopic READ canChangeNameAndTopic CONSTANT)
        Q_PROPERTY(bool canChangeAvatar READ canChangeAvatar CONSTANT)
        Q_PROPERTY(bool isEncryptionEnabled READ isEncryptionEnabled NOTIFY encryptionChanged)
        Q_PROPERTY(bool respondsToKeyRequests READ respondsToKeyRequests NOTIFY keyRequestsChanged)

public:
        RoomSettings(QString roomid, QObject *parent = nullptr);

        QString roomName() const;
        QString roomId() const;
        QString roomVersion() const;
        QString roomAvatarUrl();
        int memberCount() const;
        int notifications();
        int accessJoinRules();
        bool respondsToKeyRequests();
        bool isLoading() const;
        //! Whether the user has enough power level to send m.room.join_rules events.
        bool canChangeJoinRules() const;
        //! Whether the user has enough power level to send m.room.name & m.room.topic events.
        bool canChangeNameAndTopic() const;
        //! Whether the user has enough power level to send m.room.avatar event.
        bool canChangeAvatar() const;
        bool isEncryptionEnabled() const;

        Q_INVOKABLE void changeNotifications(int currentIndex);
        Q_INVOKABLE void changeAccessRules(int index);
        Q_INVOKABLE void changeKeyRequestsPreference(bool isOn);
        Q_INVOKABLE void enableEncryption();
        Q_INVOKABLE void updateAvatar();

signals:
        void notificationsChanged();
        void accessJoinRulesChanged();
        void keyRequestsChanged();
        void encryptionChanged();
        void avatarUrlChanged();
        void loadingChanged();
        void displayError(const QString &errorMessage);

public slots:
        void avatarChanged();
        void stopLoading();

private:
        void retrieveRoomInfo();
        void updateAccessRules(const std::string &room_id,
                               const mtx::events::state::JoinRules &,
                               const mtx::events::state::GuestAccess &);

private:
        QString roomid_;
        bool usesEncryption_ = false;
        bool isLoading_      = false;
        RoomInfo info_;
        int notifications_ = 0;
        int accessRules_   = 0;
};