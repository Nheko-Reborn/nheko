// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "RoomDirectoryModel.h"
#include "ChatPage.h"

#include <algorithm>

RoomDirectoryModel::RoomDirectoryModel(QObject *parent, const std::string &s)
                    : QAbstractListModel(parent)
                    , server_(s), userSearchString_(""), prevBatch_(""), nextBatch_(""), canFetchMore_(true)
{
    connect(this, &RoomDirectoryModel::fetchedRoomsBatchFromMtxClient, this, &RoomDirectoryModel::displayRooms, Qt::QueuedConnection);    
}

QHash<int, QByteArray> 
RoomDirectoryModel::roleNames() const
{
    return {
        {Roles::Name, "name"},
        {Roles::Id, "roomid"},
        {Roles::AvatarUrl, "avatarUrl"},
        {Roles::Topic, "topic"},
        {Roles::MemberCount, "numMembers"},
        {Roles::Previewable, "canPreview"}
    };
}

void
RoomDirectoryModel::resetDisplayedData()
{
    prevBatch_ = "";
    nextBatch_ = "";
    canFetchMore_ = true;

    beginRemoveRows(QModelIndex(), 0 , static_cast<int> (publicRoomsData_.size()));
    publicRoomsData_.clear();
    endRemoveRows();
}

void
RoomDirectoryModel::setMatrixServer(const QString &s)
{
    beginResetModel();

    server_ = s.toStdString();

    nhlog::ui()->debug("Received matrix server: {}", server_);

    resetDisplayedData();

    endResetModel();
}

void
RoomDirectoryModel::setSearchTerm(const QString &f)
{
    beginResetModel();

    userSearchString_ = f.toStdString();

    nhlog::ui()->debug("Received user query: {}", userSearchString_);

    resetDisplayedData();
 
    endResetModel();
}

std::vector<std::string>
RoomDirectoryModel::getViasForRoom(const std::vector<std::string> &aliases)
{
    std::vector<std::string> vias;
    
    vias.reserve(aliases.size());

    std::transform(aliases.begin(), aliases.end(), 
                   std::back_inserter(vias), [](const auto &alias) {
                        const auto roomAliasDelimiter = ":";
                        return alias.substr(alias.find(roomAliasDelimiter) + 1);
                   });

    return vias;
}

void
RoomDirectoryModel::joinRoom(const int &index)
{
    constexpr int delegateNoLongerValid = -1;
    if (index == delegateNoLongerValid) return;

    const auto &chunk = publicRoomsData_[index];
    nhlog::ui()->debug("'Joining room {}", chunk.room_id);
    ChatPage::instance()->joinRoomVia(chunk.room_id, getViasForRoom(chunk.aliases));
}

QVariant
RoomDirectoryModel::data(const QModelIndex &index, int role) const
{
    if (hasIndex(index.row(), index.column(), index.parent())) 
    {
        const auto &room_chunk = publicRoomsData_[index.row()]; 
        switch (role)
        {
            case Roles::Name:
                return QString::fromStdString(room_chunk.name); 
            case Roles::Id:
                return QString::fromStdString(room_chunk.room_id);
            case Roles::AvatarUrl:
                return QString::fromStdString(room_chunk.avatar_url);
            case Roles::Topic:
                return QString::fromStdString(room_chunk.topic);
            case Roles::MemberCount:
                return QVariant::fromValue(room_chunk.num_joined_members);
            case Roles::Previewable:
                return QVariant::fromValue(room_chunk.world_readable);
        }
    }
    return {};
}

void
RoomDirectoryModel::fetchMore(const QModelIndex &) 
{
    nhlog::net()->debug("Fetching more rooms from mtxclient...");
    nhlog::net()->debug("Prev batch: {} | Next batch: {}", prevBatch_, nextBatch_);

    mtx::requests::PublicRooms req;
    req.limit = limit_;
    req.since = prevBatch_;
    req.filter.generic_search_term = userSearchString_;
    // req.third_party_instance_id = third_party_instance_id;

    http::client()->post_public_rooms(req, [this, req]
                                            (const mtx::responses::PublicRooms &res, 
                                            mtx::http::RequestErr err) 
                                            {
                                                if (err) {
                                                    nhlog::net()->error
                                                    ("Failed to retrieve rooms from mtxclient - {} - {} - {}",
                                                    mtx::errors::to_string(err->matrix_error.errcode),
                                                    err->matrix_error.error,
                                                    err->parse_error);
                                                } else {
                                                    nhlog::net()->debug("signalling chunk to GUI thread");
                                                    emit fetchedRoomsBatchFromMtxClient(res.chunk, res.prev_batch, res.next_batch/*, res.total_room_count_estimate*/); 
                                                }
                                            }, server_);
}

void
RoomDirectoryModel::displayRooms(std::vector<mtx::responses::PublicRoomsChunk> fetched_rooms,
                                 const std::string &prev_batch, const std::string &next_batch
                                 /*, std::optional<size_t> total_room_count_estimate*/)
{
    nhlog::net()->debug("Prev batch: {} | Next batch: {}", prevBatch_, nextBatch_);
    nhlog::net()->debug("NP batch: {} | NN batch: {}", prev_batch, next_batch);

    if (fetched_rooms.empty()) {
        nhlog::net()->error("mtxclient helper thread yielded empty chunk!");
        return;
    }

    beginInsertRows(QModelIndex(), static_cast<int> (publicRoomsData_.size()), static_cast<int> (publicRoomsData_.size() + fetched_rooms.size()) - 1);
    this->publicRoomsData_.insert(this->publicRoomsData_.end(), fetched_rooms.begin(), fetched_rooms.end());
    endInsertRows();

    if (next_batch.empty()) {
        canFetchMore_ = false;
    }
    nhlog::ui()->debug(publicRoomsData_.size());
    prevBatch_ = std::exchange(nextBatch_, next_batch);
}