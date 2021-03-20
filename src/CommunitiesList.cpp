// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "CommunitiesList.h"
#include "Cache.h"
#include "Logging.h"
#include "MatrixClient.h"
#include "MxcImageProvider.h"
#include "Splitter.h"
#include "UserSettingsPage.h"

#include <mtx/responses/groups.hpp>
#include <nlohmann/json.hpp>

#include <QLabel>

CommunitiesList::CommunitiesList(QWidget *parent)
  : QWidget(parent)
{
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(1);
        setSizePolicy(sizePolicy);

        topLayout_ = new QVBoxLayout(this);
        topLayout_->setSpacing(0);
        topLayout_->setMargin(0);

        const auto sideBarSizes = splitter::calculateSidebarSizes(QFont{});
        setFixedWidth(sideBarSizes.groups);

        scrollArea_ = new QScrollArea(this);
        scrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scrollArea_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scrollArea_->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
        scrollArea_->setWidgetResizable(true);
        scrollArea_->setAlignment(Qt::AlignLeading | Qt::AlignTop | Qt::AlignVCenter);

        contentsLayout_ = new QVBoxLayout();
        contentsLayout_->setSpacing(0);
        contentsLayout_->setMargin(0);

        addGlobalItem();
        contentsLayout_->addStretch(1);

        scrollArea_->setLayout(contentsLayout_);
        topLayout_->addWidget(scrollArea_);

        connect(
          this, &CommunitiesList::avatarRetrieved, this, &CommunitiesList::updateCommunityAvatar);
}

void
CommunitiesList::setCommunities(const mtx::responses::JoinedGroups &response)
{
        // remove all non-tag communities
        auto it = communities_.begin();
        while (it != communities_.end()) {
                if (it->second->is_tag()) {
                        ++it;
                } else {
                        it = communities_.erase(it);
                }
        }

        addGlobalItem();

        for (const auto &group : response.groups)
                addCommunity(group);

        communities_["world"]->setPressedState(true);
        selectedCommunity_ = "world";
        emit communityChanged("world");
        sortEntries();
}

void
CommunitiesList::syncTags(const std::map<QString, RoomInfo> &info)
{
        for (const auto &room : info)
                setTagsForRoom(room.first, room.second.tags);
        emit communityChanged(selectedCommunity_);
        sortEntries();
}

void
CommunitiesList::setTagsForRoom(const QString &room_id, const std::vector<std::string> &tags)
{
        // create missing tag if any
        for (const auto &tag : tags) {
                // filter out tags we should ignore according to the spec
                // https://matrix.org/docs/spec/client_server/r0.4.0.html#id154
                // nheko currently does not make use of internal tags
                // so we ignore any tag containig a `.` (which would indicate a tag
                // in the form `tld.domain.*`) except for `m.*` and `u.*`.
                if (tag.find(".") != ::std::string::npos && tag.compare(0, 2, "m.") &&
                    tag.compare(0, 2, "u."))
                        continue;
                QString name = QString("tag:") + QString::fromStdString(tag);
                if (!communityExists(name)) {
                        addCommunity(std::string("tag:") + tag);
                }
        }
        // update membership of the room for all tags
        auto it = communities_.begin();
        while (it != communities_.end()) {
                // Skip if the community is not a tag
                if (!it->second->is_tag()) {
                        ++it;
                        continue;
                }
                // insert or remove the room from the tag as appropriate
                std::string current_tag =
                  it->first.right(static_cast<int>(it->first.size() - strlen("tag:")))
                    .toStdString();
                if (std::find(tags.begin(), tags.end(), current_tag) != tags.end()) {
                        // the room has this tag
                        it->second->addRoom(room_id);
                } else {
                        // the room does not have this tag
                        it->second->delRoom(room_id);
                }
                // Check if the tag is now empty, if yes delete it
                if (it->second->rooms().empty()) {
                        it = communities_.erase(it);
                } else {
                        ++it;
                }
        }
}

