#include "Cache.h"
#include "CommunitiesList.h"
#include "Logging.hpp"
#include "MatrixClient.h"

#include <QLabel>

CommunitiesList::CommunitiesList(QWidget *parent)
  : QWidget(parent)
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

        addGlobalItem();
        contentsLayout_->addStretch(1);

        scrollArea_->setWidget(scrollAreaContents_);
        topLayout_->addWidget(scrollArea_);

        // connect(http::client(),
        //         &MatrixClient::communityProfileRetrieved,
        //         this,
        //         [this](QString communityId, QJsonObject profile) {
        //                 fetchCommunityAvatar(communityId, profile["avatar_url"].toString());
        //         });
        connect(
          this, &CommunitiesList::avatarRetrieved, this, &CommunitiesList::updateCommunityAvatar);
}

void
CommunitiesList::setCommunities(const std::map<QString, QSharedPointer<Community>> &communities)
{
        communities_.clear();

        addGlobalItem();

        for (const auto &community : communities) {
                addCommunity(community.second, community.first);

                // http::client()->fetchCommunityProfile(community.first);
                // http::client()->fetchCommunityRooms(community.first);
        }

        communities_["world"]->setPressedState(true);
        emit communityChanged("world");
}

void
CommunitiesList::addCommunity(QSharedPointer<Community> community, const QString &community_id)
{
        CommunitiesListItem *list_item =
          new CommunitiesListItem(community, community_id, scrollArea_);

        communities_.emplace(community_id, QSharedPointer<CommunitiesListItem>(list_item));

        fetchCommunityAvatar(community_id, community->getAvatar().toString());

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

void
CommunitiesList::fetchCommunityAvatar(const QString &id, const QString &avatarUrl)
{
        auto savedImgData = cache::client()->image(avatarUrl);
        if (!savedImgData.isNull()) {
                QPixmap pix;
                pix.loadFromData(savedImgData);
                emit avatarRetrieved(id, pix);
                return;
        }

        if (avatarUrl.isEmpty())
                return;

        mtx::http::ThumbOpts opts;
        opts.mxc_url = avatarUrl.toStdString();
        http::v2::client()->get_thumbnail(
          opts, [this, opts, id](const std::string &res, mtx::http::RequestErr err) {
                  if (err) {
                          log::net()->warn("failed to download avatar: {} - ({} {})",
                                           opts.mxc_url,
                                           mtx::errors::to_string(err->matrix_error.errcode),
                                           err->matrix_error.error);
                          return;
                  }

                  cache::client()->saveImage(opts.mxc_url, res);

                  auto data = QByteArray(res.data(), res.size());

                  QPixmap pix;
                  pix.loadFromData(data);

                  emit avatarRetrieved(id, pix);
          });
}
