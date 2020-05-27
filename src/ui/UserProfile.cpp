#include "UserProfile.h"
#include "Logging.h"
#include "Utils.h"
#include "mtx/responses/crypto.hpp"

UserProfile::UserProfile(QObject *parent)
  : QObject(parent)
{}

QVector<DeviceInfo>
UserProfile::getDeviceList()
{
        return this->deviceList;
}

QString
UserProfile::getUserId()
{
        return this->userId;
}

void
UserProfile::setUserId(const QString &user_id)
{
        if (this->userId != userId)
                return;
        else {
                this->userId = user_id;
                emit UserProfile::userIdChanged();
        }
}

void
UserProfile::fetchDeviceList(const QString &userID)
{
        auto localUser = utils::localUser();
        mtx::requests::QueryKeys req;
        mtx::responses::QueryKeys res;
        req.device_keys[userID.toStdString()] = {};

        http::client()->query_keys(
          req,
          [user_id = userID.toStdString(), this](const mtx::responses::QueryKeys &res,
                                                 mtx::http::RequestErr err) {
                  if (err) {
                          nhlog::net()->warn("failed to query device keys: {},{}",
                                             err->matrix_error.errcode,
                                             static_cast<int>(err->status_code));
                          return;
                  }

                  if (res.device_keys.empty() ||
                      (res.device_keys.find(user_id) == res.device_keys.end())) {
                          nhlog::net()->warn("no devices retrieved {}", user_id);
                          return;
                  }

                  auto devices = res.device_keys.at(user_id);
                  QVector<DeviceInfo> deviceInfo;

                  for (const auto &d : devices) {
                          auto device = d.second;

                          // TODO: Verify signatures and ignore those that don't pass.
                          DeviceInfo newdevice(
                            QString::fromStdString(d.first),
                            QString::fromStdString(device.unsigned_info.device_display_name));
                          QString::fromStdString(device.unsigned_info.device_display_name);

                          deviceInfo.append(std::move(newdevice));
                  }

                  std::sort(deviceInfo.begin(),
                            deviceInfo.end(),
                            [](const DeviceInfo &a, const DeviceInfo &b) {
                                    return a.device_id > b.device_id;
                            });

                  this->deviceList = std::move(deviceInfo);
                  emit UserProfile::deviceListUpdated();
          });
}

void
UserProfile::updateDeviceList()
{
        fetchDeviceList(this->userId);
}
