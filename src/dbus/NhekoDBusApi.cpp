// SPDX-FileCopyrightText: 2010 David Sansome <me@davidsansome.com>
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "NhekoDBusApi.h"

#include <QDBusMetaType>

namespace nheko::dbus {
void
init()
{
    qDBusRegisterMetaType<RoomInfoItem>();
    qDBusRegisterMetaType<QVector<RoomInfoItem>>();
    qDBusRegisterMetaType<QImage>();
    qDBusRegisterMetaType<QVersionNumber>();
}

bool
apiVersionIsCompatible(const QVersionNumber &clientAppVersion)
{
    if (clientAppVersion.majorVersion() != nheko::dbus::apiVersion.majorVersion())
        return false;
    if (clientAppVersion.minorVersion() > nheko::dbus::apiVersion.minorVersion())
        return false;
    if (clientAppVersion.minorVersion() == nheko::dbus::apiVersion.minorVersion() &&
        clientAppVersion.microVersion() < nheko::dbus::apiVersion.microVersion())
        return false;

    return true;
}

RoomInfoItem::RoomInfoItem(const QString &roomId,
                           const QString &alias,
                           const QString &title,
                           const QImage &image,
                           const int unreadNotifications,
                           QObject *parent)
  : QObject{parent}
  , roomId_{roomId}
  , alias_{alias}
  , roomName_{title}
  , image_{image}
  , unreadNotifications_{unreadNotifications}
{}

RoomInfoItem::RoomInfoItem(const RoomInfoItem &other)
  : QObject{other.parent()}
  , roomId_{other.roomId_}
  , alias_{other.alias_}
  , roomName_{other.roomName_}
  , image_{other.image_}
  , unreadNotifications_{other.unreadNotifications_}
{}

RoomInfoItem &
RoomInfoItem::operator=(const RoomInfoItem &other)
{
    roomId_              = other.roomId_;
    alias_               = other.alias_;
    roomName_            = other.roomName_;
    image_               = other.image_;
    unreadNotifications_ = other.unreadNotifications_;
    return *this;
}

QDBusArgument &
operator<<(QDBusArgument &arg, const RoomInfoItem &item)
{
    arg.beginStructure();
    arg << item.roomId_ << item.alias_ << item.roomName_ << item.image_
        << item.unreadNotifications_;
    arg.endStructure();
    return arg;
}

const QDBusArgument &
operator>>(const QDBusArgument &arg, RoomInfoItem &item)
{
    arg.beginStructure();
    arg >> item.roomId_ >> item.alias_ >> item.roomName_ >> item.image_ >>
      item.unreadNotifications_;
    if (item.image_.isNull())
        item.image_ = QImage{QStringLiteral(":/icons/ui/speech-bubbles.svg")};

    arg.endStructure();
    return arg;
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
    arg << QByteArray(reinterpret_cast<const char *>(i.bits()), i.sizeInBytes());
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

    image = QImage(reinterpret_cast<uchar *>(bits.data()), width, height, QImage::Format_RGBA8888);

    return arg;
}

QDBusArgument &
operator<<(QDBusArgument &arg, const QVersionNumber &v)
{
    arg.beginStructure();
    arg << v.toString();
    arg.endStructure();
    return arg;
}

const QDBusArgument &
operator>>(const QDBusArgument &arg, QVersionNumber &v)
{
    arg.beginStructure();
    QString temp;
    arg >> temp;
    v = QVersionNumber::fromString(temp);
    arg.endStructure();
    return arg;
}
