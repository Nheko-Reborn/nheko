// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "CommunitiesModel.h"

#include <mtx/responses/common.hpp>
#include <set>

#include "Cache.h"
#include "Cache_p.h"
#include "ChatPage.h"
#include "Logging.h"
#include "MatrixClient.h"
#include "Permissions.h"
#include "UserSettingsPage.h"
#include "Utils.h"
#include "timeline/TimelineModel.h"

Q_DECLARE_METATYPE(SpaceItem)

CommunitiesModel::CommunitiesModel(QObject *parent)
  : QAbstractListModel(parent)
  , hiddenTagIds_{UserSettings::instance()->hiddenTags()}
  , mutedTagIds_{UserSettings::instance()->mutedTags()}
{
    static auto ignore = qRegisterMetaType<SpaceItem>();
    (void)ignore;
}

QHash<int, QByteArray>
CommunitiesModel::roleNames() const
{
    return {
      {AvatarUrl, "avatarUrl"},
      {DisplayName, "displayName"},
      {Tooltip, "tooltip"},
      {Collapsed, "collapsed"},
      {Collapsible, "collapsible"},
      {Hidden, "hidden"},
      {Depth, "depth"},
      {Id, "id"},
      {UnreadMessages, "unreadMessages"},
      {HasLoudNotification, "hasLoudNotification"},
      {Muted, "muted"},
    };
}

bool
CommunitiesModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != CommunitiesModel::Collapsed)
        return false;
    else if (index.row() >= 2 || index.row() - 2 < spaceOrder_.size()) {
        spaceOrder_.tree.at(index.row() - 2).collapsed = value.toBool();

        const auto cindex = spaceOrder_.lastChild(index.row() - 2);
        emit dataChanged(index, this->index(cindex + 2), {Collapsed, Qt::DisplayRole});
        spaceOrder_.storeCollapsed();
        return true;
    } else
        return false;
}

