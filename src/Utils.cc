#include "Utils.h"
#include "timeline/TimelineViewManager.h"

#include <variant.hpp>

using TimelineEvent = mtx::events::collections::TimelineEvents;

QString
utils::descriptiveTime(const QDateTime &then)
{
        const auto now  = QDateTime::currentDateTime();
        const auto days = then.daysTo(now);

        if (days == 0)
                return then.toString("HH:mm");
        else if (days < 2)
                return QString("Yesterday");
        else if (days < 365)
                return then.toString("dd/MM");

        return then.toString("dd/MM/yy");
}

DescInfo
utils::getMessageDescription(const TimelineEvent &event, const QString &localUser)
{
        using Audio  = mtx::events::RoomEvent<mtx::events::msg::Audio>;
        using Emote  = mtx::events::RoomEvent<mtx::events::msg::Emote>;
        using File   = mtx::events::RoomEvent<mtx::events::msg::File>;
        using Image  = mtx::events::RoomEvent<mtx::events::msg::Image>;
        using Notice = mtx::events::RoomEvent<mtx::events::msg::Notice>;
        using Text   = mtx::events::RoomEvent<mtx::events::msg::Text>;
        using Video  = mtx::events::RoomEvent<mtx::events::msg::Video>;

        if (mpark::holds_alternative<Audio>(event)) {
                const auto msg = mpark::get<Audio>(event);
                QString sender = QString::fromStdString(msg.sender);

                const auto username = TimelineViewManager::displayName(sender);
                const auto ts       = QDateTime::fromMSecsSinceEpoch(msg.origin_server_ts);

                return DescInfo{sender == localUser ? "You" : username,
                                sender,
                                " sent an audio clip",
                                utils::descriptiveTime(ts),
                                ts};
        } else if (mpark::holds_alternative<Emote>(event)) {
                auto msg       = mpark::get<Emote>(event);
                QString sender = QString::fromStdString(msg.sender);

                const auto username = TimelineViewManager::displayName(sender);
                const auto ts       = QDateTime::fromMSecsSinceEpoch(msg.origin_server_ts);
                const auto body     = QString::fromStdString(msg.content.body).trimmed();

                return DescInfo{"",
                                sender,
                                QString("* %1 %2").arg(username).arg(body),
                                utils::descriptiveTime(ts),
                                ts};
        } else if (mpark::holds_alternative<File>(event)) {
                const auto msg = mpark::get<File>(event);
                QString sender = QString::fromStdString(msg.sender);

                const auto username = TimelineViewManager::displayName(sender);
                const auto ts       = QDateTime::fromMSecsSinceEpoch(msg.origin_server_ts);

                return DescInfo{sender == localUser ? "You" : username,
                                sender,
                                " sent a file",
                                utils::descriptiveTime(ts),
                                ts};
        } else if (mpark::holds_alternative<Image>(event)) {
                const auto msg = mpark::get<Image>(event);
                QString sender = QString::fromStdString(msg.sender);

                const auto username = TimelineViewManager::displayName(sender);
                const auto ts       = QDateTime::fromMSecsSinceEpoch(msg.origin_server_ts);

                return DescInfo{sender == localUser ? "You" : username,
                                sender,
                                " sent an image",
                                utils::descriptiveTime(ts),
                                ts};
        } else if (mpark::holds_alternative<Notice>(event)) {
                const auto msg = mpark::get<Notice>(event);
                QString sender = QString::fromStdString(msg.sender);

                const auto username = TimelineViewManager::displayName(sender);
                const auto ts       = QDateTime::fromMSecsSinceEpoch(msg.origin_server_ts);

                return DescInfo{
                  username, sender, " sent a notification", utils::descriptiveTime(ts), ts};
        } else if (mpark::holds_alternative<Text>(event)) {
                const auto msg = mpark::get<Text>(event);
                QString sender = QString::fromStdString(msg.sender);

                const auto username = TimelineViewManager::displayName(sender);
                const auto ts       = QDateTime::fromMSecsSinceEpoch(msg.origin_server_ts);
                const auto body     = QString::fromStdString(msg.content.body).trimmed();

                return DescInfo{sender == localUser ? "You" : username,
                                sender,
                                QString(": %1").arg(body),
                                utils::descriptiveTime(ts),
                                ts};
        } else if (mpark::holds_alternative<Video>(event)) {
                const auto msg = mpark::get<Video>(event);
                QString sender = QString::fromStdString(msg.sender);

                const auto username = TimelineViewManager::displayName(sender);
                const auto ts       = QDateTime::fromMSecsSinceEpoch(msg.origin_server_ts);

                return DescInfo{sender == localUser ? "You" : username,
                                sender,
                                " sent a video clip",
                                utils::descriptiveTime(ts),
                                ts};
        }

        return DescInfo{};
}
