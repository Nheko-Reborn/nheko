// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "PowerlevelsEditModels.h"

#include <algorithm>
#include <set>

#include "Cache.h"
#include "Cache_p.h"
#include "ChatPage.h"
#include "Logging.h"
#include "MatrixClient.h"

PowerlevelsTypeListModel::PowerlevelsTypeListModel(const std::string &rid,
                                                   const mtx::events::state::PowerLevels &pl,
                                                   QObject *parent)
  : QAbstractListModel(parent)
  , room_id(rid)
  , powerLevels_(pl)
{
    std::set<mtx::events::state::power_level_t> seen_levels;
    for (const auto &[type, level] : powerLevels_.events) {
        if (!seen_levels.count(level)) {
            types.push_back(Entry{"", level});
            seen_levels.insert(level);
        }
        types.push_back(Entry{type, level});
    }

    for (const auto &[user, level] : powerLevels_.users) {
        (void)user;
        if (!seen_levels.count(level)) {
            types.push_back(Entry{"", level});
            seen_levels.insert(level);
        }
    }

    for (const auto &level : {
           powerLevels_.events_default,
           powerLevels_.state_default,
           powerLevels_.users_default,
           powerLevels_.ban,
           powerLevels_.kick,
           powerLevels_.invite,
           powerLevels_.redact,
         }) {
        if (!seen_levels.count(level)) {
            types.push_back(Entry{"", level});
            seen_levels.insert(level);
        }
    }

    types.push_back(Entry{"zdefault_states", powerLevels_.state_default});
    types.push_back(Entry{"zdefault_events", powerLevels_.events_default});
    types.push_back(Entry{"ban", powerLevels_.ban});
    types.push_back(Entry{"kick", powerLevels_.kick});
    types.push_back(Entry{"invite", powerLevels_.invite});
    types.push_back(Entry{"redact", powerLevels_.redact});

    std::sort(types.begin(), types.end(), [](const Entry &a, const Entry &b) {
        if (a.pl != b.pl) // sort by PL
            return a.pl > b.pl;
        else if (a.type.empty() != b.type.empty()) // empty types are headers
            return a.type.empty() > b.type.empty();
        else {
            bool a_contains_dot = a.type.find('.') != std::string::npos;
            bool b_contains_dot = b.type.find('.') != std::string::npos;
            if (a_contains_dot != b_contains_dot) // sort stuff like "invite" or "default" last
                return a_contains_dot > b_contains_dot;
            else // rest is sorted alphabetical
                return a.type < b.type;
        }
    });
}

std::map<std::string, mtx::events::state::power_level_t, std::less<>>
PowerlevelsTypeListModel::toEvents()
{
    std::map<std::string, mtx::events::state::power_level_t, std::less<>> m;
    for (const auto &[key, pl] : qAsConst(types))
        if (key.find('.') != std::string::npos)
            m[key] = pl;
    return m;
}
mtx::events::state::power_level_t
PowerlevelsTypeListModel::kick()
{
    for (const auto &[key, pl] : qAsConst(types))
        if (key == "kick")
            return pl;
    return powerLevels_.users_default;
}
mtx::events::state::power_level_t
PowerlevelsTypeListModel::invite()
{
    for (const auto &[key, pl] : qAsConst(types))
        if (key == "invite")
            return pl;
    return powerLevels_.users_default;
}
mtx::events::state::power_level_t
PowerlevelsTypeListModel::ban()
{
    for (const auto &[key, pl] : qAsConst(types))
        if (key == "ban")
            return pl;
    return powerLevels_.users_default;
}
mtx::events::state::power_level_t
PowerlevelsTypeListModel::eventsDefault()
{
    for (const auto &[key, pl] : qAsConst(types))
        if (key == "zdefault_events")
            return pl;
    return powerLevels_.users_default;
}
mtx::events::state::power_level_t
PowerlevelsTypeListModel::stateDefault()
{
    for (const auto &[key, pl] : qAsConst(types))
        if (key == "zdefault_states")
            return pl;
    return powerLevels_.users_default;
}

QHash<int, QByteArray>
PowerlevelsTypeListModel::roleNames() const
{
    return {
      {DisplayName, "displayName"},
      {Powerlevel, "powerlevel"},
      {IsType, "isType"},
      {Moveable, "moveable"},
      {Removeable, "removeable"},
    };
}

