#include "include/Community.h"

#include <QJsonArray>
#include <QJsonValue>

void
Community::parseProfile(const QJsonObject &profile)
{
        if (profile["name"].type() == QJsonValue::Type::String)
                name_ = profile["name"].toString();
        else
                name_ = "Unnamed Community"; // TODO: what is correct here?

        if (profile["avatar_url"].type() == QJsonValue::Type::String)
                avatar_ = QUrl(profile["avatar_url"].toString());

        if (profile["short_description"].type() == QJsonValue::Type::String)
                short_description_ = profile["short_description"].toString();

        if (profile["long_description"].type() == QJsonValue::Type::String)
                long_description_ = profile["long_description"].toString();
}

void
Community::parseRooms(const QJsonObject &rooms)
{
        rooms_.clear();

        for (auto const &room : rooms["chunk"].toArray()) {
                if (room.toObject().contains("room_id"))
                        rooms_.emplace_back(room.toObject()["room_id"].toString());
        }
}
