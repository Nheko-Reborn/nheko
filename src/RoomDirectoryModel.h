// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>

#include <mtxclient/http/errors.hpp>
#include "MatrixClient.h"

#include <vector>

class RoomDirectoryModel : public QAbstractListModel
{
    Q_OBJECT

public: 
    explicit RoomDirectoryModel
    (QObject *parent = nullptr, const std::string &s = "");

    enum Roles {
        RoomName = Qt::UserRole, // does this matter? Should this be Qt::DisplayRole?
        RoomId,
        RoomAvatarUrl,
        RoomTopic,
        MemberCount,
        Previewable
    };
    QHash<int, QByteArray> roleNames() const override;

    QVariant data(const QModelIndex &index, int role) const override;

    inline int rowCount(const QModelIndex &parent = QModelIndex() [[maybe_unused]]) const override
    {
            return static_cast<int> num_fetched_;
    }

    inline bool canFetchMore(const QModelIndex &parent [[maybe_unused]]) const override
    {
        return num_fetched_ != publicRoomsData_.size();
    }
    void fetchMore(const QModelIndex &parent [[maybe_unused]]) override;

signals:
    // this tells the view we've fetched more rooms, on demand, to show the user
    void fetchedMoreRooms();

public slots:
    // this acts as a "reset" slot if we wish to change the server(s).
    void setMatrixServer(const QString &server = "");

private:
    // fields for mtxclient endpoints
    constexpr size_t limit_ = 50; // how many rooms to request from mtxclient at once
    constexpr size_t num_to_display_ = 5; // how many rooms will be displayed on the screen
    size_t num_fetched_; // how many rooms from the given [chunk] have been fetched by the view.
    std::string server_; // the server hosting the rooms

    // the current batch of public rooms provided by calls to mtxclient, 
    // and the tokens for navigating rooms (maybe unused?). 
    std::vector<mtx::responses::PublicRoomsChunk> publicRoomsData_;
    std::string prev_batch_;
    std::string next_batch_;

    // functions and/or helpers for the mtxclient endpoints
    // `GET /_matrix/client/r0/publicRooms` & `POST /_matrix/client/r0/publicRooms`
    void updateListedRooms(const std::vector &chunk);
    void getPublicRooms();
    Q_INVOKABLE void postPublicRooms(const std::string &filter_term = "", 
                         const std::string &third_party_instance_id = "");
}