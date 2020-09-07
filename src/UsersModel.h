#pragma once

#include <QAbstractListModel>

class RoomMember;

class UsersModel : public QAbstractListModel
{
public:
        enum Roles
        {
                Avatar = Qt::UserRole // QImage avatar
        };

        UsersModel(const std::string &roomId, QObject *parent = nullptr);
        int rowCount(const QModelIndex &parent = QModelIndex()) const override
        {
                return (parent == QModelIndex()) ? roomMembers_.size() : 0;
        }
        QVariant data(const QModelIndex &index, int role) const override;

private:
        std::vector<RoomMember> roomMembers_;
};
