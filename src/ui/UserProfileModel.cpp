#include "UserProfileModel.h"
#include <QModelIndex>

UserProfileModel::UserProfileModel(QObject *parent)
  : QAbstractListModel(parent)
  , deviceList(nullptr)
{
        this->deviceList = new UserProfile(this);

        connect(this->deviceList, &UserProfile::userIdChanged, this, [this]() {
                emit this->deviceList->updateDeviceList();
        });
        connect(this->deviceList, &UserProfile::deviceListUpdated, this, [this]() {
                beginResetModel();
                this->beginInsertRows(
                  QModelIndex(), 0, this->deviceList->getDeviceList().size() - 1);
                this->endInsertRows();
                endResetModel();
        });
}

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
        if (!index.isValid() &&
            static_cast<int>(this->deviceList->getDeviceList().size()) <= index.row())
                return QVariant();

        const DeviceInfo device = this->deviceList->getDeviceList().at(index.row());
        switch (role) {
        case DEVICEID:
                return QVariant(device.device_id);
        case DISPLAYNAME:
                return QVariant(device.display_name);
        case VERIFIED_STATUS:
                return device.verification_status;
        }
        return QVariant();
}

QHash<int, QByteArray>
UserProfileModel::roleNames() const
{
        QHash<int, QByteArray> names;
        names[DEVICEID]        = "deviceID";
        names[DISPLAYNAME]     = "displayName";
        names[VERIFIED_STATUS] = "verified_status";
        return names;
}

UserProfile *
UserProfileModel::getList() const
{
        return (this->deviceList);
}
