// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "RoomDirectoryModel.h"
#include "CompletionModelRoles.h"

RoomDirectoryModel::RoomDirectoryModel(std::string &s, QObject *parent)
                    : QAbstractListModel(parent)
                    , server_(s)
{
    // ??? don't need to do anything special here ???
}

QHash<int, QByteArray> 
RoomDirectoryModel::roleNames() const
{
    // ??? TODO once I understand what the roles actually mean
    return {};
}

QVariant
RoomDirectoryModel::data(const QModelIndex &index, int role) const
{
    if (hasIndex(index.row(), index.column(), index.parent())) {
        switch (role)
        {
        // TODO once I figure out what roles are involved

        // case /* constant-expression */:
        //     /* code */
        //     break;
        
        // default:
        //     break;
        }
    } else {
        return {};
    }
}

void
RoomDirectoryModel::fetchMore(const QModelIndex &parent) {
    // call the GET /_matrix/client/r0/publicRooms endpoint
    // from mtxclient which returns 
    http::client()->get_public_rooms(
                            // Callback<mtx::responses::PublicRooms> cb,
                            [this](
                                const mtx::responses::PublicRooms &res,
                                RequestErr err)
                                {
                                    if (err) {
                                        nhlog::net()->error("Failed to GET public rooms batch - {} - {} - {}",
                                        mtx::errors::to_string(err->matrix_error.errcode),
                                        err->matrix_error.error,
                                        err->parse_error);
                                    } else {
                                        // ??? should we keep only the latest chunk, or should out vector
                                        // of rooms store all rooms from the very first chunk batch???  
                                        publicRoomsData_ = res.chunk; // ???copying seems like the right thing to do over moving???
                                        // ??? do we split the chunk fields and populate the displayRoomNames_ etc member fields accordingly ???
                                        for (const auto &room : res.chunk) {
                                            displayRoomNames_.push_back(room.name);
                                            displayRoomInfo_.push_back(std::make_pair(room.room_id, room.topic));
                                            displayRoomCount_.push_back(QString::number(room.num_joined_members));
                                        }
                                        prev_batch_ = res.prev_batch;
                                        next_batch = res.next_batch;
                                    }
                                }
                            server_,
                            limit_,
                            next_batch_,);
}