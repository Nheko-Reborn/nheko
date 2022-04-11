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
{
    connect(ChatPage::instance(), &ChatPage::newRoom, this, &RoomListDBusInterface::prepareModel);
    connect(ChatPage::instance(), &ChatPage::leftRoom, this, &RoomListDBusInterface::prepareModel);
    prepareModel();
}

QVector<nheko::dbus::RoomInfoItem>
RoomListDBusInterface::getRooms(const QDBusMessage &message)
{
    auto reply = message.createReply();

    m_modelAccess.lock();
    nhlog::ui()->debug("Sending {} rooms over D-Bus...", m_model->size());
    reply << QVariant::fromValue(*m_model);
    QDBusConnection::sessionBus().send(reply);
    nhlog::ui()->debug("Rooms successfully sent to D-Bus.");
    m_modelAccess.unlock();

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
RoomListDBusInterface::prepareModel()
{
    m_modifyStagingDataMutex.lock();
    static int modelSize;
    modelSize = m_parent->models.size();
    if (modelSize == 0) {
        m_modifyStagingDataMutex.unlock();
        return;
    }

    for (const auto &model : std::as_const(m_parent->models)) {
        MainWindow::instance()->imageProvider()->download(
          model->roomAvatarUrl().remove("mxc://"),
          {128, 128},
          [this, &model](const QString &, const QSize &, const QImage &image, const QString &) {
              const auto aliases = cache::client()->getRoomAliases(model->roomId().toStdString());
              QString alias;
              if (aliases.has_value()) {
                  const auto &val = aliases.value();
                  if (!val.alias.empty())
                      alias = QString::fromStdString(val.alias);
                  else if (val.alt_aliases.size() > 0)
                      alias = QString::fromStdString(val.alt_aliases.front());
              }

              m_addItemsToStagingData.lock();
              m_stagingModel->push_back(
                nheko::dbus::RoomInfoItem{model->roomId(), model->roomName(), alias, image});
              m_addItemsToStagingData.unlock();

              if (m_stagingModel->length() == modelSize) {
                  m_modelAccess.lock();
                  std::swap(m_model, m_stagingModel);
                  m_modelAccess.unlock();

                  m_stagingModel->clear();
                  m_modifyStagingDataMutex.unlock();

                  // If necessary, retrigger reloading of model. This is primarily aimed at startup
                  // scenarios.
                  if (m_model->size() != m_parent->models.size())
                      prepareModel();
              }
          },
          false);
    }
}

void
RoomListDBusInterface::bringWindowToTop() const
{
    MainWindow::instance()->show();
    MainWindow::instance()->raise();
}