QVariant
PowerlevelsTypeListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= types.size())
        return {};

    const auto &type = types.at(index.row());

    switch (static_cast<Roles>(role)) {
    case DisplayName:
        if (type.type == "zdefault_events")
            return tr("Other events");
        else if (type.type == "zdefault_states")
            return tr("Other state events");
        else if (type.type == "kick")
            return tr("Remove other users");
        else if (type.type == "ban")
            return tr("Ban other users");
        else if (type.type == "invite")
            return tr("Invite other users");
        else if (type.type == "redact")
            return tr("Redact events sent by others");
        else if (type.type == "m.reaction")
            return tr("Reactions");
        else if (type.type == "m.room.aliases")
            return tr("Deprecated aliases events");
        else if (type.type == "m.room.avatar")
            return tr("Change the room avatar");
        else if (type.type == "m.room.canonical_alias")
            return tr("Change the room addresses");
        else if (type.type == "m.room.encrypted")
            return tr("Send encrypted messages");
        else if (type.type == "m.room.encryption")
            return tr("Enable encryption");
        else if (type.type == "m.room.guest_access")
            return tr("Change guest access");
        else if (type.type == "m.room.history_visibility")
            return tr("Change history visibility");
        else if (type.type == "m.room.join_rules")
            return tr("Change who can join");
        else if (type.type == "m.room.message")
            return tr("Send messages");
        else if (type.type == "m.room.name")
            return tr("Change the room name");
        else if (type.type == "m.room.power_levels")
            return tr("Change the room permissions");
        else if (type.type == "m.room.topic")
            return tr("Change the rooms topic");
        else if (type.type == "m.widget")
            return tr("Change the widgets");
        else if (type.type == "im.vector.modular.widgets")
            return tr("Change the widgets (experimental)");
        else if (type.type == "m.room.redaction")
            return tr("Redact own events");
        else if (type.type == "m.room.pinned_events")
            return tr("Change the pinned events");
        else if (type.type == "m.room.tombstone")
            return tr("Upgrade the room");
        else if (type.type == "m.sticker")
            return tr("Send stickers");

        else if (type.type == "m.policy.rule.user")
            return tr("Ban users using policy rules");
        else if (type.type == "m.policy.rule.room")
            return tr("Ban rooms using policy rules");
        else if (type.type == "m.policy.rule.server")
            return tr("Ban servers using policy rules");

        else if (type.type == "m.space.child")
            return tr("Edit child rooms");
        else if (type.type == "m.space.parent")
            return tr("Change parent spaces");

        else if (type.type == "m.call.invite")
            return tr("Start a call");
        else if (type.type == "m.call.candidates")
            return tr("Negotiate a call");
        else if (type.type == "m.call.answer")
            return tr("Answer a call");
        else if (type.type == "m.call.hangup")
            return tr("Hang up a call");
        else if (type.type == "im.ponies.room_emotes")
            return tr("Change the room emotes");
        return QString::fromStdString(type.type);
    case Powerlevel:
        return static_cast<qlonglong>(type.pl);
    case IsType:
        return !type.type.empty();
    case Moveable:
        return !type.type.empty();
    case Removeable:
        return !type.type.empty() && type.type.find('.') != std::string::npos;
    }

    return {};
}

bool
PowerlevelsTypeListModel::remove(int row)
{
    if (row < 0 || row >= types.size() || types.at(row).type.empty())
        return false;

    beginRemoveRows(QModelIndex(), row, row);
    types.remove(row);
    endRemoveRows();

    return true;
}

void
PowerlevelsTypeListModel::add(int row, QString type)
{
    if (row < 0 || row > types.size())
        return;

    const auto typeStr = type.toStdString();
    for (int i = 0; i < types.size(); i++) {
        if (types[i].type == typeStr) {
            if (i > row)
                move(i, row + 1);
            else
                move(i, row);
            return;
        }
    }

    beginInsertRows(QModelIndex(), row + 1, row + 1);
    types.insert(row + 1, Entry{type.toStdString(), types.at(row).pl});
    endInsertRows();
}

void
PowerlevelsTypeListModel::addRole(int64_t role)
{
    for (int i = 0; i < types.size(); i++) {
        if (types[i].pl < role) {
            beginInsertRows(QModelIndex(), i, i);
            types.insert(i, Entry{"", role});
            endInsertRows();
            return;
        }
    }

    beginInsertRows(QModelIndex(), types.size(), types.size());
    types.push_back(Entry{"", role});
    endInsertRows();
}

bool
PowerlevelsTypeListModel::move(int from, int to)
{
    if (from == to)
        return false;
    if (from < to)
        to += 1;

    beginMoveRows(QModelIndex(), from, from, QModelIndex(), to);
    auto ret = moveRow(QModelIndex(), from, QModelIndex(), to);
    endMoveRows();
    return ret;
}

