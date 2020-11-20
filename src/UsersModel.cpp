#include "UsersModel.h"

#include "Cache.h"
#include "CompletionModelRoles.h"

UsersModel::UsersModel(const std::string &roomId, QObject *parent)
  : QAbstractListModel(parent)
  , room_id(roomId)
{
        roomMembers_ = cache::roomMembers(roomId);
}

QHash<int, QByteArray>
UsersModel::roleNames() const
{
        return {
          {CompletionModel::CompletionRole, "completionRole"},
          {CompletionModel::SearchRole, "searchRole"},
          {CompletionModel::SearchRole2, "searchRole2"},
          {Roles::DisplayName, "displayName"},
          {Roles::AvatarUrl, "avatarUrl"},
        };
}

QVariant
UsersModel::data(const QModelIndex &index, int role) const
{
        if (hasIndex(index.row(), index.column(), index.parent())) {
                switch (role) {
                case CompletionModel::CompletionRole:
                case CompletionModel::SearchRole:
                case Qt::DisplayRole:
                case Roles::DisplayName:
                        return QString::fromStdString(
                          cache::displayName(room_id, roomMembers_[index.row()]));
                case CompletionModel::SearchRole2:
                        return QString::fromStdString(roomMembers_[index.row()]);
                case Roles::AvatarUrl:
                        return cache::avatarUrl(QString::fromStdString(room_id),
                                                QString::fromStdString(roomMembers_[index.row()]));
                }
        }
        return {};
}
