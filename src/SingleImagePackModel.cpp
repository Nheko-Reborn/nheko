// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "SingleImagePackModel.h"

#include <QFile>
#include <QMimeDatabase>

#include <mtx/responses/media.hpp>

#include "Cache_p.h"
#include "ChatPage.h"
#include "Logging.h"
#include "MatrixClient.h"
#include "Utils.h"
#include "timeline/Permissions.h"
#include "timeline/TimelineModel.h"

Q_DECLARE_METATYPE(mtx::common::ImageInfo)

SingleImagePackModel::SingleImagePackModel(ImagePackInfo pack_, QObject *parent)
  : QAbstractListModel(parent)
  , roomid_(std::move(pack_.source_room))
  , statekey_(std::move(pack_.state_key))
  , old_statekey_(statekey_)
  , pack(std::move(pack_.pack))
  , fromSpace_(pack_.from_space)
{
    [[maybe_unused]] static auto imageInfoType = qRegisterMetaType<mtx::common::ImageInfo>();

    if (!pack.pack)
        pack.pack = mtx::events::msc2545::ImagePack::PackDescription{};

    shortcodes.reserve(pack.images.size());
    for (const auto &e : pack.images)
        shortcodes.push_back(e.first);

    connect(this, &SingleImagePackModel::addImage, this, &SingleImagePackModel::addImageCb);
    connect(this, &SingleImagePackModel::avatarUploaded, this, &SingleImagePackModel::setAvatarUrl);
}

int
SingleImagePackModel::rowCount(const QModelIndex &) const
{
    return (int)shortcodes.size();
}

QHash<int, QByteArray>
SingleImagePackModel::roleNames() const
{
    return {
      {Roles::Url, "url"},
      {Roles::ShortCode, "shortCode"},
      {Roles::Body, "body"},
      {Roles::IsEmote, "isEmote"},
      {Roles::IsSticker, "isSticker"},
    };
}

QVariant
SingleImagePackModel::data(const QModelIndex &index, int role) const
{
    if (hasIndex(index.row(), index.column(), index.parent())) {
        const auto &img = pack.images.at(shortcodes.at(index.row()));
        switch (role) {
        case Url:
            return QString::fromStdString(img.url);
        case ShortCode:
            return QString::fromStdString(shortcodes.at(index.row()));
        case Body:
            return QString::fromStdString(img.body);
        case IsEmote:
            return img.overrides_usage() ? img.is_emoji() : pack.pack->is_emoji();
        case IsSticker:
            return img.overrides_usage() ? img.is_sticker() : pack.pack->is_sticker();
        default:
            return {};
        }
    }
    return {};
}

bool
SingleImagePackModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    using mtx::events::msc2545::PackUsage;

    if (hasIndex(index.row(), index.column(), index.parent())) {
        auto &img = pack.images.at(shortcodes.at(index.row()));
        switch (role) {
        case ShortCode: {
            auto newCode = value.toString().toStdString();

            // otherwise we delete this by accident
            if (pack.images.count(newCode))
                return false;

            auto tmp     = img;
            auto oldCode = shortcodes.at(index.row());
            pack.images.erase(oldCode);
            shortcodes[index.row()] = newCode;
            pack.images.insert({newCode, tmp});

            emit dataChanged(
              this->index(index.row()), this->index(index.row()), {Roles::ShortCode});
            return true;
        }
        case Body:
            img.body = value.toString().toStdString();
            emit dataChanged(this->index(index.row()), this->index(index.row()), {Roles::Body});
            return true;
        case IsEmote: {
            bool isEmote   = value.toBool();
            bool isSticker = img.overrides_usage() ? img.is_sticker() : pack.pack->is_sticker();

            img.usage.set(PackUsage::Emoji, isEmote);
            img.usage.set(PackUsage::Sticker, isSticker);

            if (img.usage == pack.pack->usage)
                img.usage.reset();

            emit dataChanged(this->index(index.row()), this->index(index.row()), {Roles::IsEmote});

            return true;
        }
        case IsSticker: {
            bool isEmote   = img.overrides_usage() ? img.is_emoji() : pack.pack->is_emoji();
            bool isSticker = value.toBool();

            img.usage.set(PackUsage::Emoji, isEmote);
            img.usage.set(PackUsage::Sticker, isSticker);

            if (img.usage == pack.pack->usage)
                img.usage.reset();

            emit dataChanged(
              this->index(index.row()), this->index(index.row()), {Roles::IsSticker});

            return true;
        }
        }
    }
    return false;
}

