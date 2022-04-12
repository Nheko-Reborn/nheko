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
    // Just in case a room is joined while the model is being built.
    auto modelSize = m_parent->models.size();

    m_modelAccess.lock();
    for (const auto &model : std::as_const(m_parent->models)) {
        MainWindow::instance()->imageProvider()->download(
          model->roomAvatarUrl().remove("mxc://"),
          {128, 128},
          [this, message, model, modelSize](
            const QString &, const QSize &, const QImage &image, const QString &) {
              const auto aliases = cache::client()->getRoomAliases(model->roomId().toStdString());
              QString alias;
              if (aliases.has_value()) {
                  const auto &val = aliases.value();
                  if (!val.alias.empty())
                      alias = QString::fromStdString(val.alias);
                  else if (val.alt_aliases.size() > 0)
                      alias = QString::fromStdString(val.alt_aliases.front());
              }

              m_model->push_back(
                nheko::dbus::RoomInfoItem{model->roomId(), model->roomName(), alias, image});

              if (m_model->length() == modelSize) {
                  auto reply = message.createReply();
                  nhlog::ui()->debug("Sending {} rooms over D-Bus...", m_model->size());
                  reply << QVariant::fromValue(*m_model);
                  QDBusConnection::sessionBus().send(reply);
                  nhlog::ui()->debug("Rooms successfully sent to D-Bus.");

                  m_model->clear();
                  m_modelAccess.unlock();
              }
          },
          false);
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
RoomListDBusInterface::bringWindowToTop() const
{
    MainWindow::instance()->show();
    MainWindow::instance()->raise();
}
