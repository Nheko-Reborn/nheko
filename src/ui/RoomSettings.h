// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QSet>
#include <QString>

#include <mtx/events/event_type.hpp>
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
    void topicEventSent(const QString &);
    void stopLoading();
};

class RoomSettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString roomId READ roomId CONSTANT)
    Q_PROPERTY(QString roomVersion READ roomVersion CONSTANT)
    Q_PROPERTY(QString roomName READ roomName NOTIFY roomNameChanged)
    Q_PROPERTY(QString roomTopic READ roomTopic NOTIFY roomTopicChanged)
    Q_PROPERTY(QString plainRoomName READ plainRoomName NOTIFY roomNameChanged)
    Q_PROPERTY(QString plainRoomTopic READ plainRoomTopic NOTIFY roomTopicChanged)
    Q_PROPERTY(QString roomAvatarUrl READ roomAvatarUrl NOTIFY avatarUrlChanged)
    Q_PROPERTY(int memberCount READ memberCount CONSTANT)
    Q_PROPERTY(int notifications READ notifications NOTIFY notificationsChanged)
    Q_PROPERTY(int accessJoinRules READ accessJoinRules NOTIFY accessJoinRulesChanged)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY loadingChanged)
    Q_PROPERTY(bool canChangeAvatar READ canChangeAvatar CONSTANT)
    Q_PROPERTY(bool canChangeJoinRules READ canChangeJoinRules CONSTANT)
    Q_PROPERTY(bool canChangeName READ canChangeName CONSTANT)
    Q_PROPERTY(bool canChangeTopic READ canChangeTopic CONSTANT)
    Q_PROPERTY(bool isEncryptionEnabled READ isEncryptionEnabled NOTIFY encryptionChanged)
    Q_PROPERTY(bool supportsKnocking READ supportsKnocking CONSTANT)
    Q_PROPERTY(bool supportsRestricted READ supportsRestricted CONSTANT)
    Q_PROPERTY(bool supportsKnockRestricted READ supportsKnockRestricted CONSTANT)

public:
    RoomSettings(QString roomid, QObject *parent = nullptr);

    QString roomId() const;
    QString roomName() const;
    QString roomTopic() const;
    QString plainRoomName() const;
    QString plainRoomTopic() const;
    QString roomVersion() const;
    QString roomAvatarUrl();
    int memberCount() const;
    int notifications();
    int accessJoinRules();
    bool isLoading() const;
    //! Whether the user has enough power level to send m.room.join_rules events.
    bool canChangeJoinRules() const;
    //! Whether the user has enough power level to send m.room.name.
    bool canChangeName() const;
    //! Whether the user has enough power level to send m.room.topic events.
    bool canChangeTopic() const;
    //! Whether the user has enough power level to send m.room.avatar event.
    bool canChangeAvatar() const;
    bool isEncryptionEnabled() const;
    bool supportsKnocking() const;
    bool supportsRestricted() const;
    bool supportsKnockRestricted() const;

    Q_INVOKABLE void enableEncryption();
    Q_INVOKABLE void updateAvatar();
    Q_INVOKABLE void changeAccessRules(int index);
    Q_INVOKABLE void changeNotifications(int currentIndex);
    Q_INVOKABLE void changeTopic(QString topic);
    Q_INVOKABLE void changeName(QString name);

signals:
    void loadingChanged();
    void roomNameChanged();
    void roomTopicChanged();
    void avatarUrlChanged();
    void encryptionChanged();
    void notificationsChanged();
    void accessJoinRulesChanged();
    void displayError(const QString &errorMessage);

public slots:
    void stopLoading();
    void avatarChanged();

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