QVariant
CommunitiesModel::data(const QModelIndex &index, int role) const
{
    if (role == CommunitiesModel::Roles::Muted) {
        if (index.row() == 0)
            return mutedTagIds_.contains(QStringLiteral("global"));
        else
            return mutedTagIds_.contains(data(index, CommunitiesModel::Roles::Id).toString());
    }

    if (index.row() == 0) {
        switch (role) {
        case CommunitiesModel::Roles::AvatarUrl:
            return QStringLiteral(":/icons/icons/ui/world.svg");
        case CommunitiesModel::Roles::DisplayName:
            return tr("All rooms");
        case CommunitiesModel::Roles::Tooltip:
            return tr("Shows all rooms without filtering.");
        case CommunitiesModel::Roles::Collapsed:
            return false;
        case CommunitiesModel::Roles::Collapsible:
            return false;
        case CommunitiesModel::Roles::Hidden:
            return false;
        case CommunitiesModel::Roles::Parent:
            return "";
        case CommunitiesModel::Roles::Depth:
            return 0;
        case CommunitiesModel::Roles::Id:
            return "";
        case CommunitiesModel::Roles::UnreadMessages:
            return (int)globalUnreads.notification_count;
        case CommunitiesModel::Roles::HasLoudNotification:
            return globalUnreads.highlight_count > 0;
        }
    } else if (index.row() == 1) {
        switch (role) {
        case CommunitiesModel::Roles::AvatarUrl:
            return QStringLiteral(":/icons/icons/ui/people.svg");
        case CommunitiesModel::Roles::DisplayName:
            return tr("Direct Chats");
        case CommunitiesModel::Roles::Tooltip:
            return tr("Show direct chats.");
        case CommunitiesModel::Roles::Collapsed:
            return false;
        case CommunitiesModel::Roles::Collapsible:
            return false;
        case CommunitiesModel::Roles::Hidden:
            return hiddenTagIds_.contains(QStringLiteral("dm"));
        case CommunitiesModel::Roles::Parent:
            return "";
        case CommunitiesModel::Roles::Depth:
            return 0;
        case CommunitiesModel::Roles::Id:
            return "dm";
        case CommunitiesModel::Roles::UnreadMessages:
            return (int)dmUnreads.notification_count;
        case CommunitiesModel::Roles::HasLoudNotification:
            return dmUnreads.highlight_count > 0;
        }
    } else if (index.row() - 2 < spaceOrder_.size()) {
        auto id = spaceOrder_.tree.at(index.row() - 2).id;
        switch (role) {
        case CommunitiesModel::Roles::AvatarUrl:
            return QString::fromStdString(spaces_.at(id).avatar_url);
        case CommunitiesModel::Roles::DisplayName:
        case CommunitiesModel::Roles::Tooltip:
            return QString::fromStdString(spaces_.at(id).name);
        case CommunitiesModel::Roles::Collapsed:
            return spaceOrder_.tree.at(index.row() - 2).collapsed;
        case CommunitiesModel::Roles::Collapsible: {
            auto idx = index.row() - 2;
            return idx != spaceOrder_.lastChild(idx);
        }
        case CommunitiesModel::Roles::Hidden:
            return hiddenTagIds_.contains("space:" + id);
        case CommunitiesModel::Roles::Parent: {
            if (auto p = spaceOrder_.parent(index.row() - 2); p >= 0)
                return spaceOrder_.tree[p].id;

            return "";
        }
        case CommunitiesModel::Roles::Depth:
            return spaceOrder_.tree.at(index.row() - 2).depth;
        case CommunitiesModel::Roles::Id:
            return "space:" + id;
        case CommunitiesModel::Roles::UnreadMessages: {
            int count = 0;
            auto end  = spaceOrder_.lastChild(index.row() - 2);
            for (int i = index.row() - 2; i <= end; i++)
                count += spaceOrder_.tree[i].notificationCounts.notification_count;
            return count;
        }
        case CommunitiesModel::Roles::HasLoudNotification: {
            auto end = spaceOrder_.lastChild(index.row() - 2);
            for (int i = index.row() - 2; i <= end; i++)
                if (spaceOrder_.tree[i].notificationCounts.highlight_count > 0)
                    return true;
            return false;
        }
        }
    } else if (index.row() - 2 < tags_.size() + spaceOrder_.size()) {
        auto tag = tags_.at(index.row() - 2 - spaceOrder_.size());
        if (tag == QLatin1String("m.favourite")) {
            switch (role) {
            case CommunitiesModel::Roles::AvatarUrl:
                return QStringLiteral(":/icons/icons/ui/star.svg");
            case CommunitiesModel::Roles::DisplayName:
                return tr("Favourites");
            case CommunitiesModel::Roles::Tooltip:
                return tr("Rooms you have favourited.");
            }
        } else if (tag == QLatin1String("m.lowpriority")) {
            switch (role) {
            case CommunitiesModel::Roles::AvatarUrl:
                return QStringLiteral(":/icons/icons/ui/lowprio.svg");
            case CommunitiesModel::Roles::DisplayName:
                return tr("Low Priority");
            case CommunitiesModel::Roles::Tooltip:
                return tr("Rooms with low priority.");
            }
        } else if (tag == QLatin1String("m.server_notice")) {
            switch (role) {
            case CommunitiesModel::Roles::AvatarUrl:
                return QStringLiteral(":/icons/icons/ui/tag.svg");
            case CommunitiesModel::Roles::DisplayName:
                return tr("Server Notices");
            case CommunitiesModel::Roles::Tooltip:
                return tr("Messages from your server or administrator.");
            }
        } else {
            switch (role) {
            case CommunitiesModel::Roles::AvatarUrl:
                return QStringLiteral(":/icons/icons/ui/tag.svg");
            case CommunitiesModel::Roles::DisplayName:
            case CommunitiesModel::Roles::Tooltip:
                return tag.mid(2);
            }
        }

        switch (role) {
        case CommunitiesModel::Roles::Hidden:
            return hiddenTagIds_.contains("tag:" + tag);
        case CommunitiesModel::Roles::Collapsed:
            return true;
        case CommunitiesModel::Roles::Collapsible:
            return false;
        case CommunitiesModel::Roles::Parent:
            return "";
        case CommunitiesModel::Roles::Depth:
            return 0;
        case CommunitiesModel::Roles::Id:
            return "tag:" + tag;
        case CommunitiesModel::Roles::UnreadMessages:
            if (auto it = tagNotificationCache.find(tag); it != tagNotificationCache.end())
                return (int)it->second.notification_count;
            else
                return 0;
        case CommunitiesModel::Roles::HasLoudNotification:
            if (auto it = tagNotificationCache.find(tag); it != tagNotificationCache.end())
                return it->second.highlight_count > 0;
            else
                return 0;
        }
    }
    return QVariant();
}

