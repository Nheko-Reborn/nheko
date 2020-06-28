#include "UserProfile.h"
#include "Cache.h"
#include "ChatPage.h"
#include "Logging.h"
#include "Utils.h"
#include "mtx/responses/crypto.hpp"

#include <iostream> // only for debugging

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
UserProfile::callback_fn(const mtx::responses::QueryKeys &res,
                         mtx::http::RequestErr err,
                         std::string user_id)
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

        std::sort(
          deviceInfo.begin(), deviceInfo.end(), [](const DeviceInfo &a, const DeviceInfo &b) {
                  return a.device_id > b.device_id;
          });

        this->deviceList = std::move(deviceInfo);
        emit UserProfile::deviceListUpdated();
}

void
UserProfile::fetchDeviceList(const QString &userID)
{
        auto localUser  = utils::localUser();
        auto user_cache = cache::getUserCache(userID.toStdString());

        if (user_cache.user_id == userID.toStdString()) {
                mtx::http::ClientError error;
                this->callback_fn(user_cache.keys, std::move(error), userID.toStdString());
        } else {
                mtx::requests::QueryKeys req;
                req.device_keys[userID.toStdString()] = {};
                http::client()->query_keys(
                  req,
                  [user_id = userID.toStdString(), this](const mtx::responses::QueryKeys &res,
                                                         mtx::http::RequestErr err) {
                          this->callback_fn(res, err, user_id);
                  });
        }
}

void
UserProfile::updateDeviceList()
{
        fetchDeviceList(this->userId);
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
