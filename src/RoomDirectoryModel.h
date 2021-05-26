// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QString>

#include "MatrixClient.h"
#include <mtxclient/http/errors.hpp>
#include <mtx/responses/public_rooms.hpp>

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

    inline int rowCount([[maybe_unused]] const QModelIndex &parent = QModelIndex()) const override
    {
            return static_cast<int> (dummyData_.size());
    }

    // inline bool canFetchMore([[maybe_unused]] const QModelIndex &parent) const override
    // {
    //     return num_fetched_ != publicRoomsData_.size();
    // }
    // void fetchMore([[maybe_unused]] const QModelIndex &parent) override;

signals:
    // this tells the view we've fetched more rooms, on demand, to show the user
    // void fetchedMoreRooms();

public slots:
    // void setMatrixServer(const QString &server = "");
    // void clear();

private:
    using PublicRoomsChunk = mtx::responses::PublicRoomsChunk;
    // fields for mtxclient endpoints
    static constexpr size_t limit_ = 50; // how many rooms to request from mtxclient at once
    // constexpr size_t num_to_display_ = 5; // how many rooms will be displayed on the screen
    std::string server_; // the server hosting the rooms
    size_t num_fetched_; // how many rooms from the given [chunk] have been fetched by the view.

    // the current batch of public rooms provided by calls to mtxclient, 
    // and the tokens for navigating rooms (maybe unused?). 
    std::vector<std::string> dummyData_ = {"Element1", "Eleme1", "EleMent4", "NewElement1", "Element77"};
    std::vector<PublicRoomsChunk> publicRoomsData_;
    std::string prev_batch_;
    std::string next_batch_;
    std::string filter_;

    // functions and/or helpers for the mtxclient endpoints
    // `GET /_matrix/client/r0/publicRooms` & `POST /_matrix/client/r0/publicRooms`
    // void updateListedRooms(const std::vector &chunk);
    // void getPublicRooms();
    // Q_INVOKABLE void postPublicRooms(const std::string &filter_term = "", 
    //                      const std::string &third_party_instance_id = "");
};