// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QFileDialog>
#include <QImageReader>
#include <QMimeDatabase>
#include <QStandardPaths>

#include "Cache.h"
#include "Cache_p.h"
#include "ChatPage.h"
#include "Logging.h"
#include "MainWindow.h"
#include "MatrixClient.h"
#include "UserProfile.h"
#include "Utils.h"
#include "encryption/VerificationManager.h"
#include "timeline/RoomlistModel.h"
#include "timeline/TimelineModel.h"
#include "timeline/TimelineViewManager.h"
#include "ui/UIA.h"

UserProfile::UserProfile(const QString &roomid,
                         const QString &userid,
                         TimelineViewManager *manager_,
                         TimelineModel *parent)
  : QObject(parent)
  , roomid_(roomid)
  , userid_(userid)
  , globalAvatarUrl{QLatin1String("")}
  , manager(manager_)
  , model(parent)
{
    connect(this,
            &UserProfile::globalUsernameRetrieved,
            this,
            &UserProfile::setGlobalUsername,
            Qt::QueuedConnection);
    connect(this,
            &UserProfile::verificationStatiChanged,
            this,
            &UserProfile::updateVerificationStatus,
            Qt::QueuedConnection);

    if (isGlobalUserProfile()) {
        getGlobalProfileData();
    }

    if (!cache::client() || !cache::client()->isDatabaseReady() ||
        !ChatPage::instance()->timelineManager())
        return;

    connect(
      cache::client(), &Cache::verificationStatusChanged, this, [this](const std::string &user_id) {
          if (user_id != this->userid_.toStdString())
              return;

          emit verificationStatiChanged();
      });
    fetchDeviceList(this->userid_);

    if (userid != utils::localUser())
        sharedRooms_ =
          new RoomInfoModel(cache::client()->getCommonRooms(userid.toStdString()), this);
    else
        sharedRooms_ = new RoomInfoModel({}, this);

    connect(ChatPage::instance(), &ChatPage::syncUI, this, [this](const mtx::responses::Sync &res) {
        if (auto ignoreEv = std::ranges::find_if(
              res.account_data.events,
              [](const mtx::events::collections::RoomAccountDataEvents &e) {
                  return std::holds_alternative<
                    mtx::events::AccountDataEvent<mtx::events::account_data::IgnoredUsers>>(e);
              });
            ignoreEv != res.account_data.events.end()) {
            // doesn't matter much if it was actually us
            emit ignoredChanged();
        }
    });
}

QHash<int, QByteArray>
DeviceInfoModel::roleNames() const
{
    return {
      {DeviceId, "deviceId"},
      {DeviceName, "deviceName"},
      {VerificationStatus, "verificationStatus"},
      {LastIp, "lastIp"},
      {LastTs, "lastTs"},
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
    case LastIp:
        return deviceList_[index.row()].lastIp;
    case LastTs:
        return deviceList_[index.row()].lastTs;
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

RoomInfoModel::RoomInfoModel(const std::map<std::string, RoomInfo> &raw, QObject *parent)
  : QAbstractListModel(parent)
{
    for (const auto &e : raw)
        roomInfos_.push_back(e);
}

QHash<int, QByteArray>
RoomInfoModel::roleNames() const
{
    return {
      {RoomId, "roomId"},
      {RoomName, "roomName"},
      {AvatarUrl, "avatarUrl"},
    };
}

QVariant
RoomInfoModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= (int)roomInfos_.size() || index.row() < 0)
        return {};

    switch (role) {
    case RoomId:
        return QString::fromStdString(roomInfos_[index.row()].first);
    case RoomName:
        return QString::fromStdString(roomInfos_[index.row()].second.name);
    case AvatarUrl:
        return QString::fromStdString(roomInfos_[index.row()].second.avatar_url);
    default:
        return {};
    }
}

DeviceInfoModel *
UserProfile::deviceList()
{
    return &this->deviceList_;
}

RoomInfoModel *
UserProfile::sharedRooms()
{
    return this->sharedRooms_;
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
    return isGlobalUserProfile() ? globalAvatarUrl : cache::avatarUrl(roomid_, userid_);
}

bool
UserProfile::isGlobalUserProfile() const
{
    return roomid_ == QLatin1String("");
}

crypto::Trust
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
UserProfile::signOutDevice(const QString &deviceID)
{
    http::client()->delete_device(
      deviceID.toStdString(),
      UIA::instance()->genericHandler(tr("Sign out device %1").arg(deviceID)),
      [this, deviceID](mtx::http::RequestErr e) {
          if (e) {
              nhlog::ui()->critical("Failure when attempting to sign out device {}",
                                    deviceID.toStdString());
              return;
          }
          nhlog::ui()->info("Device {} successfully signed out!", deviceID.toStdString());
          // This is us. Let's update the interface accordingly
          if (isSelf() && deviceID.toStdString() == ::http::client()->device_id()) {
              ChatPage::instance()->dropToLoginPageCb(tr("You signed out this device."));
          }
          refreshDevices();
      });
}

