#pragma once

#include "UserProfile.h"
#include <QAbstractListModel>

class UserProfile; // forward declaration of the class UserProfile

class UserProfileModel : public QAbstractListModel
{
        Q_OBJECT
        Q_PROPERTY(UserProfile *deviceList READ getList)

public:
        explicit UserProfileModel(QObject *parent = nullptr);

        enum
        {
                DEVICEID,
                DISPLAYNAME,
                VERIFIED_STATUS
        };
        UserProfile *getList() const;

        int rowCount(const QModelIndex &parent = QModelIndex()) const override;
        QVariant data(const QModelIndex &index, int role) const override;
        virtual QHash<int, QByteArray> roleNames() const override;

private:
        UserProfile *deviceList;
};