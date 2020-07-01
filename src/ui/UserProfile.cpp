#include "UserProfile.h"
#include "Cache.h"
#include "ChatPage.h"
#include "Logging.h"
#include "Utils.h"
#include "mtx/responses/crypto.hpp"

#include <iostream> // only for debugging

Q_DECLARE_METATYPE(UserProfile::Status)

UserProfile::UserProfile(QObject *parent)
  : QObject(parent)
{
        qRegisterMetaType<UserProfile::Status>();
        connect(
          this, &UserProfile::updateDeviceList, this, [this]() { fetchDeviceList(this->userId); });
        connect(
          this,
          &UserProfile::appendDeviceList,
          this,
          [this](QString device_id, QString device_name, UserProfile::Status verification_status) {
                  this->deviceList.push_back(
                    DeviceInfo{device_id, device_name, verification_status});
          });
}

std::vector<DeviceInfo>
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
UserProfile::callback_fn(const mtx::responses::QueryKeys &res,
                         mtx::http::RequestErr err,
                         std::string user_id,
                         std::optional<std::vector<std::string>> cross_verified)
{
        if (err) {
                nhlog::net()->warn("failed to query device keys: {},{}",
                                   err->matrix_error.errcode,
                                   static_cast<int>(err->status_code));
                return;
        }

        if (res.device_keys.empty() || (res.device_keys.find(user_id) == res.device_keys.end())) {
                nhlog::net()->warn("no devices retrieved {}", user_id);
                return;
        }

        auto devices = res.device_keys.at(user_id);
        std::vector<DeviceInfo> deviceInfo;
        auto device_verified = cache::getVerifiedCache(user_id);

        for (const auto &d : devices) {
                auto device = d.second;

                // TODO: Verify signatures and ignore those that don't pass.
                UserProfile::Status verified = UserProfile::Status::UNVERIFIED;
                if (cross_verified.has_value()) {
                        if (std::find(cross_verified->begin(), cross_verified->end(), d.first) !=
                            cross_verified->end())
                                verified = UserProfile::Status::VERIFIED;
                } else if (device_verified.has_value()) {
                        if (std::find(device_verified->device_verified.begin(),
                                      device_verified->device_verified.end(),
                                      d.first) != device_verified->device_verified.end())
                                verified = UserProfile::Status::VERIFIED;
                } else if (device_verified.has_value()) {
                        if (std::find(device_verified->device_blocked.begin(),
                                      device_verified->device_blocked.end(),
                                      d.first) != device_verified->device_blocked.end())
                                verified = UserProfile::Status::BLOCKED;
                }

                emit UserProfile::appendDeviceList(
                  QString::fromStdString(d.first),
                  QString::fromStdString(device.unsigned_info.device_display_name),
                  verified);
        }

        // std::sort(
        //   deviceInfo.begin(), deviceInfo.end(), [](const DeviceInfo &a, const DeviceInfo &b) {
        //           return a.device_id > b.device_id;
        //   });

        this->deviceList = std::move(deviceInfo);
        emit UserProfile::deviceListUpdated();
}

void
UserProfile::fetchDeviceList(const QString &userID)
{
        auto localUser  = utils::localUser();
        auto user_cache = cache::getUserCache(userID.toStdString());

        if (user_cache.has_value()) {
                this->callback_fn(
                  user_cache->keys, {}, userID.toStdString(), user_cache->cross_verified);
        } else {
                mtx::requests::QueryKeys req;
                req.device_keys[userID.toStdString()] = {};
                http::client()->query_keys(
                  req,
                  [user_id = userID.toStdString(), this](const mtx::responses::QueryKeys &res,
                                                         mtx::http::RequestErr err) {
                          this->callback_fn(res, err, user_id, {});
                  });
        }
}

void
UserProfile::banUser()
{
        ChatPage::instance()->banUser(this->userId, "");
}

// void ignoreUser(){

// }

void
UserProfile::kickUser()
{
        ChatPage::instance()->kickUser(this->userId, "");
}

void
UserProfile::startChat()
{
        mtx::requests::CreateRoom req;
        req.preset     = mtx::requests::Preset::PrivateChat;
        req.visibility = mtx::requests::Visibility::Private;
        if (utils::localUser() != this->userId)
                req.invite = {this->userId.toStdString()};
        emit ChatPage::instance()->createRoom(req);
}
