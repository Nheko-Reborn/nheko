#ifndef NHEKODBUSINTERFACE_H
#define NHEKODBUSINTERFACE_H

#include <QDBusArgument>
#include <QIcon>
#include <QObject>

class RoomInfoItem : public QObject
{
    Q_OBJECT

public:
    RoomInfoItem(const QString &mxid  = QString{},
                 const QString &alias = QString{},
                 const QString &title = QString{},
                 const QIcon &icon    = QIcon{},
                 QObject *parent      = nullptr);

    RoomInfoItem(const RoomInfoItem &other);

    const QString &roomId() const { return roomId_; }
    const QString &alias() const { return alias_; }
    const QString &roomName() const { return roomName_; }
    const QIcon &icon() const { return icon_; }

    //! Registers all necessary classes with D-Bus. Call this before using RoomInfoItem with D-Bus.
    static void init();

    RoomInfoItem &operator=(const RoomInfoItem &other);
    friend QDBusArgument &operator<<(QDBusArgument &arg, const RoomInfoItem &item);
    friend const QDBusArgument &operator>>(const QDBusArgument &arg, RoomInfoItem &item);

private:
    QString roomId_;
    QString alias_;
    QString roomName_;
    QIcon icon_;
};
Q_DECLARE_METATYPE(RoomInfoItem)

QDBusArgument &
operator<<(QDBusArgument &arg, const RoomInfoItem &item);
const QDBusArgument &
operator>>(const QDBusArgument &arg, RoomInfoItem &item);

QDBusArgument &
operator<<(QDBusArgument &arg, const QImage &image);
const QDBusArgument &
operator>>(const QDBusArgument &arg, QImage &);

#define NHEKO_DBUS_SERVICE_NAME "io.github.Nheko-Reborn.nheko"

#endif // NHEKODBUSINTERFACE_H
