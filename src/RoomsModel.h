#pragma once

#include "Cache.h"

#include <QAbstractListModel>
#include <QString>

class RoomsModel : public QAbstractListModel
{
public:
        enum Roles
        {
                AvatarUrl = Qt::UserRole,
                RoomAlias,
                RoomID,
                RoomName,
        };

        RoomsModel(QObject *parent = nullptr);
        QHash<int, QByteArray> roleNames() const override;
        int rowCount(const QModelIndex &parent = QModelIndex()) const override
        {
                (void)parent;
                return (int)roomAliases.size();
        }
        QVariant data(const QModelIndex &index, int role) const override;

private:
        std::vector<std::string> rooms_;
        std::vector<QString> roomids;
        std::vector<QString> roomAliases;
        std::map<QString, RoomInfo> roomInfos;
};
