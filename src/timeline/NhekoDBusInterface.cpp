// SPDX-FileCopyrightText: 2010 David Sansome <me@davidsansome.com>
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "NhekoDBusInterface.h"

#include <QDBusMetaType>

const QDBusArgument &
operator>>(const QDBusArgument &arg, QImage &image);
QDBusArgument &
operator<<(QDBusArgument &arg, const QImage &image);

RoomInfoItem::RoomInfoItem(const QString &mxid,
                           const QString &alias,
                           const QString &title,
                           const QImage &image,
                           QObject *parent)
  : QObject{parent}
  , roomId_{mxid}
  , alias_{alias}
  , roomName_{title}
  , image_{image}
{}

RoomInfoItem::RoomInfoItem(const RoomInfoItem &other)
  : QObject{other.parent()}
  , roomId_{other.roomId_}
  , alias_{other.alias_}
  , roomName_{other.roomName_}
  , image_{other.image_}
{}

void
RoomInfoItem::init()
{
    qDBusRegisterMetaType<RoomInfoItem>();
    qDBusRegisterMetaType<QVector<RoomInfoItem>>();
    qDBusRegisterMetaType<QImage>();
}

RoomInfoItem &
RoomInfoItem::operator=(const RoomInfoItem &other)
{
    roomId_   = other.roomId_;
    alias_    = other.alias_;
    roomName_ = other.roomName_;
    image_    = other.image_;
    return *this;
}

QDBusArgument &
operator<<(QDBusArgument &arg, const RoomInfoItem &item)
{
    arg.beginStructure();
    arg << item.roomId_ << item.alias_ << item.roomName_ << item.image_;
    arg.endStructure();
    return arg;
}

const QDBusArgument &
operator>>(const QDBusArgument &arg, RoomInfoItem &item)
{
    arg.beginStructure();
    arg >> item.roomId_ >> item.alias_ >> item.roomName_ >> item.image_;
    if (item.image_.isNull())
        item.image_ = QIcon::fromTheme(QStringLiteral("group")).pixmap(32, 32).toImage();

    arg.endStructure();
    return arg;
}

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
    int channels = i.isGrayscale() ? 1 : (i.hasAlphaChannel() ? 4 : 3);
    arg << i.depth() / channels;
    arg << channels;
    arg << QByteArray(reinterpret_cast<const char *>(i.bits()), i.sizeInBytes());
    arg.endStructure();

    return arg;
}

// This function, however, was merely reverse-engineered from the above function
// and is not from the Clementine project (except for the byte-order block, which
// was more or less copied from the above function.
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