bool
PowerlevelsTypeListModel::moveRows(const QModelIndex &,
                                   int sourceRow,
                                   int count,
                                   const QModelIndex &,
                                   int destinationChild)
{
    if (sourceRow == destinationChild)
        return true;

    if (count != 1)
        return false;

    if (sourceRow < 0 || sourceRow >= types.size())
        return false;
    if (destinationChild < 0 || destinationChild > types.size())
        return false;

    if (types.at(sourceRow).type.empty())
        return false;

    auto pl         = types.at(destinationChild > 0 ? destinationChild - 1 : 0).pl;
    auto sourceItem = types.takeAt(sourceRow);
    sourceItem.pl   = pl;

    auto movedType = sourceItem.type;

    if (destinationChild < sourceRow)
        types.insert(destinationChild, std::move(sourceItem));
    else
        types.insert(destinationChild - 1, std::move(sourceItem));

    if (movedType == "m.room.power_levels")
        emit adminLevelChanged();
    else if (movedType == "redact")
        emit moderatorLevelChanged();

    return true;
}

PowerlevelsUserListModel::PowerlevelsUserListModel(const std::string &rid,
                                                   const mtx::events::state::PowerLevels &pl,
                                                   QObject *parent)
  : QAbstractListModel(parent)
  , room_id(rid)
  , powerLevels_(pl)
{
    std::set<mtx::events::state::power_level_t> seen_levels;
    for (const auto &[user, level] : powerLevels_.users) {
        if (!seen_levels.count(level)) {
            users.push_back(Entry{"", level});
            seen_levels.insert(level);
        }
        users.push_back(Entry{user, level});
    }

    for (const auto &[type, level] : powerLevels_.events) {
        (void)type;
        if (!seen_levels.count(level)) {
            users.push_back(Entry{"", level});
            seen_levels.insert(level);
        }
    }

    for (const auto &level : {
           powerLevels_.events_default,
           powerLevels_.state_default,
           powerLevels_.users_default,
           powerLevels_.ban,
           powerLevels_.kick,
           powerLevels_.invite,
           powerLevels_.redact,
         }) {
        if (!seen_levels.count(level)) {
            users.push_back(Entry{"", level});
            seen_levels.insert(level);
        }
    }

    users.push_back(Entry{"default", powerLevels_.users_default});

    std::sort(users.begin(), users.end(), [](const Entry &a, const Entry &b) {
        if (a.pl != b.pl)
            return a.pl > b.pl;
        else
            return a.mxid < b.mxid;
    });
}

std::map<std::string, mtx::events::state::power_level_t, std::less<>>
PowerlevelsUserListModel::toUsers()
{
    std::map<std::string, mtx::events::state::power_level_t, std::less<>> m;
    for (const auto &[key, pl] : qAsConst(users))
        if (key.size() > 0 && key.at(0) == '@')
            m[key] = pl;
    return m;
}
mtx::events::state::power_level_t
PowerlevelsUserListModel::usersDefault()
{
    for (const auto &[key, pl] : qAsConst(users))
        if (key == "default")
            return pl;
    return powerLevels_.users_default;
}

QHash<int, QByteArray>
PowerlevelsUserListModel::roleNames() const
{
    return {
      {Mxid, "mxid"},
      {DisplayName, "displayName"},
      {AvatarUrl, "avatarUrl"},
      {Powerlevel, "powerlevel"},
      {IsUser, "isUser"},
      {Moveable, "moveable"},
      {Removeable, "removeable"},
    };
}

QVariant
PowerlevelsUserListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= users.size())
        return {};

    const auto &user = users.at(index.row());

    switch (static_cast<Roles>(role)) {
    case Mxid:
        if ("default" == user.mxid)
            return QStringLiteral("*");
        return QString::fromStdString(user.mxid);
    case DisplayName:
        if (user.mxid == "default")
            return tr("Other users");
        return QString::fromStdString(cache::displayName(room_id, user.mxid));
    case AvatarUrl:
        return cache::avatarUrl(QString::fromStdString(room_id), QString::fromStdString(user.mxid));
    case Powerlevel:
        return static_cast<qlonglong>(user.pl);
    case IsUser:
        return !user.mxid.empty();
    case Moveable:
        return !user.mxid.empty();
    case Removeable:
        return !user.mxid.empty() && user.mxid.find('.') != std::string::npos;
    }

    return {};
}

bool
PowerlevelsUserListModel::remove(int row)
{
    if (row < 0 || row >= users.size() || users.at(row).mxid.empty())
        return false;

    beginRemoveRows(QModelIndex(), row, row);
    users.remove(row);
    endRemoveRows();

    return true;
}

