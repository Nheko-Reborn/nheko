// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include "ui/FlatButton.h"

#include <mtxclient/http/errors.hpp>
#include "MatrixClient.h"

#include <vector>

class RoomDirectoryModel : public QAbstractListModel
{
public: 
    explicit RoomDirectoryModel
    (const std::string &s = "", QObject *parent = nullptr);

    enum Roles {
        // ??? TODO ???
    };
    QHash<int, QByteArray> roleNames() const override;

    QVariant data(const QModelIndex &index, int role) const override;

    inline int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
            (void)parent; // ???copied from UserModel - (why) do we need this???
            return static_cast<int>(publicRoomsData_.size());
    }

    // the response's chunk field is a paginated array of PublicRoomsChunk data
    // we only want to be able to view a few of these (eg. 5 rooms) at a time.
    // Scrolling/Navigating to the next page should fetch the next set of rooms.
    inline bool canFetchMore(const QModelIndex &parent) const override
    {
        return !next_batch_.empty(); // see Client-Server spec
    }
    void fetchMore(const QModelIndex &parent) override;

private:
    // fields for mtxclient endpoints
    std::string server_; // which server to show rooms of
    constexpr size_t limit_ = 5; // how many rooms to show at once
    std::string  
    // ???some kind of member function where we call the mtxclient backend. 
    // This is if fetchMore should do something more than calling the mtxclient
    // endpoint???

    // the current batch of public rooms fetched by mtxclient, and the tokens for navigating. 
    std::vector<mtx::responses::PublicRoomsChunk> publicRoomsData_;
    std::string prev_batch_;
    std::string next_batch_;

    // ???fields to display the data for the UI???
    std::vector<QString> displayRoomNames_; // room name
    std::vector<std::pair<QString, QString>> displayRoomInfo_; // room id and/or room description
    std::vector<QString> displayRoomCount_; // # members in the room
    // ???Join/Preview button - or does this go into QML???
    std::vector<FlatButton *> joinButtons_;
    std::vector<FlatButton *> previewButtons_;
}