bool
SingleImagePackModel::isGloballyEnabled() const
{
    if (auto roomPacks = cache::client()->getAccountData(mtx::events::EventType::ImagePackRooms)) {
        if (auto tmp =
              std::get_if<mtx::events::EphemeralEvent<mtx::events::msc2545::ImagePackRooms>>(
                &*roomPacks)) {
            if (tmp->content.rooms.count(roomid_) &&
                tmp->content.rooms.at(roomid_).count(statekey_))
                return true;
        }
    }
    return false;
}
void
SingleImagePackModel::setGloballyEnabled(bool enabled)
{
    mtx::events::msc2545::ImagePackRooms content{};
    if (auto roomPacks = cache::client()->getAccountData(mtx::events::EventType::ImagePackRooms)) {
        if (auto tmp =
              std::get_if<mtx::events::EphemeralEvent<mtx::events::msc2545::ImagePackRooms>>(
                &*roomPacks)) {
            content = tmp->content;
        }
    }

    if (enabled)
        content.rooms[roomid_][statekey_] = {};
    else
        content.rooms[roomid_].erase(statekey_);

    http::client()->put_account_data(content, [](mtx::http::RequestErr) {
        // emit this->globallyEnabledChanged();
    });
}

bool
SingleImagePackModel::canEdit() const
{
    if (roomid_.empty())
        return true;
    else
        return Permissions(QString::fromStdString(roomid_))
          .canChange(qml_mtx_events::ImagePackInRoom);
}

void
SingleImagePackModel::setPackname(QString val)
{
    auto val_ = val.toStdString();
    if (val_ != this->pack.pack->display_name) {
        this->pack.pack->display_name = val_;
        emit packnameChanged();
    }
}

void
SingleImagePackModel::setAttribution(QString val)
{
    auto val_ = val.toStdString();
    if (val_ != this->pack.pack->attribution) {
        this->pack.pack->attribution = val_;
        emit attributionChanged();
    }
}

void
SingleImagePackModel::setAvatarUrl(QString val)
{
    auto val_ = val.toStdString();
    if (val_ != this->pack.pack->avatar_url) {
        this->pack.pack->avatar_url = val_;
        emit avatarUrlChanged();
    }
}

QString
SingleImagePackModel::avatarUrl() const
{
    if (!pack.pack->avatar_url.empty())
        return QString::fromStdString(pack.pack->avatar_url);
    else if (!pack.images.empty())
        return QString::fromStdString(pack.images.begin()->second.url);
    else
        return QString();
}

void
SingleImagePackModel::setStatekey(QString val)
{
    auto val_ = val.toStdString();
    if (val_ != statekey_) {
        statekey_ = val_;
        emit statekeyChanged();
    }
}

void
SingleImagePackModel::setIsStickerPack(bool val)
{
    using mtx::events::msc2545::PackUsage;
    if (val != pack.pack->is_sticker()) {
        pack.pack->usage.set(PackUsage::Sticker, val);
        emit isStickerPackChanged();
    }
}

void
SingleImagePackModel::setIsEmotePack(bool val)
{
    using mtx::events::msc2545::PackUsage;
    if (val != pack.pack->is_emoji()) {
        pack.pack->usage.set(PackUsage::Emoji, val);
        emit isEmotePackChanged();
    }
}

