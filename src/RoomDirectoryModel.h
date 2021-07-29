// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QString>
#include <vector>
#include <string>

#include "MatrixClient.h"
#include <mtxclient/http/errors.hpp>
#include <mtx/responses/public_rooms.hpp>

#include "Logging.h"

namespace mtx::http {
using RequestErr = const std::optional<mtx::http::ClientError> &;
}
namespace mtx::responses {
struct PublicRooms;
}

class RoomDirectoryModel : public QAbstractListModel
{
    Q_OBJECT

public: 
    explicit RoomDirectoryModel
    (QObject *parent = nullptr, const std::string &s = "");

    enum Roles {
        Name = Qt::UserRole,
        Id,
        AvatarUrl,
        Topic,
        MemberCount,
        Previewable
    };
    QHash<int, QByteArray> roleNames() const override;

    QVariant data(const QModelIndex &index, int role) const override;

    inline int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
            (void) parent;
            return static_cast<int> (publicRoomsData_.size());
    }

    inline bool canFetchMore(const QModelIndex &) const override
    {
        nhlog::net()->debug("determining if can fetch more");
        return canFetchMore_;
    }
    void fetchMore(const QModelIndex &) override;

    Q_INVOKABLE void joinRoom(const int &index = -1);

signals:
    void fetchedRoomsBatch(std::vector<mtx::responses::PublicRoomsChunk> rooms, 
                                        const std::string &prev_batch, const std::string &next_batch);
    void serverChanged();
    void searchTermEntered();

public slots:
    void displayRooms(std::vector<mtx::responses::PublicRoomsChunk> rooms,
                      const std::string &prev, const std::string &next);
    void setMatrixServer(const QString &s = "");
    void setSearchTerm(const QString &f);

private:
    static constexpr size_t limit_ = 50;
    
    std::string server_;
    std::string userSearchString_;
    std::string prevBatch_;
    std::string nextBatch_;
    bool canFetchMore_;
    std::vector<mtx::responses::PublicRoomsChunk> publicRoomsData_;

    std::vector<std::string> getViasForRoom(const std::vector<std::string> &room);
    void resetDisplayedData();
};