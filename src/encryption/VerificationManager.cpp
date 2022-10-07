// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "VerificationManager.h"

#include <chrono>

#include "Cache.h"
#include "ChatPage.h"
#include "DeviceVerificationFlow.h"
#include "timeline/TimelineViewManager.h"

VerificationManager::VerificationManager(TimelineViewManager *o)
  : QObject(o)
  , rooms_(o->rooms())
{}

static bool
isValidTime(std::optional<uint64_t> t)
{
    if (!t)
        return false;

    using namespace std::chrono_literals;

    std::chrono::time_point<std::chrono::system_clock> time{std::chrono::milliseconds(*t)};
    auto diff = std::chrono::system_clock::now() - time;

    return diff < 10min && diff > -5min;
}

void
VerificationManager::receivedRoomDeviceVerificationRequest(
  const mtx::events::RoomEvent<mtx::events::msg::KeyVerificationRequest> &message,
  TimelineModel *model)
{
    if (this->isInitialSync_)
        return;

    if (!isValidTime(message.origin_server_ts))
        return;

    auto event_id = QString::fromStdString(message.event_id);
    if (!this->dvList.contains(event_id)) {
        if (auto flow = DeviceVerificationFlow::NewInRoomVerification(
              this, model, message.content, QString::fromStdString(message.sender), event_id)) {
            dvList[event_id] = flow;
            emit newDeviceVerificationRequest(flow.data());
        }
    }
}

void
VerificationManager::receivedDeviceVerificationRequest(
  const mtx::events::msg::KeyVerificationRequest &msg,
  std::string sender)
{
    if (this->isInitialSync_)
        return;

    if (!isValidTime(msg.timestamp))
        return;

    if (!msg.transaction_id)
        return;

    auto txnid = QString::fromStdString(msg.transaction_id.value());
    if (!this->dvList.contains(txnid)) {
        if (auto flow = DeviceVerificationFlow::NewToDeviceVerification(
              this, msg, QString::fromStdString(sender), txnid)) {
            dvList[txnid] = flow;
            emit newDeviceVerificationRequest(flow.data());
        }
    }
}

void
VerificationManager::receivedDeviceVerificationStart(
  const mtx::events::msg::KeyVerificationStart &msg,
  std::string sender)
{
    if (this->isInitialSync_)
        return;

    // can't do this for start messages sent as to_device...
    // if (!isValidTime(msg.timestamp))
    //    return;

    if (!msg.transaction_id)
        return;

    auto txnid = QString::fromStdString(msg.transaction_id.value());
    if (!this->dvList.contains(txnid)) {
        if (auto flow = DeviceVerificationFlow::NewToDeviceVerification(
              this, msg, QString::fromStdString(sender), txnid)) {
            dvList[txnid] = flow;
            emit newDeviceVerificationRequest(flow.data());
        }
    }
}

void
VerificationManager::verifyUser(QString userid)
{
    auto joined_rooms = cache::joinedRooms();
    auto room_infos   = cache::getRoomInfo(joined_rooms);

    for (const std::string &room_id : joined_rooms) {
        if ((room_infos[QString::fromStdString(room_id)].member_count == 2) &&
            cache::isRoomEncrypted(room_id)) {
            auto room_members = cache::roomMembers(room_id);
            if (std::find(room_members.begin(), room_members.end(), (userid).toStdString()) !=
                room_members.end()) {
                if (auto model = rooms_->getRoomById(QString::fromStdString(room_id))) {
                    auto flow =
                      DeviceVerificationFlow::InitiateUserVerification(this, model.data(), userid);
                    std::unique_ptr<QObject> context{new QObject(flow.get())};
                    QObject *pcontext = context.get();
                    connect(
                      model.data(),
                      &TimelineModel::updateFlowEventId,
                      pcontext,
                      [this, flow, context = std::move(context)](std::string eventId) mutable {
                          if (context->parent() == flow.get()) {
                              dvList[QString::fromStdString(eventId)] = flow;
                              context.reset();
                          }
                      });
                    emit newDeviceVerificationRequest(flow.data());
                    return;
                }
            }
        }
    }

    emit ChatPage::instance()->showNotification(
      tr("No encrypted private chat found with this user. Create an "
         "encrypted private chat with this user and try again."));
}

void
VerificationManager::removeVerificationFlow(DeviceVerificationFlow *flow)
{
    nhlog::crypto()->debug("Removing verification flow {}", (void *)flow);
    for (auto it = dvList.keyValueBegin(); it != dvList.keyValueEnd(); ++it) {
        if (it->second == flow) {
            dvList.remove((*it).first);
            return;
        }
    }
}

void
VerificationManager::verifyDevice(QString userid, QString deviceid)
{
    auto flow = DeviceVerificationFlow::InitiateDeviceVerification(this, userid, {deviceid});
    this->dvList[flow->transactionId()] = flow;
    emit newDeviceVerificationRequest(flow.data());
}

void
VerificationManager::verifyOneOfDevices(QString userid, std::vector<QString> deviceids)
{
    auto flow =
      DeviceVerificationFlow::InitiateDeviceVerification(this, userid, std::move(deviceids));
    this->dvList[flow->transactionId()] = flow;
    emit newDeviceVerificationRequest(flow.data());
}
