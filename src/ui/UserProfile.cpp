#include "UserProfile.h"
#include "Cache.h"
#include "ChatPage.h"
#include "DeviceVerificationFlow.h"
#include "Logging.h"
#include "Utils.h"
#include "mtx/responses/crypto.hpp"
#include "timeline/TimelineModel.h"

#include <iostream> // only for debugging

UserProfile::UserProfile(QString roomid, QString userid, TimelineModel *parent)
  : QObject(parent)
  , roomid_(roomid)
  , userid_(userid)
  , model(parent)
{
        fetchDeviceList(this->userid_);
}

QHash<int, QByteArray>
DeviceInfoModel::roleNames() const
{
        return {
          {DeviceId, "deviceId"},
          {DeviceName, "deviceName"},
          {VerificationStatus, "verificationStatus"},
        };
}

QVariant
DeviceInfoModel::data(const QModelIndex &index, int role) const
{
        if (!index.isValid() || index.row() >= (int)deviceList_.size() || index.row() < 0)
                return {};

        switch (role) {
        case DeviceId:
                return deviceList_[index.row()].device_id;
        case DeviceName:
                return deviceList_[index.row()].display_name;
        case VerificationStatus:
                return QVariant::fromValue(deviceList_[index.row()].verification_status);
        default:
                return {};
        }
}

void
DeviceInfoModel::reset(const std::vector<DeviceInfo> &deviceList)
{
        beginResetModel();
        this->deviceList_ = std::move(deviceList);
        endResetModel();
}

DeviceInfoModel *
UserProfile::deviceList()
{
        return &this->deviceList_;
}

QString
UserProfile::userid()
{
        return this->userid_;
}

QString
UserProfile::displayName()
{
        return cache::displayName(roomid_, userid_);
}

QString
UserProfile::avatarUrl()
{
        return cache::avatarUrl(roomid_, userid_);
}

bool
UserProfile::getUserStatus()
{
        return isUserVerified;
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
        std::vector<DeviceInfo> deviceInfo;
        auto device_verified = cache::getVerifiedCache(user_id);

        for (const auto &d : devices) {
                auto device = d.second;

                // TODO: Verify signatures and ignore those that don't pass.
                verification::Status verified = verification::Status::UNVERIFIED;
                isUserVerified                = device_verified->is_user_verified;
                if (device_verified.has_value()) {
                        if (std::find(device_verified->cross_verified.begin(),
                                      device_verified->cross_verified.end(),
                                      d.first) != device_verified->cross_verified.end())
                                verified = verification::Status::VERIFIED;
                        if (std::find(device_verified->device_verified.begin(),
                                      device_verified->device_verified.end(),
                                      d.first) != device_verified->device_verified.end())
                                verified = verification::Status::VERIFIED;
                        if (std::find(device_verified->device_blocked.begin(),
                                      device_verified->device_blocked.end(),
                                      d.first) != device_verified->device_blocked.end())
                                verified = verification::Status::BLOCKED;
                }

                deviceInfo.push_back(
                  {QString::fromStdString(d.first),
                   QString::fromStdString(device.unsigned_info.device_display_name),
                   verified});
        }

        std::sort(
          deviceInfo.begin(), deviceInfo.end(), [](const DeviceInfo &a, const DeviceInfo &b) {
                  return a.device_id > b.device_id;
          });

        this->deviceList_.queueReset(std::move(deviceInfo));
}

void
UserProfile::fetchDeviceList(const QString &userID)
{
        auto localUser  = utils::localUser();
        auto user_cache = cache::getUserCache(userID.toStdString());

        if (user_cache.has_value()) {
                this->callback_fn(user_cache->keys, {}, userID.toStdString());
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
UserProfile::banUser()
{
        ChatPage::instance()->banUser(this->userid_, "");
}

// void ignoreUser(){

// }

void
UserProfile::kickUser()
{
        ChatPage::instance()->kickUser(this->userid_, "");
}

void
UserProfile::startChat()
{
        mtx::requests::CreateRoom req;
        req.preset     = mtx::requests::Preset::PrivateChat;
        req.visibility = mtx::requests::Visibility::Private;
        if (utils::localUser() != this->userid_)
                req.invite = {this->userid_.toStdString()};
        emit ChatPage::instance()->createRoom(req);
}

DeviceVerificationFlow *
UserProfile::createFlow(bool isVerifyUser)
{
        if (!isVerifyUser)
                return (new DeviceVerificationFlow(this, DeviceVerificationFlow::Type::ToDevice));
        else {
                std::cout << "CHECKING IF IT TO START ROOM_VERIFICATION OR TO_DEVICE VERIFICATION"
                          << std::endl;
                auto joined_rooms = cache::joinedRooms();
                auto room_infos   = cache::getRoomInfo(joined_rooms);

                for (std::string room_id : joined_rooms) {
                        if ((room_infos[QString::fromStdString(room_id)].member_count == 2) &&
                            cache::isRoomEncrypted(room_id)) {
                                auto room_members = cache::roomMembers(room_id);
                                if (std::find(room_members.begin(),
                                              room_members.end(),
                                              (this->userid()).toStdString()) !=
                                    room_members.end()) {
                                        std::cout
                                          << "FOUND A ENCRYPTED ROOM WITH THIS USER : " << room_id
                                          << std::endl;
                                        if (this->roomid_.toStdString() == room_id) {
                                                auto newflow = new DeviceVerificationFlow(
                                                  this, DeviceVerificationFlow::Type::RoomMsg);
                                                newflow->setModel(this->model);
                                                return (std::move(newflow));
                                        } else {
                                                std::cout << "FOUND A ENCRYPTED ROOM BUT CURRENTLY "
                                                             "NOT IN THAT ROOM : "
                                                          << room_id << std::endl;
                                        }
                                }
                        }
                }

                std::cout << "DIDN'T FIND A ENCRYPTED ROOM WITH THIS USER" << std::endl;
                return (new DeviceVerificationFlow(this, DeviceVerificationFlow::Type::ToDevice));
        }
}