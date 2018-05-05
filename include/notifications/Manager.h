#pragma once

#include <QImage>
#include <QString>

class NotificationsManager
{
public:
        static void postNotification(const QString &room,
                                     const QString &user,
                                     const QString &message);
};
