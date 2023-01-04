// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-FileCopyrightText: 2023 Nheko Contributors
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
{
}

namespace {
struct RoomReplyState
{
    QVector<nheko::dbus::RoomInfoItem> model;
    std::map<QString, RoomInfo> roominfos;
    std::mutex m;
};
}

QVector<nheko::dbus::RoomInfoItem>
NhekoDBusBackend::rooms() const
{
    nhlog::ui()->debug("Rooms requested over D-Bus.");

    const auto roomListModel = m_parent->models;
    QVector<nheko::dbus::RoomInfoItem> model;

    for (const auto &room : roomListModel) {
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

        model.push_back(nheko::dbus::RoomInfoItem{room->roomId(),
                                                  alias,
                                                  room->plainRoomName(),
                                                  room->roomAvatarUrl(),
                                                  room->notificationCount()});
    }

    nhlog::ui()->debug("Sending {} rooms over D-Bus...", model.size());
    return model;
}

QImage
NhekoDBusBackend::image(const QString &uri, const QDBusMessage &message) const
{
    message.setDelayedReply(true);
    nhlog::ui()->debug("Rooms requested over D-Bus.");
    MainWindow::instance()->imageProvider()->download(
      QString(uri).remove("mxc://"),
      {96, 96},
      [message](const QString &, const QSize &, const QImage &image, const QString &) {
          auto reply = message.createReply();
          reply << QVariant::fromValue(image);
          QDBusConnection::sessionBus().send(reply);
      },
      true);
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
NhekoDBusBackend::setStatusMessage(const QString &message)
{
    ChatPage::instance()->setStatus(message);
}

void
NhekoDBusBackend::bringWindowToTop() const
{
    MainWindow::instance()->show();
    MainWindow::instance()->raise();
}
