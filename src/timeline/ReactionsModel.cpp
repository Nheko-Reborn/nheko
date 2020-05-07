#include "ReactionsModel.h"

#include <Cache.h>
#include <MatrixClient.h>

QHash<int, QByteArray>
ReactionsModel::roleNames() const
{
        return {
          {Key, "key"},
          {Count, "counter"},
          {Users, "users"},
          {SelfReacted, "selfReacted"},
        };
}

int
ReactionsModel::rowCount(const QModelIndex &) const
{
        return static_cast<int>(reactions.size());
}

QVariant
ReactionsModel::data(const QModelIndex &index, int role) const
{
        const int i = index.row();
        if (i < 0 || i >= static_cast<int>(reactions.size()))
                return {};

        switch (role) {
        case Key:
                return QString::fromStdString(reactions[i].key);
        case Count:
                return static_cast<int>(reactions[i].reactions.size());
        case Users: {
                QString users;
                bool first = true;
                for (const auto &reaction : reactions[i].reactions) {
                        if (!first)
                                users += ", ";
                        else
                                first = false;
                        users += QString::fromStdString(
                          cache::displayName(room_id_, reaction.second.sender));
                }
                return users;
        }
        case SelfReacted:
                for (const auto &reaction : reactions[i].reactions)
                        if (reaction.second.sender == http::client()->user_id().to_string())
                                return true;
                return false;
        default:
                return {};
        }
}

void
ReactionsModel::addReaction(const std::string &room_id,
                            const mtx::events::RoomEvent<mtx::events::msg::Reaction> &reaction)
{
        room_id_ = room_id;

        int idx = 0;
        for (auto &storedReactions : reactions) {
                if (storedReactions.key == reaction.content.relates_to.key) {
                        storedReactions.reactions[reaction.event_id] = reaction;
                        emit dataChanged(index(idx, 0), index(idx, 0));
                        return;
                }
                idx++;
        }

        beginInsertRows(QModelIndex(), idx, idx);
        reactions.push_back(
          KeyReaction{reaction.content.relates_to.key, {{reaction.event_id, reaction}}});
        endInsertRows();
}

void
ReactionsModel::removeReaction(const mtx::events::RoomEvent<mtx::events::msg::Reaction> &reaction)
{
        int idx = 0;
        for (auto &storedReactions : reactions) {
                if (storedReactions.key == reaction.content.relates_to.key) {
                        storedReactions.reactions.erase(reaction.event_id);

                        if (storedReactions.reactions.size() == 0) {
                                beginRemoveRows(QModelIndex(), idx, idx);
                                reactions.erase(reactions.begin() + idx);
                                endRemoveRows();
                        } else
                                emit dataChanged(index(idx, 0), index(idx, 0));
                        return;
                }
                idx++;
        }
}
