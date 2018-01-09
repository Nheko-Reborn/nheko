#pragma once

#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QUrl>

class Community : public QObject
{
        Q_OBJECT

public:
        void parseProfile(const QJsonObject &profile);
        void parseRooms(const QJsonObject &rooms);

        inline QUrl getAvatar() const;
        inline QString getName() const;
        inline QString getShortDescription() const;
        inline QString getLongDescription() const;
        inline const QList<QString> getRoomList() const;

signals:
        void roomsChanged(QList<QString> &rooms);

private:
        QUrl avatar_;
        QString name_;
        QString short_description_;
        QString long_description_;

        QList<QString> rooms_;
};

inline QUrl
Community::getAvatar() const
{
        return avatar_;
}

inline QString
Community::getName() const
{
        return name_;
}

inline QString
Community::getShortDescription() const
{
        return short_description_;
}

inline QString
Community::getLongDescription() const
{
        return long_description_;
}

inline const QList<QString>
Community::getRoomList() const
{
        return rooms_;
}