namespace {
struct temptree
{
    std::map<std::string, temptree> children;

    void insert(const std::vector<std::string> &parents, const std::string &child)
    {
        temptree *t = this;
        for (const auto &e : parents)
            t = &t->children[e];
        t->children[child];
    }

    void flatten(CommunitiesModel::FlatTree &to, int i = 0) const
    {
        for (const auto &[child, subtree] : children) {
            to.tree.push_back({QString::fromStdString(child), i, {}, false});
            subtree.flatten(to, i + 1);
        }
    }
};

void
addChildren(temptree &t,
            std::vector<std::string> path,
            std::string child,
            const std::map<std::string, std::set<std::string>> &children)
{
    if (std::find(path.begin(), path.end(), child) != path.end())
        return;

    path.push_back(child);

    if (children.count(child)) {
        for (const auto &c : children.at(child)) {
            t.insert(path, c);
            addChildren(t, path, c, children);
        }
    }
}
}

void
CommunitiesModel::initializeSidebar()
{
    beginResetModel();
    tags_.clear();
    spaceOrder_.tree.clear();
    spaces_.clear();
    tagNotificationCache.clear();
    globalUnreads.notification_count = {};
    dmUnreads.notification_count     = {};

    {
        auto e = cache::client()->getAccountData(mtx::events::EventType::Direct);
        if (e) {
            if (auto event =
                  std::get_if<mtx::events::AccountDataEvent<mtx::events::account_data::Direct>>(
                    &e.value())) {
                directMessages_.clear();
                for (const auto &[userId, roomIds] : event->content.user_to_rooms)
                    for (const auto &roomId : roomIds)
                        directMessages_.push_back(roomId);
            }
        }
    }

    std::set<std::string> ts;

    std::set<std::string> isSpace;
    std::map<std::string, std::set<std::string>> spaceChilds;
    std::map<std::string, std::set<std::string>> spaceParents;

    auto infos = cache::roomInfo();
    for (auto it = infos.begin(); it != infos.end(); ++it) {
        if (it.value().is_space) {
            spaces_[it.key()] = it.value();
            isSpace.insert(it.key().toStdString());
        } else {
            for (const auto &t : it.value().tags) {
                if (t.find("u.") == 0 || t.find("m." == 0)) {
                    ts.insert(t);
                }
            }
        }

        for (const auto &t : it->tags) {
            auto tagId = QString::fromStdString(t);
            auto &tNs  = tagNotificationCache[tagId];
            tNs.notification_count += it->notification_count;
            tNs.highlight_count += it->highlight_count;
        }

        auto &e              = roomNotificationCache[it.key()];
        e.highlight_count    = it->highlight_count;
        e.notification_count = it->notification_count;
        globalUnreads.notification_count += it->notification_count;
        globalUnreads.highlight_count += it->highlight_count;

        if (std::find(begin(directMessages_), end(directMessages_), it.key().toStdString()) !=
            end(directMessages_)) {
            dmUnreads.notification_count += it->notification_count;
            dmUnreads.highlight_count += it->highlight_count;
        }
    }

    // NOTE(Nico): We build a forrest from the Directed Cyclic(!) Graph of spaces. To do that we
    // start with orphan spaces at the top. This leaves out some space circles, but there is no good
    // way to break that cycle imo anyway. Then we carefully walk a tree down from each root in our
    // forrest, carefully checking not to run in a circle and get lost forever.
    // TODO(Nico): Optimize this. We can do this with a lot fewer allocations and checks.
    for (const auto &space : isSpace) {
        spaceParents[space];
        for (const auto &p : cache::client()->getParentRoomIds(space)) {
            spaceParents[space].insert(p);
            spaceChilds[p].insert(space);
        }
    }

    temptree spacetree;
    std::vector<std::string> path;
    for (const auto &space : isSpace) {
        if (!spaceParents[space].empty())
            continue;

        spacetree.children[space] = {};
    }
    for (const auto &space : spacetree.children) {
        addChildren(spacetree, path, space.first, spaceChilds);
    }

    // NOTE(Nico): This flattens the tree into a list, preserving the depth at each element.
    spacetree.flatten(spaceOrder_);

    for (const auto &t : ts)
        tags_.push_back(QString::fromStdString(t));

    spaceOrder_.restoreCollapsed();

    for (auto &space : spaceOrder_.tree) {
        for (const auto &c : cache::client()->getChildRoomIds(space.id.toStdString())) {
            const auto &counts = roomNotificationCache[QString::fromStdString(c)];
            space.notificationCounts.highlight_count += counts.highlight_count;
            space.notificationCounts.notification_count += counts.notification_count;
        }
    }

    endResetModel();

    emit tagsChanged();
    emit hiddenTagsChanged();
    emit containsSubspacesChanged();
}

