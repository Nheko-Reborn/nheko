// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "RoomDirectoryModel.h"
// #include "CompletionModelRoles.h"

RoomDirectoryModel::RoomDirectoryModel(QObject *parent, const std::string &s)
                    : QAbstractListModel(parent)
                    , server_(s), num_fetched_(0)
{
    // setMatrixServer(s);
}

QHash<int, QByteArray> 
RoomDirectoryModel::roleNames() const
{
    return {
        {Roles::RoomName, "name"},
        {Roles::RoomId, "roomid"},
        {Roles::RoomAvatarUrl, "avatarUrl"},
        {Roles::RoomTopic, "topic"},
        {Roles::MemberCount, "numMembers"},
        {Roles::Previewable, "canPreview"}
    };
}

// void
// RoomDirectoryModel::setMatrixServer(const QString &server)
// {
//     beginResetModel();
//     server_ = server.toStdString();
//     // show a chunk of rooms for that server
//     // getPublicRooms();
//     endResetModel();
// }

// bool
// insertRows(int row, int count, const QModelIndex &parent)
// {
    

//     // 
//     num_fetched_ += num_to_display_;

//     endInsertRows();
// }

// void
// RoomDirectoryModel::fetchMore(const QModelIndex & parent) 
// {
//     if () {
//         const bool success = insertRows(, static_cast<int>(num_to_display_), parent);
//         if (!success) {
//             // is simply logging the failure enough?
//             nhlog::net()->warn("Failed to provide public rooms to QML view from Room Directory Model");
//         } else {
//             // I'm still a bit dubious how this actually loads the
//             // underlying data stored in publicRoomsData_.
//             // for this to work, beginInsertRows need to track the last emitted room index,
//             // i.e. parent needs to be the num_fetched th element of publicRoomsData_.
//             beginInsertRows(parent, num_fetched_, num_fetched_ + num_to_display_ - 1);
            
//             num_fetched_ += num_to_display_;
            
//             endInsertRows();

//             emit fetchedMoreRooms();
//         }
//     }
// }

// QVariant
// RoomDirectoryModel::data(const QModelIndex &index, int role) const
// {
//     if (hasIndex(index.row(), index.column(), index.parent())) 
//     {
//         const auto &room_chunk = publicRoomsData_[index.row()]; 
//         switch (role)
//         {
//             case Roles::RoomName:
//                 return QString::fromStdString(room_chunk.name); 
//             case Roles::RoomId:
//                 return QString::fromStdString(room_chunk.room_id);
//             case Roles::RoomAvatarUrl:
//                 return QString::fromStdString(room_chunk.avatar_url);
//             case Roles::RoomTopic:
//                 return QString::fromStdString(room_chunk.topic);
//             case Roles::MemberCount:
//                 return QVariant::fromValue(room_chunk.num_joined_members);
//             case Roles::Previewable:
//                 return QVariant::fromValue(room_chunk.world_readable);
//         }
//     }
//     return {};
// }

QVariant
RoomDirectoryModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= 0 && static_cast<size_t>(index.row()) < dummyData_.size()) 
    {
        const auto &room_chunk = dummyData_[index.row()]; 
        switch (role)
        {
            case Roles::RoomName: 
            case Roles::RoomId:
            case Roles::RoomAvatarUrl:
            case Roles::RoomTopic:
            case Roles::MemberCount:
            case Roles::Previewable:
                return QString::fromStdString(room_chunk);
        }
    }
    return {};
}

// void
// RoomDirectoryModel::updateListedRooms(const std::vector &chunk) 
// {
//     for (const auto &room: chunk) {
//         publicRoomsData_.push_back(room);
//     }
//     prev_batch_ = res.prev_batch;
//     next_batch = res.next_batch;
// }

// void
// RoomDirectoryModel::getPublicRooms() 
// {
//     http::client()->get_public_rooms(
//                             [this](
//                                 const mtx::responses::PublicRooms &res,
//                                 RequestErr err)
//                                 {
//                                     if (err) {
//                                         nhlog::net()->error("Failed to GET public rooms batch - {} - {} - {}",
//                                         mtx::errors::to_string(err->matrix_error.errcode),
//                                         err->matrix_error.error,
//                                         err->parse_error);
//                                     } else {
//                                         updateListedRooms(res.chunk);
//                                     }
//                                 },
//                             server_,
//                             limit_,
//                             next_batch_,);
// }

// void
// RoomDirectoryModel::postPublicRooms(const std::string &filter_term, const std::string &third_party_instance_id) 
// {
//     mtx::requests::PublicRooms req;
//     req.limit = limit_;
//     req.since = prev_batch_;
//     req.filter.generic_search_term = filter_term;
//     req.third_party_instance_id = third_party_instance_id;

//     http::client()->post_public_rooms(req, [this, req](
//                                             const mtx::responses::PublicRooms &res, 
//                                             RequestErr err) 
//                                             {
//                                                 if (err) {
//                                                     nhlog::net()->error("Failed to POST public rooms batch - {} - {} - {}",
//                                                     mtx::errors::to_string(err->matrix_error.errcode),
//                                                     err->matrix_error.error,
//                                                     err->parse_error);
//                                                 } else {
//                                                     updateListedRooms(res.chunk); 
//                                                 }
//                                             }, server_);
// }