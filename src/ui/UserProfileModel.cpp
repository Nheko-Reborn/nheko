#include "UserProfileModel.h"
#include "UserProfile.h"
#include <QModelIndex>

UserProfileModel::UserProfileModel(QObject *parent)
  : QAbstractListModel(parent)
  , deviceList(nullptr)
{}

int
UserProfileModel::rowCount(const QModelIndex &parent) const
{
        if (parent.isValid() || !this->deviceList)
                return 0;
        return this->deviceList->getDeviceList().size();
}

QVariant
UserProfileModel::data(const QModelIndex &index, int role) const
{
        if (!index.isValid() || !this->deviceList)
                return QVariant();

        const DeviceInfo device = this->deviceList->getDeviceList().at(index.row());
        switch (role) {
        case DEVICEID:
                return QVariant(device.device_id);
        case DISPLAYNAME:
                return QVariant(device.display_name);
        }
        return QVariant();
}

QHash<int, QByteArray>
UserProfileModel::roleNames() const
{
        QHash<int, QByteArray> names;
        names[DEVICEID]    = "deviceID";
        names[DISPLAYNAME] = "displayName";
        return names;
}

UserProfile *
UserProfileModel::getList() const
{
        return (this->deviceList);
}

void
UserProfileModel::setList(UserProfile *devices)
{
        beginResetModel();

        if (devices)
                devices->disconnect(this);

        if (this->deviceList) {
                const int index = this->deviceList->getDeviceList().size();
                beginInsertRows(QModelIndex(), index, index);
                endInsertRows();
        }

        this->deviceList = devices;

        endResetModel();
}