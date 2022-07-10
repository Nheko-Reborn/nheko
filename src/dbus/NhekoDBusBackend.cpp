// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "NhekoDBusBackend.h"

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

QVector<nheko::dbus::RoomInfoItem>
NhekoDBusBackend::rooms(const QDBusMessage &message)
{
    message.setDelayedReply(true);

    const auto roomListModel = m_parent->models;
    QSharedPointer<QVector<nheko::dbus::RoomInfoItem>> model{
      new QVector<nheko::dbus::RoomInfoItem>};

    for (const auto &room : roomListModel) {
        auto addRoom = [room, roomListModelSize = roomListModel.size(), message, model](
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

            model->push_back(nheko::dbus::RoomInfoItem{
              room->roomId(), alias, room->roomName(), image, room->notificationCount()});

            if (model->length() == roomListModelSize) {
                nhlog::ui()->debug("Sending {} rooms over D-Bus...", model->size());
                auto reply = message.createReply();
                reply << QVariant::fromValue(*model);
                QDBusConnection::sessionBus().send(reply);
                nhlog::ui()->debug("Rooms successfully sent to D-Bus.");
            }
        };

        auto avatarUrl = room->roomAvatarUrl();
        if (avatarUrl.isEmpty())
            addRoom(QImage());
        else
            MainWindow::instance()->imageProvider()->download(
              avatarUrl.remove("mxc://"),
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