void
SingleImagePackModel::save()
{
    if (roomid_.empty()) {
        http::client()->put_account_data(pack, [](mtx::http::RequestErr e) {
            if (e)
                ChatPage::instance()->showNotification(
                  tr("Failed to update image pack: %1")
                    .arg(QString::fromStdString(e->matrix_error.error)));
        });
    } else {
        if (old_statekey_ != statekey_) {
            http::client()->send_state_event(
              roomid_,
              to_string(mtx::events::EventType::ImagePackInRoom),
              old_statekey_,
              nlohmann::json::object(),
              [](const mtx::responses::EventId &, mtx::http::RequestErr e) {
                  if (e)
                      ChatPage::instance()->showNotification(
                        tr("Failed to delete old image pack: %1")
                          .arg(QString::fromStdString(e->matrix_error.error)));
              });
        }

        http::client()->send_state_event(
          roomid_,
          statekey_,
          pack,
          [this](const mtx::responses::EventId &, mtx::http::RequestErr e) {
              if (e)
                  ChatPage::instance()->showNotification(
                    tr("Failed to update image pack: %1")
                      .arg(QString::fromStdString(e->matrix_error.error)));

              nhlog::net()->info("Uploaded image pack: %1", statekey_);
          });
    }
}

void
SingleImagePackModel::addStickers(QList<QUrl> files)
{
    for (const auto &f : files) {
        auto file = QFile(f.toLocalFile());
        if (!file.open(QFile::ReadOnly)) {
            ChatPage::instance()->showNotification(
              tr("Failed to open image: %1").arg(f.toLocalFile()));
            return;
        }

        auto bytes = file.readAll();
        auto img   = utils::readImage(bytes);

        mtx::common::ImageInfo info{};

        auto sz = img.size() / 2;
        if (sz.width() > 512 || sz.height() > 512) {
            sz.scale(512, 512, Qt::AspectRatioMode::KeepAspectRatio);
        } else if (img.height() < 128 && img.width() < 128) {
            sz = img.size();
        }

        info.h        = sz.height();
        info.w        = sz.width();
        info.size     = bytes.size();
        info.mimetype = QMimeDatabase().mimeTypeForFile(f.toLocalFile()).name().toStdString();

        auto filename = f.fileName().toStdString();
        http::client()->upload(
          bytes.toStdString(),
          QMimeDatabase().mimeTypeForFile(f.toLocalFile()).name().toStdString(),
          filename,
          [this, filename, info](const mtx::responses::ContentURI &uri, mtx::http::RequestErr e) {
              if (e) {
                  ChatPage::instance()->showNotification(
                    tr("Failed to upload image: %1")
                      .arg(QString::fromStdString(e->matrix_error.error)));
                  return;
              }

              emit addImage(uri.content_uri, filename, info);
          });
    }
}

void
SingleImagePackModel::setAvatar(QUrl f)
{
    auto file = QFile(f.toLocalFile());
    if (!file.open(QFile::ReadOnly)) {
        ChatPage::instance()->showNotification(tr("Failed to open image: %1").arg(f.toLocalFile()));
        return;
    }

    auto bytes = file.readAll();

    auto filename = f.fileName().toStdString();
    http::client()->upload(
      bytes.toStdString(),
      QMimeDatabase().mimeTypeForFile(f.toLocalFile()).name().toStdString(),
      filename,
      [this, filename](const mtx::responses::ContentURI &uri, mtx::http::RequestErr e) {
          if (e) {
              ChatPage::instance()->showNotification(
                tr("Failed to upload image: %1")
                  .arg(QString::fromStdString(e->matrix_error.error)));
              return;
          }

          emit avatarUploaded(QString::fromStdString(uri.content_uri));
      });
}

void
SingleImagePackModel::remove(int idx)
{
    if (idx < (int)shortcodes.size() && idx >= 0) {
        beginRemoveRows(QModelIndex(), idx, idx);
        auto s = shortcodes.at(idx);
        shortcodes.erase(shortcodes.begin() + idx);
        pack.images.erase(s);
        endRemoveRows();
    }
}

void
SingleImagePackModel::addImageCb(std::string uri, std::string filename, mtx::common::ImageInfo info)
{
    mtx::events::msc2545::PackImage img{};
    img.url  = uri;
    img.info = info;
    beginInsertRows(
      QModelIndex(), static_cast<int>(shortcodes.size()), static_cast<int>(shortcodes.size()));

    pack.images[filename] = img;
    shortcodes.push_back(filename);

    endInsertRows();

    if (this->pack.pack->avatar_url.empty())
        this->setAvatarUrl(QString::fromStdString(uri));
}
