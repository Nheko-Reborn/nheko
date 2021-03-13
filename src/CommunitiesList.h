// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QScrollArea>
#include <QSharedPointer>
#include <QVBoxLayout>

#include "CacheStructs.h"
#include "CommunitiesListItem.h"
#include "ui/Theme.h"

namespace mtx::responses {
struct GroupProfile;
struct JoinedGroups;
}

class CommunitiesList : public QWidget
{
        Q_OBJECT

public:
        CommunitiesList(QWidget *parent = nullptr);

        void clear() { communities_.clear(); }

        void addCommunity(const std::string &id);
        void removeCommunity(const QString &id) { communities_.erase(id); };
        std::set<QString> roomList(const QString &id) const;

        void syncTags(const std::map<QString, RoomInfo> &info);
        void setTagsForRoom(const QString &id, const std::vector<std::string> &tags);
        std::vector<std::string> currentTags() const;
        std::set<QString> hiddenTagsAndCommunities() const;

signals:
        void communityChanged(const QString &id);
        void avatarRetrieved(const QString &id, const QPixmap &img);
        void groupProfileRetrieved(const QString &group_id, const mtx::responses::GroupProfile &);
        void groupRoomsRetrieved(const QString &group_id, const std::set<QString> &res);

public slots:
        void updateCommunityAvatar(const QString &id, const QPixmap &img);
        void highlightSelectedCommunity(const QString &id);
        void setCommunities(const mtx::responses::JoinedGroups &groups);

private:
        void fetchCommunityAvatar(const QString &id, const QString &avatarUrl);
        void addGlobalItem() { addCommunity("world"); }
        void sortEntries();

        //! Check whether or not a community id is currently managed.
        bool communityExists(const QString &id) const
        {
                return communities_.find(id) != communities_.end();
        }

        QString selectedCommunity_;
        QVBoxLayout *topLayout_;
        QVBoxLayout *contentsLayout_;
        QScrollArea *scrollArea_;

        std::map<QString, QSharedPointer<CommunitiesListItem>> communities_;
};