void
UserProfile::refreshDevices()
{
    cache::client()->markUserKeysOutOfDate({this->userid_.toStdString()});
    fetchDeviceList(this->userid_);
}

bool
UserProfile::ignored() const
{
    auto old = TimelineViewManager::instance()->getIgnoredUsers();
    return old.contains(userid_);
}

void
UserProfile::setIgnored(bool ignore)
{
    auto old = TimelineViewManager::instance()->getIgnoredUsers();
    if (ignore) {
        if (old.contains(userid_)) {
            emit ignoredChanged();
            return;
        }
        old.append(userid_);
    } else {
        if (!old.contains(userid_)) {
            emit ignoredChanged();
            return;
        }
        old.removeAll(userid_);
    }

    std::vector<mtx::events::account_data::IgnoredUser> content;
    for (const QString &item : std::as_const(old)) {
        content.push_back({item.toStdString()});
    }

    mtx::events::account_data::IgnoredUsers payload{.users{content}};

    auto userid = userid_;

    http::client()->put_account_data(payload, [userid](mtx::http::RequestErr e) {
        if (e) {
            MainWindow::instance()->showNotification(
              tr("Failed to ignore \"%1\": %2")
                .arg(userid, QString::fromStdString(e->matrix_error.error)));
        }
    });

    if (ignore) {
        const QHash<QString, RoomInfo> invites = cache::invites();
        FilteredRoomlistModel *room_model      = FilteredRoomlistModel::instance();

        for (auto room = invites.keyBegin(), end = invites.keyEnd(); room != end; room++) {
            if (room_model->getRoomPreviewById(*room).inviterUserId() == userid) {
                room_model->declineInvite(*room);
            }
        }
    }
}

void
UserProfile::fetchDeviceList(const QString &userID)
{
    if (!cache::client() || !cache::client()->isDatabaseReady())
        return;

    cache::client()->query_keys(
      userID.toStdString(),
      [other_user_id = userID.toStdString(), this](const UserKeyCache &,
                                                   mtx::http::RequestErr err) {
          if (err) {
              nhlog::net()->warn("failed to query device keys: {}", *err);
          }

          // Ensure local key cache is up to date
          cache::client()->query_keys(
            utils::localUser().toStdString(),
            [this](const UserKeyCache &, mtx::http::RequestErr err) {
                using namespace mtx;
                std::string local_user_id = utils::localUser().toStdString();

                if (err) {
                    nhlog::net()->warn("failed to query device keys: {}", *err);
                }

                emit verificationStatiChanged();
            });
      });
}

void
UserProfile::updateVerificationStatus()
{
    if (!cache::client() || !cache::client()->isDatabaseReady())
        return;

    auto user_keys = cache::client()->userKeys(userid_.toStdString());
    if (!user_keys) {
        this->hasMasterKey   = false;
        this->isUserVerified = crypto::Trust::Unverified;
        this->deviceList_.reset({});
        emit userStatusChanged();
        return;
    }

    this->hasMasterKey = !user_keys->master_keys.keys.empty();

    std::vector<DeviceInfo> deviceInfo;
    auto devices            = user_keys->device_keys;
    auto verificationStatus = cache::client()->verificationStatus(userid_.toStdString());

    this->isUserVerified = verificationStatus.user_verified;
    emit userStatusChanged();

    deviceInfo.reserve(devices.size());
    for (const auto &d : devices) {
        auto device = d.second;
        verification::Status verified =
          std::find(verificationStatus.verified_devices.begin(),
                    verificationStatus.verified_devices.end(),
                    device.device_id) == verificationStatus.verified_devices.end()
            ? verification::UNVERIFIED
            : verification::VERIFIED;

        if (isSelf() && device.device_id == ::http::client()->device_id())
            verified = verification::Status::SELF;

        deviceInfo.emplace_back(QString::fromStdString(d.first),
                                QString::fromStdString(device.unsigned_info.device_display_name),
                                verified);
    }

    // For self, also query devices without keys
    if (isSelf()) {
        http::client()->query_devices(
          [this, deviceInfo](const mtx::responses::QueryDevices &allDevs,
                             mtx::http::RequestErr err) mutable {
              if (err) {
                  nhlog::net()->warn("failed to query device keys: {}", *err);
                  this->deviceList_.queueReset(std::move(deviceInfo));
                  emit devicesChanged();
                  return;
              }
              for (const auto &d : allDevs.devices) {
                  // First, check if we already have an entry for this device
                  bool found = false;
                  for (auto &e : deviceInfo) {
                      if (e.device_id.toStdString() == d.device_id) {
                          found = true;
                          // Gottem! Let's fill in the blanks
                          e.lastIp = QString::fromStdString(d.last_seen_ip);
                          e.lastTs = static_cast<qlonglong>(d.last_seen_ts);
                          break;
                      }
                  }
                  // No entry? Let's add one.
                  if (!found) {
                      deviceInfo.emplace_back(QString::fromStdString(d.device_id),
                                              QString::fromStdString(d.display_name),
                                              verification::NOT_APPLICABLE,
                                              QString::fromStdString(d.last_seen_ip),
                                              d.last_seen_ts);
                  }
              }

              this->deviceList_.queueReset(std::move(deviceInfo));
              emit devicesChanged();
          });
        return;
    }

    this->deviceList_.queueReset(std::move(deviceInfo));
    emit devicesChanged();
}