void
CommunitiesModel::FlatTree::storeCollapsed()
{
    QList<QStringList> elements;

    int depth = -1;

    QStringList current;
    elements.reserve(static_cast<int>(tree.size()));

    for (const auto &e : tree) {
        if (e.depth > depth) {
            current.push_back(e.id);
        } else if (e.depth == depth) {
            current.back() = e.id;
        } else {
            current.pop_back();
            current.back() = e.id;
        }

        if (e.collapsed)
            elements.push_back(current);
    }

    UserSettings::instance()->setCollapsedSpaces(elements);
}
void
CommunitiesModel::FlatTree::restoreCollapsed()
{
    QList<QStringList> elements = UserSettings::instance()->collapsedSpaces();

    int depth = -1;

    QStringList current;

    for (auto &e : tree) {
        if (e.depth > depth) {
            current.push_back(e.id);
        } else if (e.depth == depth) {
            current.back() = e.id;
        } else {
            current.pop_back();
            current.back() = e.id;
        }

        if (elements.contains(current))
            e.collapsed = true;
    }
}

void
CommunitiesModel::clear()
{
    beginResetModel();
    tags_.clear();
    endResetModel();
    resetCurrentTagId();

    emit tagsChanged();
}

void
CommunitiesModel::sync(const mtx::responses::Sync &sync_)
{
    bool tagsUpdated  = false;
    const auto userid = http::client()->user_id().to_string();

    for (const auto &[roomid, room] : sync_.rooms.join) {
        for (const auto &e : room.account_data.events)
            if (std::holds_alternative<
                  mtx::events::AccountDataEvent<mtx::events::account_data::Tags>>(e)) {
                tagsUpdated = true;
            }
        for (const auto &e : room.state.events) {
            if (std::holds_alternative<mtx::events::StateEvent<mtx::events::state::space::Child>>(
                  e) ||
                std::holds_alternative<mtx::events::StateEvent<mtx::events::state::space::Parent>>(
                  e))
                tagsUpdated = true;

            if (auto ev = std::get_if<mtx::events::StateEvent<mtx::events::state::Member>>(&e);
                ev && ev->state_key == userid)
                tagsUpdated = true;
        }
        for (const auto &e : room.timeline.events) {
            if (std::holds_alternative<mtx::events::StateEvent<mtx::events::state::space::Child>>(
                  e) ||
                std::holds_alternative<mtx::events::StateEvent<mtx::events::state::space::Parent>>(
                  e))
                tagsUpdated = true;

            if (auto ev = std::get_if<mtx::events::StateEvent<mtx::events::state::Member>>(&e);
                ev && ev->state_key == userid)
                tagsUpdated = true;
        }

        auto roomId            = QString::fromStdString(roomid);
        auto &oldUnreads       = roomNotificationCache[roomId];
        auto notificationCDiff = -static_cast<int64_t>(oldUnreads.notification_count) +
                                 static_cast<int64_t>(room.unread_notifications.notification_count);
        auto highlightCDiff = -static_cast<int64_t>(oldUnreads.highlight_count) +
                              static_cast<int64_t>(room.unread_notifications.highlight_count);

        auto applyDiff = [notificationCDiff,
                          highlightCDiff](mtx::responses::UnreadNotifications &n) {
            n.highlight_count    = static_cast<int64_t>(n.highlight_count) + highlightCDiff;
            n.notification_count = static_cast<int64_t>(n.notification_count) + notificationCDiff;
        };
        if (highlightCDiff || notificationCDiff) {
            // bool hidden = hiddenTagIds_.contains(roomId);
            applyDiff(globalUnreads);
            emit dataChanged(index(0),
                             index(0),
                             {
                               UnreadMessages,
                               HasLoudNotification,
                             });
            if (std::find(begin(directMessages_), end(directMessages_), roomid) !=
                end(directMessages_)) {
                applyDiff(dmUnreads);
                emit dataChanged(index(1),
                                 index(1),
                                 {
                                   UnreadMessages,
                                   HasLoudNotification,
                                 });
            }

            auto spaces = cache::client()->getParentRoomIds(roomid);
            auto tags   = cache::singleRoomInfo(roomid).tags;

            for (const auto &t : tags) {
                auto tagId = QString::fromStdString(t);
                applyDiff(tagNotificationCache[tagId]);
                int idx = tags_.indexOf(tagId) + 2 + spaceOrder_.size();
                emit dataChanged(index(idx),
                                 index(idx),
                                 {
                                   UnreadMessages,
                                   HasLoudNotification,
                                 });
            }

            for (const auto &s : spaces) {
                auto spaceId = QString::fromStdString(s);

                for (int i = 0; i < spaceOrder_.size(); i++) {
                    if (spaceOrder_.tree[i].id != spaceId)
                        continue;

                    applyDiff(spaceOrder_.tree[i].notificationCounts);

                    int idx = i;
                    do {
                        emit dataChanged(index(idx + 2),
                                         index(idx + 2),
                                         {
                                           UnreadMessages,
                                           HasLoudNotification,
                                         });
                        idx = spaceOrder_.parent(idx);
                    } while (idx != -1);
                }
            }
        }

        roomNotificationCache[roomId] = room.unread_notifications;
    }
    for (const auto &[roomid, room] : sync_.rooms.leave) {
        (void)room;
        if (spaces_.count(QString::fromStdString(roomid)))
            tagsUpdated = true;
    }
    for (const auto &e : sync_.account_data.events) {
        if (auto event =
              std::get_if<mtx::events::AccountDataEvent<mtx::events::account_data::Direct>>(&e)) {
            directMessages_.clear();
            for (const auto &[userId, roomIds] : event->content.user_to_rooms)
                for (const auto &roomId : roomIds)
                    directMessages_.push_back(roomId);
            tagsUpdated = true;
            break;
        }
    }

    if (tagsUpdated)
        initializeSidebar();
}

