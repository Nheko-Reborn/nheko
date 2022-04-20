// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "CommunitiesModel.h"

#include <set>

#include "Cache.h"
#include "Cache_p.h"
#include "ChatPage.h"
#include "Logging.h"
#include "UserSettingsPage.h"
#include "Utils.h"

CommunitiesModel::CommunitiesModel(QObject *parent)
  : QAbstractListModel(parent)
{
    connect(ChatPage::instance(), &ChatPage::unreadMessages, this, [this](int) {
        // Simply updating every space is easier than tracking which ones need updated.
        if (!spaces_.empty())
            emit dataChanged(index(2, 0), index(spaces_.size() + 2, 0), {Roles::UnreadMessages});
    });
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
            return 0;
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
            return 0;
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
        case CommunitiesModel::Roles::UnreadMessages:
            return utils::getChildNotificationsForSpace(id);
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
            to.tree.push_back({QString::fromStdString(child), i, false});
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

    std::set<std::string> ts;

    std::set<std::string> isSpace;
    std::map<std::string, std::set<std::string>> spaceChilds;
    std::map<std::string, std::set<std::string>> spaceParents;

    auto infos = cache::roomInfo();
    for (auto it = infos.begin(); it != infos.end(); it++) {
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

    hiddenTagIds_ = UserSettings::instance()->hiddenTags();
    spaceOrder_.restoreCollapsed();

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
    bool tagsUpdated = false;

    for (const auto &[roomid, room] : sync_.rooms.join) {
        for (const auto &e : room.account_data.events)
            if (std::holds_alternative<
                  mtx::events::AccountDataEvent<mtx::events::account_data::Tags>>(e)) {
                tagsUpdated = true;
            }
        for (const auto &e : room.state.events)
            if (std::holds_alternative<mtx::events::StateEvent<mtx::events::state::space::Child>>(
                  e) ||
                std::holds_alternative<mtx::events::StateEvent<mtx::events::state::space::Parent>>(
                  e)) {
                tagsUpdated = true;
            }
        for (const auto &e : room.timeline.events)
            if (std::holds_alternative<mtx::events::StateEvent<mtx::events::state::space::Child>>(
                  e) ||
                std::holds_alternative<mtx::events::StateEvent<mtx::events::state::space::Parent>>(
                  e)) {
                tagsUpdated = true;
            }
    }
    for (const auto &[roomid, room] : sync_.rooms.leave) {
        (void)room;
        if (spaces_.count(QString::fromStdString(roomid)))
            tagsUpdated = true;
    }
    for (const auto &e : sync_.account_data.events) {
        if (std::holds_alternative<
              mtx::events::AccountDataEvent<mtx::events::account_data::Direct>>(e)) {
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
    if (hiddenTagIds_.contains(tagId)) {
        hiddenTagIds_.removeOne(tagId);
        UserSettings::instance()->setHiddenTags(hiddenTagIds_);
    } else {
        hiddenTagIds_.push_back(tagId);
        UserSettings::instance()->setHiddenTags(hiddenTagIds_);
    }

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
