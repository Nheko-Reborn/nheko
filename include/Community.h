#pragma once

#include <QJsonObject>
#include <QString>
#include <QUrl>
#include <vector>

struct Community
{
        void parseProfile(const QJsonObject &profile);
        void parseRooms(const QJsonObject &rooms);

        QUrl getAvatar() const { return avatar_; }
        QString getName() const { return name_; }
        QString getShortDescription() const { return short_description_; }
        QString getLongDescription() const { return long_description_; }
        std::vector<QString> getRoomList() const { return rooms_; }

        QUrl avatar_;
        QString name_;
        QString short_description_;
        QString long_description_;

        std::vector<QString> rooms_;
};