void
CommunitiesModel::setCurrentTagId(const QString &tagId)
{
    if (tagId.startsWith(QLatin1String("tag:"))) {
        auto tag = tagId.mid(4);
        for (const auto &t : qAsConst(tags_)) {
            if (t == tag) {
                this->currentTagId_ = tagId;
                emit currentTagIdChanged(currentTagId_);
                return;
            }
        }
    } else if (tagId.startsWith(QLatin1String("space:"))) {
        auto tag = tagId.mid(6);
        for (const auto &t : spaceOrder_.tree) {
            if (t.id == tag) {
                this->currentTagId_ = tagId;
                emit currentTagIdChanged(currentTagId_);
                return;
            }
        }
    } else if (tagId == QLatin1String("dm")) {
        this->currentTagId_ = tagId;
        emit currentTagIdChanged(currentTagId_);
        return;
    }

    this->currentTagId_ = QLatin1String("");
    emit currentTagIdChanged(currentTagId_);
}

void
CommunitiesModel::toggleTagId(QString tagId)
{
    if (hiddenTagIds_.contains(tagId))
        hiddenTagIds_.removeOne(tagId);
    else
        hiddenTagIds_.push_back(tagId);
    UserSettings::instance()->setHiddenTags(hiddenTagIds_);

    if (tagId.startsWith(QLatin1String("tag:"))) {
        auto idx = tags_.indexOf(tagId.mid(4));
        if (idx != -1)
            emit dataChanged(
              index(idx + 1 + spaceOrder_.size()), index(idx + 1 + spaceOrder_.size()), {Hidden});
    } else if (tagId.startsWith(QLatin1String("space:"))) {
        auto idx = spaceOrder_.indexOf(tagId.mid(6));
        if (idx != -1)
            emit dataChanged(index(idx + 1), index(idx + 1), {Hidden});
    } else if (tagId == QLatin1String("dm")) {
        emit dataChanged(index(1), index(1), {Hidden});
    }

    emit hiddenTagsChanged();
}