void
PowerlevelsUserListModel::add(int row, QString user)
{
    if (row < 0 || row > users.size())
        return;

    const auto userStr = user.toStdString();
    for (int i = 0; i < users.size(); i++) {
        if (users[i].mxid == userStr) {
            if (i > row)
                move(i, row + 1);
            else
                move(i, row);
            return;
        }
    }

    beginInsertRows(QModelIndex(), row + 1, row + 1);
    users.insert(row + 1, Entry{user.toStdString(), users.at(row).pl});
    endInsertRows();
}

void
PowerlevelsUserListModel::addRole(int64_t role)
{
    for (int i = 0; i < users.size(); i++) {
        if (users[i].pl < role) {
            beginInsertRows(QModelIndex(), i, i);
            users.insert(i, Entry{"", role});
            endInsertRows();
            return;
        }
    }

    beginInsertRows(QModelIndex(), users.size(), users.size());
    users.push_back(Entry{"", role});
    endInsertRows();
}

bool
PowerlevelsUserListModel::move(int from, int to)
{
    if (from == to)
        return false;
    if (from < to)
        to += 1;

    beginMoveRows(QModelIndex(), from, from, QModelIndex(), to);
    auto ret = moveRow(QModelIndex(), from, QModelIndex(), to);
    endMoveRows();
    return ret;
}

bool
PowerlevelsUserListModel::moveRows(const QModelIndex &,
                                   int sourceRow,
                                   int count,
                                   const QModelIndex &,
                                   int destinationChild)
{
    if (sourceRow == destinationChild)
        return true;

    if (count != 1)
        return false;

    if (sourceRow < 0 || sourceRow >= users.size())
        return false;
    if (destinationChild < 0 || destinationChild > users.size())
        return false;

    if (users.at(sourceRow).mxid.empty())
        return false;

    auto pl         = users.at(destinationChild > 0 ? destinationChild - 1 : 0).pl;
    auto sourceItem = users.takeAt(sourceRow);
    sourceItem.pl   = pl;

    auto movedType = sourceItem.mxid;

    if (destinationChild < sourceRow)
        users.insert(destinationChild, std::move(sourceItem));
    else
        users.insert(destinationChild - 1, std::move(sourceItem));

    if (movedType == "default")
        emit defaultUserLevelChanged();

    return true;
}

PowerlevelEditingModels::PowerlevelEditingModels(QString room_id, QObject *parent)
  : QObject(parent)
  , powerLevels_(cache::client()
                   ->getStateEvent<mtx::events::state::PowerLevels>(room_id.toStdString())
                   .value_or(mtx::events::StateEvent<mtx::events::state::PowerLevels>{})
                   .content)
  , types_(room_id.toStdString(), powerLevels_, this)
  , users_(room_id.toStdString(), powerLevels_, this)
  , room_id_(room_id.toStdString())
{
    connect(&types_,
            &PowerlevelsTypeListModel::adminLevelChanged,
            this,
            &PowerlevelEditingModels::adminLevelChanged);
    connect(&types_,
            &PowerlevelsTypeListModel::moderatorLevelChanged,
            this,
            &PowerlevelEditingModels::moderatorLevelChanged);
    connect(&users_,
            &PowerlevelsUserListModel::defaultUserLevelChanged,
            this,
            &PowerlevelEditingModels::defaultUserLevelChanged);
}

void
PowerlevelEditingModels::commit()
{
    powerLevels_.events         = types_.toEvents();
    powerLevels_.kick           = types_.kick();
    powerLevels_.invite         = types_.invite();
    powerLevels_.ban            = types_.ban();
    powerLevels_.events_default = types_.eventsDefault();
    powerLevels_.state_default  = types_.stateDefault();
    powerLevels_.users          = users_.toUsers();
    powerLevels_.users_default  = users_.usersDefault();

    http::client()->send_state_event(
      room_id_, powerLevels_, [](const mtx::responses::EventId &, mtx::http::RequestErr e) {
          if (e) {
              nhlog::net()->error("Failed to send PL event: {}", *e);
              ChatPage::instance()->showNotification(
                tr("Failed to update powerlevel: %1")
                  .arg(QString::fromStdString(e->matrix_error.error)));
          }
      });
}

void
PowerlevelEditingModels::addRole(int pl)
{
    for (const auto &e : qAsConst(types_.types))
        if (pl == int(e.pl))
            return;

    types_.addRole(pl);
    users_.addRole(pl);
}