void
CommunitiesList::addCommunity(const std::string &group_id)
{
        auto hiddenTags = UserSettings::instance()->hiddenTags();

        const auto id = QString::fromStdString(group_id);

        CommunitiesListItem *list_item = new CommunitiesListItem(id, scrollArea_);

        if (hiddenTags.contains(id))
                list_item->setDisabled(true);

        communities_.emplace(id, QSharedPointer<CommunitiesListItem>(list_item));
        contentsLayout_->insertWidget(contentsLayout_->count() - 1, list_item);

        connect(list_item,
                &CommunitiesListItem::clicked,
                this,
                &CommunitiesList::highlightSelectedCommunity);
        connect(list_item, &CommunitiesListItem::isDisabledChanged, this, [this]() {
                for (const auto &community : communities_) {
                        if (community.second->isPressed()) {
                                emit highlightSelectedCommunity(community.first);
                                break;
                        }
                }

                auto hiddenTags = hiddenTagsAndCommunities();
                // Qt < 5.14 compat
                QStringList hiddenTags_;
                for (auto &&t : hiddenTags)
                        hiddenTags_.push_back(t);
                UserSettings::instance()->setHiddenTags(hiddenTags_);
        });

        if (group_id.empty() || group_id.front() != '+')
                return;

        nhlog::ui()->debug("Add community: {}", group_id);

        connect(this,
                &CommunitiesList::groupProfileRetrieved,
                this,
                [this](const QString &id, const mtx::responses::GroupProfile &profile) {
                        if (communities_.find(id) == communities_.end())
                                return;

                        communities_.at(id)->setName(QString::fromStdString(profile.name));

                        if (!profile.avatar_url.empty())
                                fetchCommunityAvatar(id,
                                                     QString::fromStdString(profile.avatar_url));
                });
        connect(this,
                &CommunitiesList::groupRoomsRetrieved,
                this,
                [this](const QString &id, const std::set<QString> &rooms) {
                        nhlog::ui()->info(
                          "Fetched rooms for {}: {}", id.toStdString(), rooms.size());
                        if (communities_.find(id) == communities_.end())
                                return;

                        communities_.at(id)->setRooms(rooms);
                });

        http::client()->group_profile(
          group_id, [id, this](const mtx::responses::GroupProfile &res, mtx::http::RequestErr err) {
                  if (err) {
                          return;
                  }

                  emit groupProfileRetrieved(id, res);
          });

        http::client()->group_rooms(
          group_id, [id, this](const nlohmann::json &res, mtx::http::RequestErr err) {
                  if (err) {
                          return;
                  }

                  std::set<QString> room_ids;
                  for (const auto &room : res.at("chunk"))
                          room_ids.emplace(QString::fromStdString(room.at("room_id")));

                  emit groupRoomsRetrieved(id, room_ids);
          });
}

void
CommunitiesList::updateCommunityAvatar(const QString &community_id, const QPixmap &img)
{
        if (!communityExists(community_id)) {
                nhlog::ui()->warn("Avatar update on nonexistent community {}",
                                  community_id.toStdString());
                return;
        }

        communities_.at(community_id)->setAvatar(img.toImage());
}

void
CommunitiesList::highlightSelectedCommunity(const QString &community_id)
{
        if (!communityExists(community_id)) {
                nhlog::ui()->debug("CommunitiesList: clicked unknown community");
                return;
        }

        selectedCommunity_ = community_id;
        emit communityChanged(community_id);

        for (const auto &community : communities_) {
                if (community.first != community_id) {
                        community.second->setPressedState(false);
                } else {
                        community.second->setPressedState(true);
                        scrollArea_->ensureWidgetVisible(community.second.data());
                }
        }
}

void
CommunitiesList::fetchCommunityAvatar(const QString &id, const QString &avatarUrl)
{
        MxcImageProvider::download(
          QString(avatarUrl).remove(QStringLiteral("mxc://")),
          QSize(96, 96),
          [this, id](QString, QSize, QImage img, QString) {
                  if (img.isNull()) {
                          nhlog::net()->warn("failed to download avatar: {})", id.toStdString());
                          return;
                  }

                  emit avatarRetrieved(id, QPixmap::fromImage(img));
          });
}

std::set<QString>
CommunitiesList::roomList(const QString &id) const
{
        if (communityExists(id))
                return communities_.at(id)->rooms();

        return {};
}

std::vector<std::string>
CommunitiesList::currentTags() const
{
        std::vector<std::string> tags;
        for (auto &entry : communities_) {
                CommunitiesListItem *item = entry.second.data();
                if (item->is_tag())
                        tags.push_back(entry.first.mid(4).toStdString());
        }
        return tags;
}

std::set<QString>
CommunitiesList::hiddenTagsAndCommunities() const
{
        std::set<QString> hiddenTags;
        for (auto &entry : communities_) {
                if (entry.second->isDisabled())
                        hiddenTags.insert(entry.first);
        }

        return hiddenTags;
}

void
CommunitiesList::sortEntries()
{
        std::vector<CommunitiesListItem *> header;
        std::vector<CommunitiesListItem *> communities;
        std::vector<CommunitiesListItem *> tags;
        std::vector<CommunitiesListItem *> footer;
        // remove all the contents and sort them in the 4 vectors
        for (auto &entry : communities_) {
                CommunitiesListItem *item = entry.second.data();
                contentsLayout_->removeWidget(item);
                // world is handled separately
                if (entry.first == "world")
                        continue;
                // sort the rest
                if (item->is_tag())
                        if (entry.first == "tag:m.favourite")
                                header.push_back(item);
                        else if (entry.first == "tag:m.lowpriority")
                                footer.push_back(item);
                        else
                                tags.push_back(item);
                else
                        communities.push_back(item);
        }

        // now there remains only the stretch in the layout, remove it
        QLayoutItem *stretch = contentsLayout_->itemAt(0);
        contentsLayout_->removeItem(stretch);

        contentsLayout_->addWidget(communities_["world"].data());

        auto insert_widgets = [this](auto &vec) {
                for (auto item : vec)
                        contentsLayout_->addWidget(item);
        };
        insert_widgets(header);
        insert_widgets(communities);
        insert_widgets(tags);
        insert_widgets(footer);

        contentsLayout_->addItem(stretch);
}
