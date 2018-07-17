#include "CommunitiesList.h"
#include "Cache.h"
#include "Logging.h"
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

        connect(
          this, &CommunitiesList::avatarRetrieved, this, &CommunitiesList::updateCommunityAvatar);
}

void
CommunitiesList::setCommunities(const mtx::responses::JoinedGroups &response)
{
        communities_.clear();

        addGlobalItem();

        for (const auto &group : response.groups)
                addCommunity(group);

        communities_["world"]->setPressedState(true);
        emit communityChanged("world");
}

void
CommunitiesList::addCommunity(const std::string &group_id)
{
        const auto id = QString::fromStdString(group_id);

        CommunitiesListItem *list_item = new CommunitiesListItem(id, scrollArea_);
        communities_.emplace(id, QSharedPointer<CommunitiesListItem>(list_item));
        contentsLayout_->insertWidget(contentsLayout_->count() - 1, list_item);

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
                [this](const QString &id, const std::vector<QString> &rooms) {
                        if (communities_.find(id) == communities_.end())
                                return;

                        communities_.at(id)->setRooms(rooms);
                });
        connect(list_item,
                &CommunitiesListItem::clicked,
                this,
                &CommunitiesList::highlightSelectedCommunity);

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

                  std::vector<QString> room_ids;
                  for (const auto &room : res.at("chunk"))
                          room_ids.push_back(QString::fromStdString(room.at("room_id")));

                  emit groupRoomsRetrieved(id, room_ids);
          });
}

void
CommunitiesList::updateCommunityAvatar(const QString &community_id, const QPixmap &img)
{
        if (!communityExists(community_id)) {
                qWarning() << "Avatar update on nonexistent community" << community_id;
                return;
        }

        communities_.at(community_id)->setAvatar(img.toImage());
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
        http::client()->get_thumbnail(
          opts, [this, opts, id](const std::string &res, mtx::http::RequestErr err) {
                  if (err) {
                          nhlog::net()->warn("failed to download avatar: {} - ({} {})",
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

std::vector<QString>
CommunitiesList::roomList(const QString &id) const
{
        if (communityExists(id))
                return communities_.at(id)->rooms();

        return {};
}