void
CommunitiesModel::toggleTagMute(QString tagId)
{
    if (tagId.isEmpty())
        tagId = QStringLiteral("global");

    if (mutedTagIds_.contains(tagId))
        mutedTagIds_.removeOne(tagId);
    else
        mutedTagIds_.push_back(tagId);
    UserSettings::instance()->setMutedTags(mutedTagIds_);

    if (tagId.startsWith(QLatin1String("tag:"))) {
        auto idx = tags_.indexOf(tagId.mid(4));
        if (idx != -1)
            emit dataChanged(index(idx + 2 + spaceOrder_.size()),
                             index(idx + 2 + spaceOrder_.size()));
    } else if (tagId.startsWith(QLatin1String("space:"))) {
        auto idx = spaceOrder_.indexOf(tagId.mid(6));
        if (idx != -1)
            emit dataChanged(index(idx + 2), index(idx + 2));
    } else if (tagId == QLatin1String("dm")) {
        emit dataChanged(index(1), index(1));
    } else if (tagId == QLatin1String("global")) {
        emit dataChanged(index(0), index(0));
    }
}

FilteredCommunitiesModel::FilteredCommunitiesModel(CommunitiesModel *model, QObject *parent)
  : QSortFilterProxyModel(parent)
{
    setSourceModel(model);
    setDynamicSortFilter(true);
    sort(0);
}

namespace {
enum Categories
{
    World,
    Direct,
    Favourites,
    Server,
    LowPrio,
    Space,
    UserTag,
};

Categories
tagIdToCat(QString tagId)
{
    if (tagId.isEmpty())
        return World;
    else if (tagId == QLatin1String("dm"))
        return Direct;
    else if (tagId == QLatin1String("tag:m.favourite"))
        return Favourites;
    else if (tagId == QLatin1String("tag:m.server_notice"))
        return Server;
    else if (tagId == QLatin1String("tag:m.lowpriority"))
        return LowPrio;
    else if (tagId.startsWith(QLatin1String("space:")))
        return Space;
    else
        return UserTag;
}
}

bool
FilteredCommunitiesModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    QModelIndex const left_idx  = sourceModel()->index(left.row(), 0, QModelIndex());
    QModelIndex const right_idx = sourceModel()->index(right.row(), 0, QModelIndex());

    Categories leftCat = tagIdToCat(sourceModel()->data(left_idx, CommunitiesModel::Id).toString());
    Categories rightCat =
      tagIdToCat(sourceModel()->data(right_idx, CommunitiesModel::Id).toString());

    if (leftCat != rightCat)
        return leftCat < rightCat;

    if (leftCat == Space) {
        return left.row() < right.row();
    }

    QString leftName  = sourceModel()->data(left_idx, CommunitiesModel::DisplayName).toString();
    QString rightName = sourceModel()->data(right_idx, CommunitiesModel::DisplayName).toString();

    return leftName.compare(rightName, Qt::CaseInsensitive) < 0;
}
bool
FilteredCommunitiesModel::filterAcceptsRow(int sourceRow, const QModelIndex &) const
{
    CommunitiesModel *m = qobject_cast<CommunitiesModel *>(this->sourceModel());
    if (!m)
        return true;

    if (sourceRow < 2 || sourceRow - 2 >= m->spaceOrder_.size())
        return true;

    auto idx = sourceRow - 2;

    while (idx >= 0 && m->spaceOrder_.tree[idx].depth > 0) {
        idx = m->spaceOrder_.parent(idx);

        if (idx >= 0 && m->spaceOrder_.tree.at(idx).collapsed)
            return false;
    }

    return true;
}

QVariantList
CommunitiesModel::spaceChildrenListFromIndex(QString room, int idx) const
{
    if (idx < -1)
        return {};

    auto room_ = room.toStdString();

    int begin = idx + 1;
    int end   = idx >= 0 ? this->spaceOrder_.lastChild(idx) + 1 : this->spaceOrder_.size();
    QVariantList ret;

    bool canSendParent = Permissions(room).canChange(qml_mtx_events::SpaceParent);

    for (int i = begin; i < end; i++) {
        const auto &e = spaceOrder_.tree[i];
        if (e.depth == spaceOrder_.tree[begin].depth && spaces_.count(e.id)) {
            bool canSendChild = Permissions(e.id).canChange(qml_mtx_events::SpaceChild);
            // For now hide the space, if we can't send any child, since then the only allowed
            // action would be removing a space and even that only works if it currently only has a
            // parent set in the child.
            if (!canSendChild)
                continue;

            auto spaceId = e.id.toStdString();
            auto child =
              cache::client()->getStateEvent<mtx::events::state::space::Child>(spaceId, room_);
            auto parent =
              cache::client()->getStateEvent<mtx::events::state::space::Parent>(room_, spaceId);

            bool childValid =
              child && !child->content.via.value_or(std::vector<std::string>{}).empty();
            bool parentValid =
              parent && !parent->content.via.value_or(std::vector<std::string>{}).empty();
            bool canonical = parent && parent->content.canonical;

            if (e.id == room) {
                canonical = parentValid = childValid = canSendChild = canSendParent = false;
            }

            ret.push_back(
              QVariant::fromValue(SpaceItem(e.id,
                                            QString::fromStdString(spaces_.at(e.id).name),
                                            i,
                                            childValid,
                                            parentValid,
                                            canonical,
                                            canSendChild,
                                            canSendParent)));
        }
    }

    nhlog::ui()->critical("Returning {} spaces", ret.size());
    return ret;
}

