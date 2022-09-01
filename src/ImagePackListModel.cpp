// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ImagePackListModel.h"

#include <QQmlEngine>

#include "Cache_p.h"
#include "SingleImagePackModel.h"

ImagePackListModel::ImagePackListModel(const std::string &roomId, QObject *parent)
  : QAbstractListModel(parent)
  , room_id(roomId)
{
    auto packs_ = cache::client()->getImagePacks(room_id, std::nullopt);

    packs.reserve(packs_.size());
    for (const auto &pack : packs_) {
        packs.push_back(QSharedPointer<SingleImagePackModel>(new SingleImagePackModel(pack)));
    }
}

int
ImagePackListModel::rowCount(const QModelIndex &) const
{
    return (int)packs.size();
}

QHash<int, QByteArray>
ImagePackListModel::roleNames() const
{
    return {
      {Roles::DisplayName, "displayName"},
      {Roles::AvatarUrl, "avatarUrl"},
      {Roles::FromAccountData, "fromAccountData"},
      {Roles::FromCurrentRoom, "fromCurrentRoom"},
      {Roles::FromSpace, "fromSpace"},
      {Roles::StateKey, "statekey"},
      {Roles::RoomId, "roomid"},
    };
}

QVariant
ImagePackListModel::data(const QModelIndex &index, int role) const
{
    if (hasIndex(index.row(), index.column(), index.parent())) {
        const auto &pack = packs.at(index.row());
        switch (role) {
        case Roles::DisplayName:
            return pack->packname();
        case Roles::AvatarUrl:
            return pack->avatarUrl();
        case Roles::FromAccountData:
            return pack->roomid().isEmpty();
        case Roles::FromCurrentRoom:
            return pack->roomid().toStdString() == this->room_id;
        case Roles::FromSpace:
            return pack->fromSpace();
        case Roles::StateKey:
            return pack->statekey();
        case Roles::RoomId:
            return pack->roomid();
        default:
            return {};
        }
    }
    return {};
}

SingleImagePackModel *
ImagePackListModel::packAt(int row)
{
    if (row < 0 || static_cast<size_t>(row) >= packs.size())
        return {};
    auto e = packs.at(row).get();
    QQmlEngine::setObjectOwnership(e, QQmlEngine::CppOwnership);
    return e;
}

SingleImagePackModel *
ImagePackListModel::newPack(bool inRoom)
{
    ImagePackInfo info{};
    if (inRoom)
        info.source_room = room_id;
    return new SingleImagePackModel(info);
}

bool
ImagePackListModel::containsAccountPack() const
{
    for (const auto &p : packs)
        if (p->roomid().isEmpty())
            return true;
    return false;
}
