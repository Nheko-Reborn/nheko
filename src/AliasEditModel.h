// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include <QVector>

#include <mtx/events/canonical_alias.hpp>

#include "CacheStructs.h"

class FetchPublishedAliasesJob : public QObject
{
    Q_OBJECT

public:
    explicit FetchPublishedAliasesJob(QObject *p = nullptr)
      : QObject(p)
    {}

signals:
    void aliasFetched(std::string alias, std::string target);
    void advertizedAliasesFetched(std::vector<std::string> aliases);
};

class AliasEditingModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(bool canAdvertize READ canAdvertize CONSTANT)

public:
    enum Roles
    {
        Name,
        IsPublished,
        IsCanonical,
        IsAdvertized,
    };

    explicit AliasEditingModel(const std::string &room_id_, QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &) const override { return static_cast<int>(aliases.size()); }
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    bool canAdvertize() const { return canSendStateEvent; }

    Q_INVOKABLE bool deleteAlias(int row);
    Q_INVOKABLE void addAlias(QString newAlias);
    Q_INVOKABLE void makeCanonical(int row);
    Q_INVOKABLE void togglePublish(int row);
    Q_INVOKABLE void toggleAdvertize(int row);
    Q_INVOKABLE void commit();

private slots:
    void updateAlias(std::string alias, std::string target);
    void updatePublishedAliases(std::vector<std::string> aliases);

private:
    void fetchAliasesStatus(const std::string &alias);
    void fetchPublishedAliases();

    struct Entry
    {
        ~Entry() = default;

        std::string alias;
        bool canonical  = false;
        bool advertized = false;
        bool published  = false;
    };

    std::string room_id;
    QVector<Entry> aliases;
    mtx::events::state::CanonicalAlias aliasEvent;
    bool canSendStateEvent = false;
};
