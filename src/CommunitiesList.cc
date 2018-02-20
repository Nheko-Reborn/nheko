#include "CommunitiesList.h"

#include <QLabel>

CommunitiesList::CommunitiesList(QSharedPointer<MatrixClient> client, QWidget *parent)
  : QWidget(parent)
  , client_(client)
{
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(1);
        setSizePolicy(sizePolicy);

        setStyleSheet("border-style: none;");

        topLayout_ = new QVBoxLayout(this);
        topLayout_->setSpacing(0);
        topLayout_->setMargin(0);

        setFixedWidth(ui::sidebar::CommunitiesSidebarSize);

        scrollArea_ = new QScrollArea(this);
        scrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scrollArea_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scrollArea_->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
        scrollArea_->setWidgetResizable(true);
        scrollArea_->setAlignment(Qt::AlignLeading | Qt::AlignTop | Qt::AlignVCenter);

        scrollAreaContents_ = new QWidget();

        contentsLayout_ = new QVBoxLayout(scrollAreaContents_);
        contentsLayout_->setSpacing(0);
        contentsLayout_->setMargin(0);

        WorldCommunityListItem *world_list_item = new WorldCommunityListItem();
        contentsLayout_->addWidget(world_list_item);
        communities_.emplace("world", QSharedPointer<CommunitiesListItem>(world_list_item));
        connect(world_list_item,
                &WorldCommunityListItem::clicked,
                this,
                &CommunitiesList::highlightSelectedCommunity);
        contentsLayout_->addStretch(1);

        scrollArea_->setWidget(scrollAreaContents_);
        topLayout_->addWidget(scrollArea_);

        connect(client_.data(),
                &MatrixClient::communityProfileRetrieved,
                this,
                [this](QString communityId, QJsonObject profile) {
                        client_->fetchCommunityAvatar(communityId,
                                                      QUrl(profile["avatar_url"].toString()));
                });
        connect(client_.data(),
                SIGNAL(communityAvatarRetrieved(const QString &, const QPixmap &)),
                this,
                SLOT(updateCommunityAvatar(const QString &, const QPixmap &)));
}

void
CommunitiesList::setCommunities(const std::map<QString, QSharedPointer<Community>> &communities)
{
        communities_.clear();

        // TODO: still not sure how to handle the "world" special-case
        WorldCommunityListItem *world_list_item = new WorldCommunityListItem();
        communities_.emplace("world", QSharedPointer<CommunitiesListItem>(world_list_item));
        connect(world_list_item,
                &WorldCommunityListItem::clicked,
                this,
                &CommunitiesList::highlightSelectedCommunity);
        contentsLayout_->insertWidget(0, world_list_item);

        for (const auto &community : communities) {
                addCommunity(community.second, community.first);

                client_->fetchCommunityProfile(community.first);
                client_->fetchCommunityRooms(community.first);
        }

        world_list_item->setPressedState(true);
        emit communityChanged("world");
}

void
CommunitiesList::addCommunity(QSharedPointer<Community> community, const QString &community_id)
{
        CommunitiesListItem *list_item =
          new CommunitiesListItem(community, community_id, scrollArea_);

        communities_.emplace(community_id, QSharedPointer<CommunitiesListItem>(list_item));

        client_->fetchCommunityAvatar(community_id, community->getAvatar());

        contentsLayout_->insertWidget(contentsLayout_->count() - 1, list_item);

        connect(list_item,
                &CommunitiesListItem::clicked,
                this,
                &CommunitiesList::highlightSelectedCommunity);
}

void
CommunitiesList::updateCommunityAvatar(const QString &community_id, const QPixmap &img)
{
        if (!communityExists(community_id)) {
                qWarning() << "Avatar update on nonexistent community" << community_id;
                return;
        }

        communities_.find(community_id)->second->setAvatar(img.toImage());
}

void
CommunitiesList::highlightSelectedCommunity(const QString &community_id)
{
        if (!communityExists(community_id)) {
                qDebug() << "CommunitiesList: clicked unknown community";
                return;
        }

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
