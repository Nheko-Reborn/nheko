// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include <QSortFilterProxyModel>

#include <mtx/events/power_levels.hpp>

#include "CacheStructs.h"

class PowerlevelsTypeListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles
    {
        DisplayName,
        Powerlevel,
        IsType,
        Moveable,
        Removeable,
    };

    explicit PowerlevelsTypeListModel(const std::string &room_id_,
                                      const mtx::events::state::PowerLevels &pl,
                                      QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &) const override { return static_cast<int>(types.size()); }
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    Q_INVOKABLE bool remove(int row);
    Q_INVOKABLE bool move(int from, int to);
    Q_INVOKABLE void add(int index, QString type);

    bool moveRows(const QModelIndex &sourceParent,
                  int sourceRow,
                  int count,
                  const QModelIndex &destinationParent,
                  int destinationChild) override;

    std::map<std::string, mtx::events::state::power_level_t, std::less<>> toEvents();
    mtx::events::state::power_level_t kick();
    mtx::events::state::power_level_t invite();
    mtx::events::state::power_level_t ban();
    mtx::events::state::power_level_t eventsDefault();
    mtx::events::state::power_level_t stateDefault();

private:
    struct Entry
    {
        std::string type;
        mtx::events::state::power_level_t pl;
    };

    std::string room_id;
    QVector<Entry> types;
    mtx::events::state::PowerLevels powerLevels_;
};

class PowerlevelsUserListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles
    {
        Mxid,
        DisplayName,
        AvatarUrl,
        Powerlevel,
        IsUser,
        Moveable,
        Removeable,
    };

    explicit PowerlevelsUserListModel(const std::string &room_id_,
                                      const mtx::events::state::PowerLevels &pl,
                                      QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &) const override { return static_cast<int>(users.size()); }
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    Q_INVOKABLE bool remove(int row);
    Q_INVOKABLE bool move(int from, int to);
    Q_INVOKABLE void add(int index, QString user);

    bool moveRows(const QModelIndex &sourceParent,
                  int sourceRow,
                  int count,
                  const QModelIndex &destinationParent,
                  int destinationChild) override;

    std::map<std::string, mtx::events::state::power_level_t, std::less<>> toUsers();
    mtx::events::state::power_level_t usersDefault();

private:
    struct Entry
    {
        std::string mxid;
        mtx::events::state::power_level_t pl;
    };

    std::string room_id;
    QVector<Entry> users;
    mtx::events::state::PowerLevels powerLevels_;
};

class PowerlevelEditingModels : public QObject
{
    Q_OBJECT

    Q_PROPERTY(PowerlevelsUserListModel *users READ users CONSTANT)
    Q_PROPERTY(PowerlevelsTypeListModel *types READ types CONSTANT)
    Q_PROPERTY(qlonglong adminLevel READ adminLevel CONSTANT)
    Q_PROPERTY(qlonglong moderatorLevel READ moderatorLevel CONSTANT)

public:
    explicit PowerlevelEditingModels(QString room_id, QObject *parent = nullptr);

    PowerlevelsUserListModel *users() { return &users_; }
    PowerlevelsTypeListModel *types() { return &types_; }
    qlonglong adminLevel() const
    {
        return powerLevels_.state_level(to_string(mtx::events::EventType::RoomPowerLevels));
    }
    qlonglong moderatorLevel() const { return powerLevels_.redact; }

    Q_INVOKABLE void commit();

    mtx::events::state::PowerLevels powerLevels_;
    PowerlevelsTypeListModel types_;
    PowerlevelsUserListModel users_;
    std::string room_id_;
};
