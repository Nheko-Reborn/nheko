#include "UserProfile.h"
#include "Cache_p.h"
#include "ChatPage.h"
#include "DeviceVerificationFlow.h"
#include "Logging.h"
#include "Utils.h"
#include "mtx/responses/crypto.hpp"
#include "timeline/TimelineModel.h"
#include "timeline/TimelineViewManager.h"
#include <mtx/responses/common.hpp>
#include <mtx/responses.hpp>

UserProfile::UserProfile(QString roomid,
                         QString userid,
                         TimelineViewManager *manager_,
                         TimelineModel *parent)
  : QObject(parent)
  , roomid_(roomid)
  , userid_(userid)
  , manager(manager_)
  , model(parent)
{
        fetchDeviceList(this->userid_);

        connect(cache::client(),
                &Cache::verificationStatusChanged,
                this,
                [this](const std::string &user_id) {
                        if (user_id != this->userid_.toStdString())
                                return;

                        auto status = cache::verificationStatus(user_id);
                        if (!status)
                                return;
                        this->isUserVerified = status->user_verified;
                        emit userStatusChanged();

                        for (auto &deviceInfo : deviceList_.deviceList_) {
                                deviceInfo.verification_status =
                                  std::find(status->verified_devices.begin(),
                                            status->verified_devices.end(),
                                            deviceInfo.device_id.toStdString()) ==
                                      status->verified_devices.end()
                                    ? verification::UNVERIFIED
                                    : verification::VERIFIED;
                        }
                        deviceList_.reset(deviceList_.deviceList_);
                });

        connect(this,
                &UserProfile::globalUsernameRetrieved,
                this,
                &UserProfile::setGlobalUsername,
                Qt::QueuedConnection);

        http::client()->get_profile(
          userid_.toStdString(),
          [this](const mtx::responses::Profile &res, mtx::http::RequestErr err) {
                  if (err) {
                          nhlog::net()->warn("failed to retrieve own profile info");
                          return;
                  }

                  emit globalUsernameRetrieved(QString::fromStdString(res.display_name));
          });
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
        return globalUserProfile() ? globalUsername : cache::displayName(roomid_, userid_);
}

QString
UserProfile::avatarUrl()
{
        return cache::avatarUrl(roomid_, userid_);
}

bool
UserProfile::globalUserProfile() const
{
        return (roomid_ == "") && isSelf();
}

bool
UserProfile::getUserStatus()
{
        return isUserVerified;
}

bool
UserProfile::userVerificationEnabled() const
{
        return hasMasterKey;
}
bool
UserProfile::isSelf() const
{
        return this->userid_ == utils::localUser();
}

void
UserProfile::fetchDeviceList(const QString &userID)
{
        auto localUser = utils::localUser();

        cache::client()->query_keys(
          userID.toStdString(),
          [other_user_id = userID.toStdString(), this](const UserKeyCache &other_user_keys,
                                                       mtx::http::RequestErr err) {
                  if (err) {
                          nhlog::net()->warn("failed to query device keys: {},{}",
                                             err->matrix_error.errcode,
                                             static_cast<int>(err->status_code));
                          return;
                  }

                  // Ensure local key cache is up to date
                  cache::client()->query_keys(
                    utils::localUser().toStdString(),
                    [other_user_id, other_user_keys, this](const UserKeyCache &,
                                                           mtx::http::RequestErr err) {
                            using namespace mtx;
                            std::string local_user_id = utils::localUser().toStdString();

                            if (err) {
                                    nhlog::net()->warn("failed to query device keys: {},{}",
                                                       err->matrix_error.errcode,
                                                       static_cast<int>(err->status_code));
                                    return;
                            }

                            this->hasMasterKey = !other_user_keys.master_keys.keys.empty();

                            std::vector<DeviceInfo> deviceInfo;
                            auto devices = other_user_keys.device_keys;
                            auto verificationStatus =
                              cache::client()->verificationStatus(other_user_id);

                            isUserVerified = verificationStatus.user_verified;
                            emit userStatusChanged();

                            for (const auto &d : devices) {
                                    auto device = d.second;
                                    verification::Status verified =
                                      verification::Status::UNVERIFIED;

                                    if (std::find(verificationStatus.verified_devices.begin(),
                                                  verificationStatus.verified_devices.end(),
                                                  device.device_id) !=
                                          verificationStatus.verified_devices.end() &&
                                        mtx::crypto::verify_identity_signature(
                                          device,
                                          DeviceId(device.device_id),
                                          UserId(other_user_id)))
                                            verified = verification::Status::VERIFIED;

                                    deviceInfo.push_back(
                                      {QString::fromStdString(d.first),
                                       QString::fromStdString(
                                         device.unsigned_info.device_display_name),
                                       verified});
                            }

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
        ChatPage::instance()->startChat(this->userid_);
}

void
UserProfile::changeUsername(QString username)
{
        if (globalUserProfile()) {
                // change global
                http::client()->set_displayname(
                  username.toStdString(), [this](mtx::http::RequestErr err) {
                          if (err) {
                                  nhlog::net()->warn("could not change username");
                                  return;
                          }
                  });
        } else {
                // change room username
                mtx::events::state::Member member;
                member.display_name = username.toStdString();
                member.avatar_url =
                  cache::avatarUrl(roomid_,
                                   QString::fromStdString(http::client()->user_id().to_string()))
                    .toStdString();
                member.membership = mtx::events::state::Membership::Join;

                http::client()->send_state_event(
                  roomid_.toStdString(),
                  http::client()->user_id().to_string(),
                  member,
                  [](mtx::responses::EventId, mtx::http::RequestErr err) {
                          if (err)
                                  nhlog::net()->error("Failed to set room displayname: {}",
                                                      err->matrix_error.error);
                  });
        }

        allowUsernameEditing(false);
}

void
UserProfile::verify(QString device)
{
        if (!device.isEmpty())
                manager->verifyDevice(userid_, device);
        else {
                manager->verifyUser(userid_);
        }
}

void
UserProfile::unverify(QString device)
{
        cache::markDeviceUnverified(userid_.toStdString(), device.toStdString());
}

void
UserProfile::allowUsernameEditing(bool allow)
{
        usernameEditing = allow;
        emit usernameEditingChanged();
}

bool
UserProfile::isUsernameEditingAllowed() const
{
        return usernameEditing;
}

void
UserProfile::setGlobalUsername(const QString& globalUser)
{
        globalUsername = globalUser;
        emit displayNameChanged();
}