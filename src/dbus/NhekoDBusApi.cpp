// SPDX-FileCopyrightText: 2010 David Sansome <me@davidsansome.com>
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-FileCopyrightText: 2023 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "NhekoDBusApi.h"

#include <QDBusInterface>
#include <QDBusMetaType>
#include <QDBusReply>

namespace nheko::dbus {
void
init()
{
    qDBusRegisterMetaType<RoomInfoItem>();
    qDBusRegisterMetaType<QVector<RoomInfoItem>>();
    qDBusRegisterMetaType<QImage>();
}

bool
apiVersionIsCompatible(const QVersionNumber &clientAppVersion)
{
    if (clientAppVersion.majorVersion() != nheko::dbus::dbusApiVersion.majorVersion())
        return false;
    if (clientAppVersion.minorVersion() > nheko::dbus::dbusApiVersion.minorVersion())
        return false;
    if (clientAppVersion.minorVersion() == nheko::dbus::dbusApiVersion.minorVersion() &&
        clientAppVersion.microVersion() < nheko::dbus::dbusApiVersion.microVersion())
        return false;

    return true;
}

RoomInfoItem::RoomInfoItem(const QString &roomId,
                           const QString &alias,
                           const QString &title,
                           const QString &avatarUrl,
                           const int unreadNotifications,
                           QObject *parent)
  : QObject{parent}
  , roomId_{roomId}
  , alias_{alias}
  , roomName_{title}
  , avatarUrl_{avatarUrl}
  , unreadNotifications_{unreadNotifications}
{
}

RoomInfoItem::RoomInfoItem(const RoomInfoItem &other)
  : QObject{other.parent()}
  , roomId_{other.roomId_}
  , alias_{other.alias_}
  , roomName_{other.roomName_}
  , avatarUrl_{other.avatarUrl_}
  , unreadNotifications_{other.unreadNotifications_}
{
}

RoomInfoItem &
RoomInfoItem::operator=(const RoomInfoItem &other)
{
    roomId_              = other.roomId_;
    alias_               = other.alias_;
    roomName_            = other.roomName_;
    avatarUrl_           = other.avatarUrl_;
    unreadNotifications_ = other.unreadNotifications_;
    return *this;
}

QDBusArgument &
operator<<(QDBusArgument &arg, const RoomInfoItem &item)
{
    arg.beginStructure();
    arg << item.roomId_ << item.alias_ << item.roomName_ << item.avatarUrl_
        << item.unreadNotifications_;
    arg.endStructure();
    return arg;
}

const QDBusArgument &
operator>>(const QDBusArgument &arg, RoomInfoItem &item)
{
    arg.beginStructure();
    arg >> item.roomId_ >> item.alias_ >> item.roomName_ >> item.avatarUrl_ >>
      item.unreadNotifications_;

    arg.endStructure();
    return arg;
}

QString
apiVersion()
{
    if (QDBusInterface interface{QStringLiteral(NHEKO_DBUS_SERVICE_NAME), QStringLiteral("/")};
        interface.isValid())
        return QDBusReply<QString>{interface.call(QStringLiteral("apiVersion"))}.value();
    else
        return {};
}

QString
nhekoVersion()
{
    if (QDBusInterface interface{QStringLiteral(NHEKO_DBUS_SERVICE_NAME), QStringLiteral("/")};
        interface.isValid())
        return QDBusReply<QString>{interface.call(QStringLiteral("nhekoVersion"))}.value();
    else
        return {};
}

QVector<RoomInfoItem>
rooms()
{
    if (QDBusInterface interface{QStringLiteral(NHEKO_DBUS_SERVICE_NAME), QStringLiteral("/")};
        interface.isValid())
        return QDBusReply<QVector<RoomInfoItem>>{interface.call(QStringLiteral("rooms"))}.value();
    else
        return {};
}

QImage
image(const QString &mxcuri)
{
    if (QDBusInterface interface{QStringLiteral(NHEKO_DBUS_SERVICE_NAME), QStringLiteral("/")};
        interface.isValid())
        return QDBusReply<QImage>{interface.call(QStringLiteral("image"), mxcuri)}.value();
    else
        return {};
}

void
activateRoom(const QString &alias)
{
    if (QDBusInterface interface{QStringLiteral(NHEKO_DBUS_SERVICE_NAME), QStringLiteral("/")};
        interface.isValid())
        interface.call(QDBus::NoBlock, QStringLiteral("activateRoom"), alias);
}

void
joinRoom(const QString &alias)
{
    if (QDBusInterface interface{QStringLiteral(NHEKO_DBUS_SERVICE_NAME), QStringLiteral("/")};
        interface.isValid())
        interface.call(QDBus::NoBlock, QStringLiteral("joinRoom"), alias);
}

void
directChat(const QString &userId)
{
    if (QDBusInterface interface{QStringLiteral(NHEKO_DBUS_SERVICE_NAME), QStringLiteral("/")};
        interface.isValid())
        interface.call(QDBus::NoBlock, QStringLiteral("directChat"), userId);
}

void
setStatusMessage(const QString &message)
{
    if (QDBusInterface interface{QStringLiteral(NHEKO_DBUS_SERVICE_NAME), QStringLiteral("/")};
        interface.isValid())
        interface.call(QDBus::NoBlock, QStringLiteral("setStatusMessage"), message);
}

} // nheko::dbus

/**
 * Automatic marshaling of a QImage for org.freedesktop.Notifications.Notify
 *
 * This function is heavily based on a function from the Clementine project (see
 * http://www.clementine-player.org) and licensed under the GNU General Public
 * License, version 3 or later.
 *
 * SPDX-FileCopyrightText: 2010 David Sansome <me@davidsansome.com>
 */
QDBusArgument &
operator<<(QDBusArgument &arg, const QImage &image)
{
    if (image.isNull()) {
        arg.beginStructure();
        arg << 0 << 0 << 0 << false << 0 << 0 << QByteArray();
        arg.endStructure();
        return arg;
    }

    QImage i = image.height() > 100 || image.width() > 100
                 ? image.scaledToHeight(100, Qt::SmoothTransformation)
                 : image;
    i        = std::move(i).convertToFormat(QImage::Format_RGBA8888);

    arg.beginStructure();
    arg << i.width();
    arg << i.height();
    arg << i.bytesPerLine();
    arg << i.hasAlphaChannel();
    int channels = i.hasAlphaChannel() ? 4 : 3;
    arg << i.depth() / channels;
    arg << channels;
    arg << QByteArray(reinterpret_cast<const char *>(i.bits()), static_cast<int>(i.sizeInBytes()));
    arg.endStructure();

    return arg;
}

// This function, however, was merely reverse-engineered from the above function
// and is not from the Clementine project.
const QDBusArgument &
operator>>(const QDBusArgument &arg, QImage &image)
{
    // garbage is used as a sort of /dev/null
    int width, height, garbage;
    QByteArray bits;

    arg.beginStructure();
    arg >> width >> height >> garbage >> garbage >> garbage >> garbage >> bits;
    arg.endStructure();

    // Unfortunately, this copy-and-detach is necessary to ensure that the source buffer
    // is copied properly. If anybody finds a better solution, please implement it.
    auto temp =
      QImage(reinterpret_cast<uchar *>(bits.data()), width, height, QImage::Format_RGBA8888);
    image = temp;
    image.detach();

    return arg;
}
