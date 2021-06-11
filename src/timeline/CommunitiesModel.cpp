// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "CommunitiesModel.h"

#include <set>

#include "Cache.h"
#include "UserSettingsPage.h"

CommunitiesModel::CommunitiesModel(QObject *parent)
  : QAbstractListModel(parent)
{}

QHash<int, QByteArray>
CommunitiesModel::roleNames() const
{
        return {
          {AvatarUrl, "avatarUrl"},
          {DisplayName, "displayName"},
          {Tooltip, "tooltip"},
          {ChildrenHidden, "childrenHidden"},
          {Hidden, "hidden"},
          {Id, "id"},
        };
}

QVariant
CommunitiesModel::data(const QModelIndex &index, int role) const
{
        if (index.row() == 0) {
                switch (role) {
                case CommunitiesModel::Roles::AvatarUrl:
                        return QString(":/icons/icons/ui/world.png");
                case CommunitiesModel::Roles::DisplayName:
                        return tr("All rooms");
                case CommunitiesModel::Roles::Tooltip:
                        return tr("Shows all rooms without filtering.");
                case CommunitiesModel::Roles::ChildrenHidden:
                        return false;
                case CommunitiesModel::Roles::Hidden:
                        return false;
                case CommunitiesModel::Roles::Id:
                        return "";
                }
        } else if (index.row() - 1 < tags_.size()) {
                auto tag = tags_.at(index.row() - 1);
                if (tag == "m.favourite") {
                        switch (role) {
                        case CommunitiesModel::Roles::AvatarUrl:
                                return QString(":/icons/icons/ui/star.png");
                        case CommunitiesModel::Roles::DisplayName:
                                return tr("Favourites");
                        case CommunitiesModel::Roles::Tooltip:
                                return tr("Rooms you have favourited.");
                        }
                } else if (tag == "m.lowpriority") {
                        switch (role) {
                        case CommunitiesModel::Roles::AvatarUrl:
                                return QString(":/icons/icons/ui/star.png");
                        case CommunitiesModel::Roles::DisplayName:
                                return tr("Low Priority");
                        case CommunitiesModel::Roles::Tooltip:
                                return tr("Rooms with low priority.");
                        }
                } else if (tag == "m.server_notice") {
                        switch (role) {
                        case CommunitiesModel::Roles::AvatarUrl:
                                return QString(":/icons/icons/ui/tag.png");
                        case CommunitiesModel::Roles::DisplayName:
                                return tr("Server Notices");
                        case CommunitiesModel::Roles::Tooltip:
                                return tr("Messages from your server or administrator.");
                        }
                } else {
                        switch (role) {
                        case CommunitiesModel::Roles::AvatarUrl:
                                return QString(":/icons/icons/ui/tag.png");
                        case CommunitiesModel::Roles::DisplayName:
                                return tag.mid(2);
                        case CommunitiesModel::Roles::Tooltip:
                                return tag.mid(2);
                        }
                }

                switch (role) {
                case CommunitiesModel::Roles::Hidden:
                        return hiddentTagIds_.contains("tag:" + tag);
                case CommunitiesModel::Roles::ChildrenHidden:
                        return true;
                case CommunitiesModel::Roles::Id:
                        return "tag:" + tag;
                }
        }
        return QVariant();
}

void
CommunitiesModel::initializeSidebar()
{
        std::set<std::string> ts;
        for (const auto &e : cache::roomInfo()) {
                for (const auto &t : e.tags) {
                        if (t.find("u.") == 0 || t.find("m." == 0)) {
                                ts.insert(t);
                        }
                }
        }

        beginResetModel();
        tags_.clear();
        for (const auto &t : ts)
                tags_.push_back(QString::fromStdString(t));

        hiddentTagIds_ = UserSettings::instance()->hiddenTags();
        endResetModel();

        emit tagsChanged();
        emit hiddenTagsChanged();
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
CommunitiesModel::sync(const mtx::responses::Rooms &rooms)
{
        bool tagsUpdated = false;

        for (const auto &[roomid, room] : rooms.join) {
                (void)roomid;
                for (const auto &e : room.account_data.events)
                        if (std::holds_alternative<
                              mtx::events::AccountDataEvent<mtx::events::account_data::Tags>>(e)) {
                                tagsUpdated = true;
                        }
        }

        if (tagsUpdated)
                initializeSidebar();
}

void
CommunitiesModel::setCurrentTagId(QString tagId)
{
        if (tagId.startsWith("tag:")) {
                auto tag = tagId.mid(4);
                for (const auto &t : tags_) {
                        if (t == tag) {
                                this->currentTagId_ = tagId;
                                emit currentTagIdChanged(currentTagId_);
                                return;
                        }
                }
        }

        this->currentTagId_ = "";
        emit currentTagIdChanged(currentTagId_);
}

void
CommunitiesModel::toggleTagId(QString tagId)
{
        if (hiddentTagIds_.contains(tagId)) {
                hiddentTagIds_.removeOne(tagId);
                UserSettings::instance()->setHiddenTags(hiddentTagIds_);
        } else {
                hiddentTagIds_.push_back(tagId);
                UserSettings::instance()->setHiddenTags(hiddentTagIds_);
        }

        if (tagId.startsWith("tag:")) {
                auto idx = tags_.indexOf(tagId.mid(4));
                if (idx != -1)
                        emit dataChanged(index(idx), index(idx), {Hidden});
        }

        emit hiddenTagsChanged();
}
