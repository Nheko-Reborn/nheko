#include "UsersModel.h"

#include "Cache.h"
#include "CompletionModelRoles.h"

#include <QPixmap>

UsersModel::UsersModel(const std::string &roomId, QObject *parent)
  : QAbstractListModel(parent)
{
        roomMembers_ = cache::getMembers(roomId, 0, 9999);
}

QVariant
UsersModel::data(const QModelIndex &index, int role) const
{
        if (hasIndex(index.row(), index.column(), index.parent())) {
                switch (role) {
                case CompletionModel::CompletionRole:
                case CompletionModel::SearchRole:
                case Qt::DisplayRole:
                        return roomMembers_[index.row()].display_name;
                case Avatar:
                        return roomMembers_[index.row()].avatar;
                }
        }
        return {};
}
