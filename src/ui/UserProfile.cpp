#include "UserProfile.h"
#include "Logging.h"
#include "MatrixClient.h"
#include "Utils.h"

UserProfile::UserProfile(QObject *parent)
  : QObject(parent)
{}

QMap<QString, QString>
UserProfile::getDeviceList()
{
        return this->deviceList;
}

void
UserProfile::fetchDeviceList(const QString &userId)
{
        auto localUser = utils::localUser();
        mtx::requests::QueryKeys req;
        req.device_keys[userId.toStdString()] = {};

        http::client()->query_keys(
          req,
          [user_id = userId.toStdString()](const mtx::responses::QueryKeys &res,
                                           mtx::http::RequestErr err) {
                  if (err) {
                          nhlog::net()->warn("failed to query device keys: {} {}",
                                             err->matrix_error.error,
                                             static_cast<int>(err->status_code));
                          return;
                  }

                  if (res.device_keys.empty() ||
                      (res.device_keys.find(user_id) == res.device_keys.end())) {
                          nhlog::net()->warn("no devices retrieved {}", user_id);
                          return;
                  }

                  auto devices = res.device_keys.at(user_id);

                  std::vector<DeviceInfo> deviceInfo;
                  for (const auto &d : devices) {
                          auto device = d.second;

                          // TODO: Verify signatures and ignore those that don't pass.
                          deviceInfo.emplace_back(DeviceInfo{
                            QString::fromStdString(d.first),
                            QString::fromStdString(device.unsigned_info.device_display_name)});
                  }

                  std::sort(deviceInfo.begin(),
                            deviceInfo.end(),
                            [](const DeviceInfo &a, const DeviceInfo &b) {
                                    return a.device_id > b.device_id;
                            });
          });
}
