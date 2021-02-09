#include <QFileDialog>
#include <QImageReader>
#include <QMimeDatabase>
#include <QStandardPaths>

#include "Cache_p.h"
#include "ChatPage.h"
#include "DeviceVerificationFlow.h"
#include "Logging.h"
#include "UserProfile.h"
#include "Utils.h"
#include "mtx/responses/crypto.hpp"
#include "timeline/TimelineModel.h"
#include "timeline/TimelineViewManager.h"

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
        globalAvatarUrl = "";

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

        if (isGlobalUserProfile()) {
                getGlobalProfileData();
        }
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
        return isGlobalUserProfile() ? globalUsername : cache::displayName(roomid_, userid_);
}

QString
UserProfile::avatarUrl()
{
        return (isGlobalUserProfile() && globalAvatarUrl != "")
                 ? globalAvatarUrl
                 : cache::avatarUrl(roomid_, userid_);
        ;
}

bool
UserProfile::isGlobalUserProfile() const
{
        return roomid_ == "";
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
        if (isGlobalUserProfile()) {
                // change global
                http::client()->set_displayname(
                  username.toStdString(), [](mtx::http::RequestErr err) {
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

                updateRoomMemberState(std::move(member));
        }
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
UserProfile::setGlobalUsername(const QString &globalUser)
{
        globalUsername = globalUser;
        emit displayNameChanged();
}

void
UserProfile::changeAvatar()
{
        const QString picturesFolder =
          QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
        const QString fileName = QFileDialog::getOpenFileName(
          nullptr, tr("Select an avatar"), picturesFolder, tr("All Files (*)"));

        if (fileName.isEmpty())
                return;

        QMimeDatabase db;
        QMimeType mime = db.mimeTypeForFile(fileName, QMimeDatabase::MatchContent);

        const auto format = mime.name().split("/")[0];

        QFile file{fileName, this};
        if (format != "image") {
                emit displayError(tr("The selected file is not an image"));
                return;
        }

        if (!file.open(QIODevice::ReadOnly)) {
                emit displayError(tr("Error while reading file: %1").arg(file.errorString()));
                return;
        }

        const auto bin        = file.peek(file.size());
        const auto payload    = std::string(bin.data(), bin.size());

        isLoading_ = true;
        emit loadingChanged();

        // First we need to create a new mxc URI
        // (i.e upload media to the Matrix content repository) for the new avatar.
        http::client()->upload(
          payload,
          mime.name().toStdString(),
          QFileInfo(fileName).fileName().toStdString(),
          [this,
           payload,
           mimetype = mime.name().toStdString(),
           size     = payload.size(),
           room_id  = roomid_.toStdString(),
           content  = std::move(bin)](const mtx::responses::ContentURI &res,
                                     mtx::http::RequestErr err) {
                  if (err) {
                          nhlog::ui()->error("Failed to upload image", err->matrix_error.error);
                          return;
                  }

                  if (isGlobalUserProfile()) {
                          http::client()->set_avatar_url(
                            res.content_uri, [this](mtx::http::RequestErr err) {
                                    if (err) {
                                            nhlog::ui()->error("Failed to set user avatar url",
                                                               err->matrix_error.error);
                                    }

                                    isLoading_ = false;
                                    emit loadingChanged();
                                    getGlobalProfileData();
                            });
                  } else {
                          // change room username
                          mtx::events::state::Member member;
                          member.display_name = cache::displayName(roomid_, userid_).toStdString();
                          member.avatar_url   = res.content_uri;
                          member.membership   = mtx::events::state::Membership::Join;

                          updateRoomMemberState(std::move(member));
                  }
          });
}

void
UserProfile::updateRoomMemberState(mtx::events::state::Member member)
{
        http::client()->send_state_event(
          roomid_.toStdString(),
          http::client()->user_id().to_string(),
          member,
          [](mtx::responses::EventId, mtx::http::RequestErr err) {
                  if (err)
                          nhlog::net()->error("Failed to update room member state : ",
                                              err->matrix_error.error);
          });
}

void
UserProfile::updateAvatarUrl()
{
        isLoading_ = false;
        emit loadingChanged();

        emit avatarUrlChanged();
}

bool
UserProfile::isLoading() const
{
        return isLoading_;
}

void
UserProfile::getGlobalProfileData()
{
        http::client()->get_profile(
          userid_.toStdString(),
          [this](const mtx::responses::Profile &res, mtx::http::RequestErr err) {
                  if (err) {
                          nhlog::net()->warn("failed to retrieve own profile info");
                          return;
                  }

                  emit globalUsernameRetrieved(QString::fromStdString(res.display_name));
                  globalAvatarUrl = QString::fromStdString(res.avatar_url);
                  emit avatarUrlChanged();
          });
}