void
UserProfile::banUser()
{
    ChatPage::instance()->banUser(roomid_, this->userid_, QLatin1String(""));
}

void
UserProfile::kickUser()
{
    ChatPage::instance()->kickUser(roomid_, this->userid_, QLatin1String(""));
}

void
UserProfile::startChat(bool encryption)
{
    ChatPage::instance()->startChat(this->userid_, encryption);
}

void
UserProfile::startChat()
{
    ChatPage::instance()->startChat(this->userid_, std::nullopt);
}

void
UserProfile::changeUsername(const QString &username)
{
    if (isGlobalUserProfile()) {
        // change global
        http::client()->set_displayname(username.toStdString(), [](mtx::http::RequestErr err) {
            if (err) {
                nhlog::net()->warn("could not change username: {}", *err);
                return;
            }
        });
    } else {
        // change room username
        mtx::events::state::Member member;
        member.display_name = username.toStdString();
        member.avatar_url =
          cache::avatarUrl(roomid_, QString::fromStdString(http::client()->user_id().to_string()))
            .toStdString();
        member.membership = mtx::events::state::Membership::Join;

        updateRoomMemberState(std::move(member));
    }
}

void
UserProfile::changeDeviceName(const QString &deviceID, const QString &deviceName)
{
    http::client()->set_device_name(
      deviceID.toStdString(), deviceName.toStdString(), [this](mtx::http::RequestErr err) {
          if (err) {
              nhlog::net()->warn("could not change device name: {}", *err);
              return;
          }
          refreshDevices();
      });
}

void
UserProfile::verify(QString device)
{
    if (!device.isEmpty())
        manager->verificationManager()->verifyDevice(userid_, device);
    else {
        manager->verificationManager()->verifyUser(userid_);
    }
}

void
UserProfile::unverify(const QString &device)
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

    const auto format = mime.name().split(QStringLiteral("/"))[0];

    QFile file{fileName, this};
    if (format != QLatin1String("image")) {
        emit displayError(tr("The selected file is not an image"));
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        emit displayError(tr("Error while reading file: %1").arg(file.errorString()));
        return;
    }

    const auto bin     = file.peek(file.size());
    const auto payload = std::string(bin.data(), bin.size());

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
       content = std::move(bin)](const mtx::responses::ContentURI &res, mtx::http::RequestErr err) {
          if (err) {
              nhlog::ui()->error("Failed to upload image: {}", *err);
              return;
          }

          if (isGlobalUserProfile()) {
              http::client()->set_avatar_url(res.content_uri, [this](mtx::http::RequestErr err) {
                  if (err) {
                      nhlog::ui()->error("Failed to set user avatar url: {}", *err);
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
    http::client()->send_state_event(roomid_.toStdString(),
                                     http::client()->user_id().to_string(),
                                     member,
                                     [](mtx::responses::EventId, mtx::http::RequestErr err) {
                                         if (err)
                                             nhlog::net()->error(
                                               "Failed to update room member state: {}", *err);
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
    auto profProx = std::make_shared<UserProfileFetchProxy>();
    connect(profProx.get(),
            &UserProfileFetchProxy::profileFetched,
            this,
            [this](const mtx::responses::Profile &res) {
                emit globalUsernameRetrieved(QString::fromStdString(res.display_name));
                globalAvatarUrl = QString::fromStdString(res.avatar_url);
                emit avatarUrlChanged();
            });

    connect(profProx.get(),
            &UserProfileFetchProxy::failedToFetchProfile,
            this,
            &UserProfile::failedToFetchProfile);

    http::client()->get_profile(userid_.toStdString(),
                                [prox = std::move(profProx), user = userid_.toStdString()](
                                  const mtx::responses::Profile &res, mtx::http::RequestErr err) {
                                    if (err) {
                                        nhlog::net()->warn("failed to retrieve profile info for {}",
                                                           user);
                                        emit prox->failedToFetchProfile();
                                        return;
                                    }

                                    emit prox->profileFetched(res);
                                });
}

void
UserProfile::openGlobalProfile()
{
    emit manager->openGlobalUserProfile(userid_);
}

#include "moc_UserProfile.cpp"
