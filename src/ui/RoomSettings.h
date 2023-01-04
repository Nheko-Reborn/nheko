// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-FileCopyrightText: 2023 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QSet>
#include <QString>

#include <unordered_set>

#include <mtx/events/event_type.hpp>
#include <mtx/events/guest_access.hpp>

#include "CacheStructs.h"

/// Convenience class which connects events emmited from threads
/// outside of main with the UI code.
class ThreadProxy final : public QObject
{
    Q_OBJECT

signals:
    void error(const QString &msg);
    void nameEventSent(const QString &);
    void topicEventSent(const QString &);
    void stopLoading();
};

class RoomSettings;

class RoomSettingsAllowedRoomsModel final : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles
    {
        Name,
        IsAllowed,
        IsSpaceParent,
    };

    explicit RoomSettingsAllowedRoomsModel(RoomSettings *parent);

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool
    setData(const QModelIndex &index, const QVariant &value, int role = Qt::DisplayRole) override;
    Q_INVOKABLE void addRoom(QString room);

    Qt::ItemFlags flags(const QModelIndex &) const override
    {
        return Qt::ItemIsEditable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled |
               Qt::ItemNeverHasChildren;
    }

    QStringList allowedRoomIds;

private:
    QStringList listedRoomIds;
    std::unordered_set<QString> parentSpaces;
    RoomSettings *settings;
};

class RoomSettings final : public QObject
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
    Q_PROPERTY(bool privateAccess READ privateAccess NOTIFY accessJoinRulesChanged)
    Q_PROPERTY(bool guestAccess READ guestAccess NOTIFY accessJoinRulesChanged)
    Q_PROPERTY(bool knockingEnabled READ knockingEnabled NOTIFY accessJoinRulesChanged)
    Q_PROPERTY(bool restrictedEnabled READ restrictedEnabled NOTIFY accessJoinRulesChanged)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY loadingChanged)
    Q_PROPERTY(bool canChangeAvatar READ canChangeAvatar CONSTANT)
    Q_PROPERTY(bool canChangeJoinRules READ canChangeJoinRules CONSTANT)
    Q_PROPERTY(bool canChangeName READ canChangeName CONSTANT)
    Q_PROPERTY(bool canChangeTopic READ canChangeTopic CONSTANT)
    Q_PROPERTY(bool isEncryptionEnabled READ isEncryptionEnabled NOTIFY encryptionChanged)
    Q_PROPERTY(bool supportsKnocking READ supportsKnocking CONSTANT)
    Q_PROPERTY(bool supportsRestricted READ supportsRestricted CONSTANT)
    Q_PROPERTY(bool supportsKnockRestricted READ supportsKnockRestricted CONSTANT)
    Q_PROPERTY(
      QStringList allowedRooms READ allowedRooms WRITE setAllowedRooms NOTIFY allowedRoomsChanged)
    Q_PROPERTY(RoomSettingsAllowedRoomsModel *allowedRoomsModel MEMBER allowedRoomsModel CONSTANT)
    Q_PROPERTY(
      bool allowedRoomsModified READ allowedRoomsModified NOTIFY allowedRoomsModifiedChanged)

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
    bool privateAccess() const;
    bool guestAccess() const;
    bool knockingEnabled() const;
    bool restrictedEnabled() const;
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
    QStringList allowedRooms() const;
    void setAllowedRooms(QStringList rooms);
    bool allowedRoomsModified() const { return allowedRoomsModified_; }

    Q_INVOKABLE void enableEncryption();
    Q_INVOKABLE void updateAvatar();
    Q_INVOKABLE void changeAccessRules(bool private_,
                                       bool guestsAllowed,
                                       bool knockingAllowed,
                                       bool restrictedAllowed);
    Q_INVOKABLE void changeNotifications(int currentIndex);
    Q_INVOKABLE void changeTopic(const QString &topic);
    Q_INVOKABLE void changeName(const QString &name);

    Q_INVOKABLE void applyAllowedFromModel();

signals:
    void loadingChanged();
    void roomNameChanged();
    void roomTopicChanged();
    void avatarUrlChanged();
    void encryptionChanged();
    void notificationsChanged();
    void accessJoinRulesChanged();
    void allowedRoomsChanged();
    void displayError(const QString &errorMessage);
    void allowedRoomsModifiedChanged();

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
    bool usesEncryption_       = false;
    bool isLoading_            = false;
    bool allowedRoomsModified_ = false;
    RoomInfo info_;
    int notifications_ = 0;

    mtx::events::state::JoinRules accessRules_;
    mtx::events::state::AccessState guestRules_ = mtx::events::state::AccessState::Forbidden;

    RoomSettingsAllowedRoomsModel *allowedRoomsModel;
};
