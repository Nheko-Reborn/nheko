// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <optional>

#include <QObject>

#include <mtx/responses/public_rooms.hpp>

class RoomSummaryProxy : public QObject
{
    Q_OBJECT

public:
    RoomSummaryProxy() {}
signals:
    void loaded(mtx::responses::PublicRoom room);
    void failed();
};

class RoomSummary : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString reason READ reason WRITE setReason NOTIFY reasonChanged)

    Q_PROPERTY(QString roomid READ roomid NOTIFY loaded)
    Q_PROPERTY(QString roomName READ roomName NOTIFY loaded)
    Q_PROPERTY(QString roomTopic READ roomTopic NOTIFY loaded)
    Q_PROPERTY(QString roomAvatarUrl READ roomAvatarUrl NOTIFY loaded)
    Q_PROPERTY(bool isInvite READ isInvite NOTIFY loaded)
    Q_PROPERTY(bool isSpace READ isSpace NOTIFY loaded)
    Q_PROPERTY(bool isKnockOnly READ isKnockOnly NOTIFY loaded)
    Q_PROPERTY(bool isLoaded READ isLoaded NOTIFY loaded)
    Q_PROPERTY(int memberCount READ memberCount NOTIFY loaded)

public:
    explicit RoomSummary(std::string roomIdOrAlias_,
                         std::vector<std::string> vias_,
                         QString reason_,
                         QObject *p = nullptr);

    void setReason(const QString &r)
    {
        reason_ = r;
        emit reasonChanged();
    }
    QString reason() const { return reason_; }

    QString roomid() const { return room ? QString::fromStdString(room->room_id) : ""; }
    QString roomName() const { return QString::fromStdString(room ? room->name : roomIdOrAlias); }
    QString roomTopic() const { return room ? QString::fromStdString(room->topic) : ""; }
    QString roomAvatarUrl() const { return room ? QString::fromStdString(room->avatar_url) : ""; }
    bool isInvite() const
    {
        return room && room->membership == mtx::events::state::Membership::Invite;
    }
    bool isSpace() const { return room && room->room_type == mtx::events::state::room_type::space; }
    int memberCount() const { return room ? room->num_joined_members : 0; }
    bool isKnockOnly() const
    {
        return room && (room->join_rule == mtx::events::state::JoinRule::Knock ||
                        room->join_rule == mtx::events::state::JoinRule::KnockRestricted);
    }

    bool isLoaded() const { return room.has_value() || loaded_; }

    Q_INVOKABLE void join();

signals:
    void loaded();
    void reasonChanged();

private:
    std::string roomIdOrAlias;
    std::vector<std::string> vias;
    std::optional<mtx::responses::PublicRoom> room;
    QString reason_;
    bool loaded_ = false;
};
