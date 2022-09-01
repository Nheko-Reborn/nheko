// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QUrl>

#include <mtx/events/mscs/image_packs.hpp>

#include "CacheStructs.h"

class SingleImagePackModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(QString roomid READ roomid CONSTANT)
    Q_PROPERTY(bool fromSpace READ fromSpace CONSTANT)
    Q_PROPERTY(QString statekey READ statekey WRITE setStatekey NOTIFY statekeyChanged)
    Q_PROPERTY(QString attribution READ attribution WRITE setAttribution NOTIFY attributionChanged)
    Q_PROPERTY(QString packname READ packname WRITE setPackname NOTIFY packnameChanged)
    Q_PROPERTY(QString avatarUrl READ avatarUrl WRITE setAvatarUrl NOTIFY avatarUrlChanged)
    Q_PROPERTY(
      bool isStickerPack READ isStickerPack WRITE setIsStickerPack NOTIFY isStickerPackChanged)
    Q_PROPERTY(bool isEmotePack READ isEmotePack WRITE setIsEmotePack NOTIFY isEmotePackChanged)
    Q_PROPERTY(bool isGloballyEnabled READ isGloballyEnabled WRITE setGloballyEnabled NOTIFY
                 globallyEnabledChanged)
    Q_PROPERTY(bool canEdit READ canEdit CONSTANT)

public:
    enum Roles
    {
        Url = Qt::UserRole,
        ShortCode,
        Body,
        IsEmote,
        IsSticker,
    };
    Q_ENUM(Roles);

    SingleImagePackModel(ImagePackInfo pack_, QObject *parent = nullptr);
    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    QString roomid() const { return QString::fromStdString(roomid_); }
    bool fromSpace() const { return fromSpace_; }
    QString statekey() const { return QString::fromStdString(statekey_); }
    QString packname() const { return QString::fromStdString(pack.pack->display_name); }
    QString attribution() const { return QString::fromStdString(pack.pack->attribution); }
    QString avatarUrl() const;
    bool isStickerPack() const { return pack.pack->is_sticker(); }
    bool isEmotePack() const { return pack.pack->is_emoji(); }

    bool isGloballyEnabled() const;
    bool canEdit() const;
    void setGloballyEnabled(bool enabled);

    void setPackname(QString val);
    void setAttribution(QString val);
    void setAvatarUrl(QString val);
    void setStatekey(QString val);
    void setIsStickerPack(bool val);
    void setIsEmotePack(bool val);

    Q_INVOKABLE void save();
    Q_INVOKABLE void addStickers(QList<QUrl> files);
    Q_INVOKABLE void remove(int index);
    Q_INVOKABLE void setAvatar(QUrl file);

signals:
    void globallyEnabledChanged();
    void statekeyChanged();
    void attributionChanged();
    void packnameChanged();
    void avatarUrlChanged();
    void isEmotePackChanged();
    void isStickerPackChanged();

    void addImage(std::string uri, std::string filename, mtx::common::ImageInfo info);
    void avatarUploaded(QString uri);

private slots:
    void addImageCb(std::string uri, std::string filename, mtx::common::ImageInfo info);

private:
    std::string roomid_;
    std::string statekey_, old_statekey_;

    mtx::events::msc2545::ImagePack pack;
    std::vector<std::string> shortcodes;

    bool fromSpace_ = false;
};
