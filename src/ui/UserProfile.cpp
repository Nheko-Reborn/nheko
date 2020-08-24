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
UserProfile::fetchDeviceList(const QString &userID)
{
        auto localUser = utils::localUser();

        mtx::requests::QueryKeys req;
        req.device_keys[userID.toStdString()] = {};
        ChatPage::instance()->query_keys(
          req,
          [user_id = userID.toStdString(), local_user_id = localUser.toStdString(), this](
            const mtx::responses::QueryKeys &res, mtx::http::RequestErr err) {
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

                  // Finding if the User is Verified or not based on the Signatures
                  mtx::requests::QueryKeys req;
                  req.device_keys[local_user_id] = {};

                  ChatPage::instance()->query_keys(
                    req,
                    [&local_user_id, &user_id, other_res = res, this](
                      const mtx::responses::QueryKeys &res, mtx::http::RequestErr err) {
                            using namespace mtx;

                            if (err) {
                                    nhlog::net()->warn("failed to query device keys: {},{}",
                                                       err->matrix_error.errcode,
                                                       static_cast<int>(err->status_code));
                                    return;
                            }

                            std::optional<crypto::CrossSigningKeys> lmk, lsk, luk, mk, sk, uk;

                            if (res.master_keys.find(local_user_id) != res.master_keys.end())
                                    lmk = res.master_keys.at(local_user_id);
                            if (res.user_signing_keys.find(local_user_id) !=
                                res.user_signing_keys.end())
                                    luk = res.user_signing_keys.at(local_user_id);
                            if (res.self_signing_keys.find(local_user_id) !=
                                res.self_signing_keys.end())
                                    lsk = res.self_signing_keys.at(local_user_id);
                            if (other_res.master_keys.find(user_id) != other_res.master_keys.end())
                                    mk = other_res.master_keys.at(user_id);
                            if (other_res.user_signing_keys.find(user_id) !=
                                other_res.user_signing_keys.end())
                                    uk = other_res.user_signing_keys.at(user_id);
                            if (other_res.self_signing_keys.find(user_id) !=
                                other_res.self_signing_keys.end())
                                    sk = other_res.self_signing_keys.at(user_id);

                            // First checking if the user is verified
                            if (lmk.has_value() && luk.has_value()) {
                                    bool is_user_verified = false;
                                    for (auto sign_key : lmk.value().keys) {
                                            if (!luk.value().signatures.empty()) {
                                                    for (auto signature :
                                                         luk.value().signatures.at(local_user_id)) {
                                                            is_user_verified =
                                                              is_user_verified ||
                                                              (olm::client()->ed25519_verify_sig(
                                                                sign_key.second,
                                                                json(luk.value()),
                                                                signature.second));
                                                    }
                                            }
                                    }
                                    std::cout << (isUserVerified ? "Yes" : "No") << std::endl;
                            }
                    });

                  std::sort(deviceInfo.begin(),
                            deviceInfo.end(),
                            [](const DeviceInfo &a, const DeviceInfo &b) {
                                    return a.device_id > b.device_id;
                            });

                  this->deviceList_.queueReset(std::move(deviceInfo));
          });
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
                                                  this,
                                                  DeviceVerificationFlow::Type::RoomMsg,
                                                  this->model);
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