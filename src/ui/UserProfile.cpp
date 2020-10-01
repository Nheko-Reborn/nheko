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

        ChatPage::instance()->query_keys(
          userID.toStdString(),
          [other_user_id = userID.toStdString(), this](const UserKeyCache &other_user_keys,
                                                       mtx::http::RequestErr err) {
                  if (err) {
                          nhlog::net()->warn("failed to query device keys: {},{}",
                                             err->matrix_error.errcode,
                                             static_cast<int>(err->status_code));
                          return;
                  }

                  // Finding if the User is Verified or not based on the Signatures
                  ChatPage::instance()->query_keys(
                    utils::localUser().toStdString(),
                    [other_user_id, other_user_keys, this](const UserKeyCache &res,
                                                           mtx::http::RequestErr err) {
                            using namespace mtx;
                            std::string local_user_id = utils::localUser().toStdString();

                            if (err) {
                                    nhlog::net()->warn("failed to query device keys: {},{}",
                                                       err->matrix_error.errcode,
                                                       static_cast<int>(err->status_code));
                                    return;
                            }

                            if (res.device_keys.empty()) {
                                    nhlog::net()->warn("no devices retrieved {}", local_user_id);
                                    return;
                            }

                            std::vector<DeviceInfo> deviceInfo;
                            auto devices         = other_user_keys.device_keys;
                            auto device_verified = cache::verificationStatus(other_user_id);

                            if (device_verified.has_value()) {
                                    // TODO: properly check cross-signing signatures here
                                    isUserVerified = !device_verified->verified_master_key.empty();
                            }

                            std::optional<crypto::CrossSigningKeys> lmk, lsk, luk, mk, sk, uk;

                            lmk = res.master_keys;
                            luk = res.user_signing_keys;
                            lsk = res.self_signing_keys;
                            mk  = other_user_keys.master_keys;
                            uk  = other_user_keys.user_signing_keys;
                            sk  = other_user_keys.self_signing_keys;

                            // First checking if the user is verified
                            if (luk.has_value() && mk.has_value()) {
                                    // iterating through the public key of local user_signing keys
                                    for (auto sign_key : luk.value().keys) {
                                            // checking if the signatures are empty as "at" could
                                            // cause exceptions
                                            auto signs = mk->signatures;
                                            if (!signs.empty() &&
                                                signs.find(local_user_id) != signs.end()) {
                                                    auto sign = signs.at(local_user_id);
                                                    try {
                                                            isUserVerified =
                                                              isUserVerified ||
                                                              (olm::client()->ed25519_verify_sig(
                                                                sign_key.second,
                                                                json(mk.value()),
                                                                sign.at(sign_key.first)));
                                                    } catch (std::out_of_range &) {
                                                            isUserVerified =
                                                              isUserVerified || false;
                                                    }
                                            }
                                    }
                            }

                            for (const auto &d : devices) {
                                    auto device = d.second;
                                    verification::Status verified =
                                      verification::Status::UNVERIFIED;

                                    if (device_verified.has_value()) {
                                            if (std::find(device_verified->cross_verified.begin(),
                                                          device_verified->cross_verified.end(),
                                                          d.first) !=
                                                device_verified->cross_verified.end())
                                                    verified = verification::Status::VERIFIED;
                                            if (std::find(device_verified->device_verified.begin(),
                                                          device_verified->device_verified.end(),
                                                          d.first) !=
                                                device_verified->device_verified.end())
                                                    verified = verification::Status::VERIFIED;
                                            if (std::find(device_verified->device_blocked.begin(),
                                                          device_verified->device_blocked.end(),
                                                          d.first) !=
                                                device_verified->device_blocked.end())
                                                    verified = verification::Status::BLOCKED;
                                    } else if (isUserVerified) {
                                            device_verified = VerificationCache{};
                                    }

                                    // won't check for already verified devices
                                    if (verified != verification::Status::VERIFIED &&
                                        isUserVerified) {
                                            if ((sk.has_value()) && (!device.signatures.empty())) {
                                                    for (auto sign_key : sk.value().keys) {
                                                            auto signs =
                                                              device.signatures.at(other_user_id);
                                                            try {
                                                                    if (olm::client()
                                                                          ->ed25519_verify_sig(
                                                                            sign_key.second,
                                                                            json(device),
                                                                            signs.at(
                                                                              sign_key.first))) {
                                                                            verified =
                                                                              verification::Status::
                                                                                VERIFIED;
                                                                            device_verified.value()
                                                                              .cross_verified
                                                                              .push_back(d.first);
                                                                    }
                                                            } catch (std::out_of_range &) {
                                                            }
                                                    }
                                            }
                                    }

                                    // TODO(Nico): properly show cross-signing
                                    // if (device_verified.has_value()) {
                                    //        device_verified.value().is_user_verified =
                                    //          isUserVerified;
                                    //        cache::setVerifiedCache(user_id,
                                    //                                device_verified.value());
                                    //}

                                    deviceInfo.push_back(
                                      {QString::fromStdString(d.first),
                                       QString::fromStdString(
                                         device.unsigned_info.device_display_name),
                                       verified});
                            }

                            std::cout << (isUserVerified ? "Yes" : "No") << std::endl;

                            std::sort(deviceInfo.begin(),
                                      deviceInfo.end(),
                                      [](const DeviceInfo &a, const DeviceInfo &b) {
                                              return a.device_id > b.device_id;
                                      });

                            this->deviceList_.queueReset(std::move(deviceInfo));
                    });
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
