#pragma once

#include <QObject>
#include <QString>

#include <mtx/events/guest_access.hpp>

#include "CacheStructs.h"

class RoomSettings : public QObject
{
		Q_OBJECT
        Q_PROPERTY(QString roomName READ roomName CONSTANT)
        Q_PROPERTY(int notifications READ notifications NOTIFY notificationsChanged)
        Q_PROPERTY(int accessJoinRules READ accessJoinRules NOTIFY accessJoinRulesChanged)
        Q_PROPERTY(bool canChangeJoinRules READ canChangeJoinRules CONSTANT)
        Q_PROPERTY(bool isEncryptionEnabled READ isEncryptionEnabled NOTIFY encryptionChanged)
        Q_PROPERTY(bool respondsToKeyRequests READ respondsToKeyRequests NOTIFY keyRequestsChanged)

public:
        RoomSettings(QString roomid, QObject *parent = nullptr);

        QString roomName() const;
        int notifications();
        int accessJoinRules();
        bool respondsToKeyRequests();
        //! Whether the user has enough power level to send m.room.join_rules events.
        bool canChangeJoinRules() const;
        bool isEncryptionEnabled() const;

        Q_INVOKABLE void changeNotifications(int currentIndex);
        Q_INVOKABLE void changeAccessRules(int index);
        Q_INVOKABLE void changeKeyRequestsPreference(bool isOn);
        Q_INVOKABLE void enableEncryption();

signals:
        void notificationsChanged();
        void accessJoinRulesChanged();
        void keyRequestsChanged();
        void encryptionChanged();

private:
        void retrieveRoomInfo();
        void updateAccessRules(const std::string &room_id,
                               const mtx::events::state::JoinRules &,
                               const mtx::events::state::GuestAccess &);

private:
        QString roomid_;
        bool usesEncryption_ = false;
        RoomInfo info_;
        int notifications_ = 0;
        int accessRules_   = 0;
};