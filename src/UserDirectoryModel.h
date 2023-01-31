// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-FileCopyrightText: 2023 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include <QString>
#include <string>
#include <vector>

#include <mtx/responses/users.hpp>

class FetchUsersFromDirectoryJob final : public QObject
{
    Q_OBJECT
public:
    explicit FetchUsersFromDirectoryJob(QObject *p = nullptr)
      : QObject(p)
    {
    }
signals:
    void
    fetchedSearchResults(std::vector<mtx::responses::User> results, const std::string &searchTerm);
};
class UserDirectoryModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(bool searchingUsers READ searchingUsers NOTIFY searchingUsersChanged)

public:
    explicit UserDirectoryModel(QObject *parent = nullptr);

    enum Roles
    {
        DisplayName,
        Mxid,
        AvatarUrl,
    };
    QHash<int, QByteArray> roleNames() const override;

    QVariant data(const QModelIndex &index, int role) const override;

    inline int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        (void)parent;
        return static_cast<int>(results_.size());
    }
    bool canFetchMore(const QModelIndex &) const override { return canFetchMore_; }
    void fetchMore(const QModelIndex &) override;

private:
    std::vector<mtx::responses::User> results_;
    std::string userSearchString_;
    bool searchingUsers_{false};
    bool canFetchMore_{false};

signals:
    void searchingUsersChanged();

public slots:
    void setSearchString(const QString &f);
    bool searchingUsers() const { return searchingUsers_; }

private slots:
    void
    displaySearchResults(std::vector<mtx::responses::User> results, const std::string &searchTerm);
};
