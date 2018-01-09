#include "include/Community.h"

#include <QJsonArray>
#include <QJsonValue>

void
Community::parseProfile(const QJsonObject &profile)
{
        if (profile["name"].type() == QJsonValue::Type::String) {
                name_ = profile["name"].toString();
        } else {
                name_ = "Unnamed Community"; // TODO: what is correct here?
        }

        if (profile["avatar_url"].type() == QJsonValue::Type::String) {
                avatar_ = QUrl(profile["avatar_url"].toString());
        } else {
                avatar_ = QUrl();
        }

        if (profile["short_description"].type() == QJsonValue::Type::String) {
                short_description_ = profile["short_description"].toString();
        } else {
                short_description_ = "";
        }

        if (profile["long_description"].type() == QJsonValue::Type::String) {
                long_description_ = profile["long_description"].toString();
        } else {
                long_description_ = "";
        }
}

void
Community::parseRooms(const QJsonObject &rooms)
{
        rooms_.clear();

        for (auto i = 0; i < rooms["chunk"].toArray().size(); i++) {
                rooms_.append(rooms["chunk"].toArray()[i].toObject()["room_id"].toString());
        }

        emit roomsChanged(rooms_);
}
