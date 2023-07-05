// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include <QQmlEngine>
#include <QSortFilterProxyModel>

#include <mtx/events/power_levels.hpp>

#include "CacheStructs.h"

class PowerlevelsTypeListModel final : public QAbstractListModel
{
    Q_OBJECT

signals:
    void adminLevelChanged();
    void moderatorLevelChanged();

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
    void addRole(int64_t role);

    bool moveRows(const QModelIndex &sourceParent,
                  int sourceRow,
                  int count,
                  const QModelIndex &destinationParent,
                  int destinationChild) override;

    std::map<std::string, mtx::events::state::power_level_t, std::less<>> toEvents() const;
    mtx::events::state::power_level_t kick() const;
    mtx::events::state::power_level_t invite() const;
    mtx::events::state::power_level_t ban() const;
    mtx::events::state::power_level_t eventsDefault() const;
    mtx::events::state::power_level_t stateDefault() const;

    struct Entry
    {
        ~Entry() = default;

        std::string type;
        mtx::events::state::power_level_t pl;
    };

    std::string room_id;
    QVector<Entry> types;
    mtx::events::state::PowerLevels powerLevels_;
};

class PowerlevelsUserListModel final : public QAbstractListModel
{
    Q_OBJECT

signals:
    void defaultUserLevelChanged();

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
    void addRole(int64_t role);

    bool moveRows(const QModelIndex &sourceParent,
                  int sourceRow,
                  int count,
                  const QModelIndex &destinationParent,
                  int destinationChild) override;

    std::map<std::string, mtx::events::state::power_level_t, std::less<>> toUsers() const;
    mtx::events::state::power_level_t usersDefault() const;

    struct Entry
    {
        ~Entry() = default;

        std::string mxid;
        mtx::events::state::power_level_t pl;
    };

    std::string room_id;
    QVector<Entry> users;
    mtx::events::state::PowerLevels powerLevels_;
};

class PowerlevelsSpacesListModel final : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(bool applyToChildren READ applyToChildren WRITE setApplyToChildren NOTIFY
                 applyToChildrenChanged)
    Q_PROPERTY(bool overwriteDiverged READ overwriteDiverged WRITE setOverwriteDiverged NOTIFY
                 overwriteDivergedChanged)

signals:
    void applyToChildrenChanged();
    void overwriteDivergedChanged();

public:
    enum Roles
    {
        DisplayName,
        AvatarUrl,
        IsSpace,
        IsEditable,
        IsDifferentFromBase,
        IsAlreadyUpToDate,
        ApplyPermissions,
    };

    explicit PowerlevelsSpacesListModel(const std::string &room_id_,
                                        const mtx::events::state::PowerLevels &pl,
                                        QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &) const override { return static_cast<int>(spaces.size()); }
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool
    setData(const QModelIndex &index, const QVariant &value, int role = Qt::DisplayRole) override;

    bool applyToChildren() const { return applyToChildren_; }
    bool overwriteDiverged() const { return overwriteDiverged_; }

    void setApplyToChildren(bool val)
    {
        applyToChildren_ = val;
        emit applyToChildrenChanged();
        updateToDefaults();
    }
    void setOverwriteDiverged(bool val)
    {
        overwriteDiverged_ = val;
        emit overwriteDivergedChanged();
        updateToDefaults();
    }

    void updateToDefaults();

    Q_INVOKABLE void commit();

    struct Entry
    {
        ~Entry() = default;

        std::string roomid;
        mtx::events::state::PowerLevels pl;
        bool apply = false;
    };

    std::string room_id;
    QVector<Entry> spaces;
    mtx::events::state::PowerLevels oldPowerLevels_, newPowerlevels_;

    bool applyToChildren_ = true, overwriteDiverged_ = false;
};

class PowerlevelEditingModels final : public QObject
{
    Q_OBJECT

    QML_ELEMENT
    QML_UNCREATABLE("Please use editPowerlevels to create the models")

    Q_PROPERTY(PowerlevelsUserListModel *users READ users CONSTANT)
    Q_PROPERTY(PowerlevelsTypeListModel *types READ types CONSTANT)
    Q_PROPERTY(PowerlevelsSpacesListModel *spaces READ spaces CONSTANT)
    Q_PROPERTY(qlonglong adminLevel READ adminLevel NOTIFY adminLevelChanged)
    Q_PROPERTY(qlonglong moderatorLevel READ moderatorLevel NOTIFY moderatorLevelChanged)
    Q_PROPERTY(qlonglong defaultUserLevel READ defaultUserLevel NOTIFY defaultUserLevelChanged)
    Q_PROPERTY(bool isSpace READ isSpace CONSTANT)

signals:
    void adminLevelChanged();
    void moderatorLevelChanged();
    void defaultUserLevelChanged();

private:
    mtx::events::state::PowerLevels calculateNewPowerlevel() const;

public:
    explicit PowerlevelEditingModels(QString room_id, QObject *parent = nullptr);

    PowerlevelsUserListModel *users() { return &users_; }
    PowerlevelsTypeListModel *types() { return &types_; }
    PowerlevelsSpacesListModel *spaces() { return &spaces_; }
    qlonglong adminLevel() const
    {
        return powerLevels_.state_level(to_string(mtx::events::EventType::RoomPowerLevels));
    }
    qlonglong moderatorLevel() const { return powerLevels_.redact; }
    qlonglong defaultUserLevel() const { return powerLevels_.users_default; }
    bool isSpace() const;

    Q_INVOKABLE void commit();
    Q_INVOKABLE void updateSpacesModel();
    Q_INVOKABLE void addRole(int pl);

    mtx::events::state::PowerLevels powerLevels_;
    PowerlevelsTypeListModel types_;
    PowerlevelsUserListModel users_;
    PowerlevelsSpacesListModel spaces_;
    std::string room_id_;
};