void
CommunitiesModel::updateSpaceStatus(QString space,
                                    QString room,
                                    bool setParent,
                                    bool setChild,
                                    bool canonical) const
{
    nhlog::ui()->critical("Setting space {} children {}: {} {} {}",
                          space.toStdString(),
                          room.toStdString(),
                          setParent,
                          setChild,
                          canonical);
    auto child =
      cache::client()
        ->getStateEvent<mtx::events::state::space::Child>(space.toStdString(), room.toStdString())
        .value_or(mtx::events::StateEvent<mtx::events::state::space::Child>{})
        .content;
    auto parent =
      cache::client()
        ->getStateEvent<mtx::events::state::space::Parent>(room.toStdString(), space.toStdString())
        .value_or(mtx::events::StateEvent<mtx::events::state::space::Parent>{})
        .content;

    if (setChild) {
        if (!child.via || child.via->empty()) {
            child.via       = utils::roomVias(room.toStdString());
            child.suggested = true;

            http::client()->send_state_event(
              space.toStdString(),
              room.toStdString(),
              child,
              [space, room](mtx::responses::EventId, mtx::http::RequestErr err) {
                  if (err) {
                      ChatPage::instance()->showNotification(
                        tr("Failed to update space child: %1")
                          .arg(QString::fromStdString(err->matrix_error.error)));
                      nhlog::net()->error("Failed to update child {} of {}: {}",
                                          room.toStdString(),
                                          space.toStdString());
                  }
              });
        }
    } else {
        if (child.via && !child.via->empty()) {
            http::client()->send_state_event(
              space.toStdString(),
              room.toStdString(),
              mtx::events::state::space::Child{},
              [space, room](mtx::responses::EventId, mtx::http::RequestErr err) {
                  if (err) {
                      ChatPage::instance()->showNotification(
                        tr("Failed to delete space child: %1")
                          .arg(QString::fromStdString(err->matrix_error.error)));
                      nhlog::net()->error("Failed to delete child {} of {}: {}",
                                          room.toStdString(),
                                          space.toStdString());
                  }
              });
        }
    }

    if (setParent) {
        if (!parent.via || parent.via->empty() || canonical != parent.canonical) {
            parent.via       = utils::roomVias(room.toStdString());
            parent.canonical = canonical;

            http::client()->send_state_event(
              room.toStdString(),
              space.toStdString(),
              parent,
              [space, room](mtx::responses::EventId, mtx::http::RequestErr err) {
                  if (err) {
                      ChatPage::instance()->showNotification(
                        tr("Failed to update space parent: %1")
                          .arg(QString::fromStdString(err->matrix_error.error)));
                      nhlog::net()->error("Failed to update parent {} of {}: {}",
                                          space.toStdString(),
                                          room.toStdString());
                  }
              });
        }
    } else {
        if (parent.via && !parent.via->empty()) {
            http::client()->send_state_event(
              room.toStdString(),
              space.toStdString(),
              mtx::events::state::space::Parent{},
              [space, room](mtx::responses::EventId, mtx::http::RequestErr err) {
                  if (err) {
                      ChatPage::instance()->showNotification(
                        tr("Failed to delete space parent: %1")
                          .arg(QString::fromStdString(err->matrix_error.error)));
                      nhlog::net()->error("Failed to delete parent {} of {}: {}",
                                          space.toStdString(),
                                          room.toStdString());
                  }
              });
        }
    }
}
