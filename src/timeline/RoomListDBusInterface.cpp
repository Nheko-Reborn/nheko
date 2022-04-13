// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "RoomListDBusInterface.h"

#include "Cache_p.h"
#include "ChatPage.h"
#include "Logging.h"
#include "MainWindow.h"
#include "MxcImageProvider.h"
#include "RoomlistModel.h"

#include <QDBusConnection>

RoomListDBusInterface::RoomListDBusInterface(RoomlistModel *parent)
  : QObject{parent}
  , m_parent{parent}
{}

QVector<nheko::dbus::RoomInfoItem>
RoomListDBusInterface::getRooms(const QDBusMessage &message)
{
    const auto roomListModel = m_parent->models;
    QSharedPointer<QVector<nheko::dbus::RoomInfoItem>> model{
      new QVector<nheko::dbus::RoomInfoItem>};

    for (const auto &room : roomListModel) {
        MainWindow::instance()->imageProvider()->download(
          room->roomAvatarUrl().remove("mxc://"),
          {96, 96},
          [message, room, model, roomListModel](
            const QString &, const QSize &, const QImage &image, const QString &) {
              const auto aliases = cache::client()->getRoomAliases(room->roomId().toStdString());
              QString alias;
              if (aliases.has_value()) {
                  const auto &val = aliases.value();
                  if (!val.alias.empty())
                      alias = QString::fromStdString(val.alias);
                  else if (val.alt_aliases.size() > 0)
                      alias = QString::fromStdString(val.alt_aliases.front());
              }

              model->push_back(
                nheko::dbus::RoomInfoItem{room->roomId(), room->roomName(), alias, image});

              if (model->length() == roomListModel.size()) {
                  auto reply = message.createReply();
                  nhlog::ui()->debug("Sending {} rooms over D-Bus...", model->size());
                  reply << QVariant::fromValue(*model);
                  QDBusConnection::sessionBus().send(reply);
                  nhlog::ui()->debug("Rooms successfully sent to D-Bus.");
              }
          },
          true);
    }

    return {};
}

void
RoomListDBusInterface::activateRoom(const QString &alias) const
{
    bringWindowToTop();
    m_parent->setCurrentRoom(alias);
}

void
RoomListDBusInterface::joinRoom(const QString &alias) const
{
    bringWindowToTop();
    ChatPage::instance()->joinRoom(alias);
}

void
RoomListDBusInterface::startDirectChat(const QString &userId) const
{
    bringWindowToTop();
    ChatPage::instance()->startChat(userId);
}

void
RoomListDBusInterface::bringWindowToTop() const
{
    MainWindow::instance()->show();
    MainWindow::instance()->raise();
}
