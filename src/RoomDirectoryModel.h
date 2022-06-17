// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include <QString>
#include <string>
#include <vector>

#include <mtx/responses/public_rooms.hpp>

class RoomDirectoryModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(bool loadingMoreRooms READ loadingMoreRooms NOTIFY loadingMoreRoomsChanged)
    Q_PROPERTY(
      bool reachedEndOfPagination READ reachedEndOfPagination NOTIFY reachedEndOfPaginationChanged)

public:
    explicit RoomDirectoryModel(QObject *parent = nullptr, const std::string &server = "");

    enum Roles
    {
        Name = Qt::UserRole,
        Id,
        AvatarUrl,
        Topic,
        MemberCount,
        Previewable,
        CanJoin,
    };
    QHash<int, QByteArray> roleNames() const override;

    QVariant data(const QModelIndex &index, int role) const override;

    inline int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        (void)parent;
        return static_cast<int>(publicRoomsData_.size());
    }

    bool canFetchMore(const QModelIndex &) const override { return canFetchMore_; }

    bool loadingMoreRooms() const { return loadingMoreRooms_; }

    bool reachedEndOfPagination() const { return reachedEndOfPagination_; }

    void fetchMore(const QModelIndex &) override;

    Q_INVOKABLE void joinRoom(const int &index = -1);

signals:
    void fetchedRoomsBatch(std::vector<mtx::responses::PublicRoomsChunk> rooms,
                           const std::string &next_batch);
    void loadingMoreRoomsChanged();
    void reachedEndOfPaginationChanged();

public slots:
    void setMatrixServer(const QString &s = QLatin1String(""));
    void setSearchTerm(const QString &f);

private slots:

    void displayRooms(std::vector<mtx::responses::PublicRoomsChunk> rooms,
                      const std::string &next_batch);

private:
    bool canJoinRoom(const QString &room) const;

    static constexpr size_t limit_ = 50;

    std::string server_;
    std::string userSearchString_;
    std::string prevBatch_;
    std::string nextBatch_;
    bool canFetchMore_{true};
    bool loadingMoreRooms_{false};
    bool reachedEndOfPagination_{false};
    std::vector<mtx::responses::PublicRoomsChunk> publicRoomsData_;

    std::vector<std::string> getViasForRoom(const std::vector<std::string> &room);
    void resetDisplayedData();
};
