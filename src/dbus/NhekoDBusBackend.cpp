// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "NhekoDBusBackend.h"

#include <mutex>

#include "Cache.h"
#include "Cache_p.h"
#include "ChatPage.h"
#include "Logging.h"
#include "MainWindow.h"
#include "MxcImageProvider.h"
#include "timeline/RoomlistModel.h"

#include <QDBusConnection>

NhekoDBusBackend::NhekoDBusBackend(RoomlistModel *parent)
  : QObject{parent}
  , m_parent{parent}
{}

namespace {
struct RoomReplyState
{
    QVector<nheko::dbus::RoomInfoItem> model;
    std::map<QString, RoomInfo> roominfos;
    std::mutex m;
};
}

QVector<nheko::dbus::RoomInfoItem>
NhekoDBusBackend::rooms(const QDBusMessage &message)
{
    message.setDelayedReply(true);
    nhlog::ui()->debug("Rooms requested over D-Bus.");

    const auto roomListModel = m_parent->models;

    auto state = QSharedPointer<RoomReplyState>::create();

    std::vector<std::string> roomids;
    roomids.reserve(roomids.size());
    for (const auto &room : roomListModel) {
        roomids.push_back(room->roomId().toStdString());
    }
    state->roominfos = cache::getRoomInfo(roomids);

    for (const auto &room : roomListModel) {
        auto addRoom = [room, roomListModelSize = roomListModel.size(), message, state](
                         const QImage &image) {
            const auto aliases = cache::client()->getStateEvent<mtx::events::state::CanonicalAlias>(
              room->roomId().toStdString());
            QString alias;
            if (aliases.has_value()) {
                const auto &val = aliases.value().content;
                if (!val.alias.empty())
                    alias = QString::fromStdString(val.alias);
                else if (val.alt_aliases.size() > 0)
                    alias = QString::fromStdString(val.alt_aliases.front());
            }

            std::lock_guard<std::mutex> childLock(state->m);
            state->model.push_back(nheko::dbus::RoomInfoItem{
              room->roomId(),
              alias,
              QString::fromStdString(state->roominfos[room->roomId()].name),
              image,
              room->notificationCount()});

            if (state->model.size() == roomListModelSize) {
                nhlog::ui()->debug("Sending {} rooms over D-Bus...", state->model.size());
                auto reply = message.createReply();
                reply << QVariant::fromValue(state->model);
                QDBusConnection::sessionBus().send(reply);
                nhlog::ui()->debug("Rooms successfully sent to D-Bus.");
            } else {
                // nhlog::ui()->debug("DBUS: {}/{}", state->model.size(), roomListModelSize);
            }
        };

        if (state->roominfos[room->roomId()].avatar_url.empty())
            addRoom(QImage());
        else
            MainWindow::instance()->imageProvider()->download(
              QString::fromStdString(state->roominfos[room->roomId()].avatar_url).remove("mxc://"),
              {96, 96},
              [addRoom](const QString &, const QSize &, const QImage &image, const QString &) {
                  addRoom(image);
              },
              true);
    }

    return {};
}

void
NhekoDBusBackend::activateRoom(const QString &alias) const
{
    bringWindowToTop();
    m_parent->setCurrentRoom(alias);
}

void
NhekoDBusBackend::joinRoom(const QString &alias) const
{
    bringWindowToTop();
    ChatPage::instance()->joinRoom(alias);
}

void
NhekoDBusBackend::directChat(const QString &userId) const
{
    bringWindowToTop();
    ChatPage::instance()->startChat(userId);
}

void
NhekoDBusBackend::bringWindowToTop() const
{
    MainWindow::instance()->show();
    MainWindow::instance()->raise();
}
