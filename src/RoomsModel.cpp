#include "RoomsModel.h"

#include "Cache_p.h"
#include "CompletionModelRoles.h"

RoomsModel::RoomsModel(QObject *parent)
  : QAbstractListModel(parent)
{
        rooms_    = cache::joinedRooms();
        roomInfos = cache::getRoomInfo(rooms_);

        for (const auto &r : rooms_) {
                auto roomAliasesList = cache::client()->getRoomAliases(r);

                if (roomAliasesList) {
                        roomAliases.push_back(QString::fromStdString(roomAliasesList->alias));
                        roomids.push_back(QString::fromStdString(r));
                }
        }
}

QHash<int, QByteArray>
RoomsModel::roleNames() const
{
        return {{CompletionModel::CompletionRole, "completionRole"},
                {CompletionModel::SearchRole, "searchRole"},
                {CompletionModel::SearchRole2, "searchRole2"},
                {Roles::RoomAlias, "roomAlias"},
                {Roles::AvatarUrl, "avatarUrl"},
                {Roles::RoomID, "roomid"},
                {Roles::RoomName, "roomName"}};
}

QVariant
RoomsModel::data(const QModelIndex &index, int role) const
{
        if (hasIndex(index.row(), index.column(), index.parent())) {
                switch (role) {
                case CompletionModel::CompletionRole:
                        return QString("[%1](https://matrix.to/%1)").arg(roomAliases[index.row()]);
                case CompletionModel::SearchRole:
                case Qt::DisplayRole:
                case Roles::RoomAlias:
                        return roomAliases[index.row()];
                case CompletionModel::SearchRole2:
                        return roomAliases[index.row()];
                case Roles::AvatarUrl:
                        return QString::fromStdString(
                          roomInfos.at(roomids[index.row()]).avatar_url);
                case Roles::RoomID:
                        return roomids[index.row()];
                case Roles::RoomName:
                        return QString::fromStdString(roomInfos.at(roomids[index.row()]).name);
                }
        }
        return {};
}
