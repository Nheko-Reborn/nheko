// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>

#include <mtx/events/mscs/image_packs.hpp>

#include "CacheStructs.h"

class SingleImagePackModel : public QAbstractListModel
{
        Q_OBJECT

        Q_PROPERTY(QString roomid READ roomid CONSTANT)
        Q_PROPERTY(QString statekey READ statekey CONSTANT)
        Q_PROPERTY(QString attribution READ statekey CONSTANT)
        Q_PROPERTY(QString packname READ packname CONSTANT)
        Q_PROPERTY(QString avatarUrl READ avatarUrl CONSTANT)
        Q_PROPERTY(bool isStickerPack READ isStickerPack CONSTANT)
        Q_PROPERTY(bool isEmotePack READ isEmotePack CONSTANT)
        Q_PROPERTY(bool isGloballyEnabled READ isGloballyEnabled WRITE setGloballyEnabled NOTIFY
                     globallyEnabledChanged)
public:
        enum Roles
        {
                Url = Qt::UserRole,
                ShortCode,
                Body,
                IsEmote,
                IsSticker,
        };

        SingleImagePackModel(ImagePackInfo pack_, QObject *parent = nullptr);
        QHash<int, QByteArray> roleNames() const override;
        int rowCount(const QModelIndex &parent = QModelIndex()) const override;
        QVariant data(const QModelIndex &index, int role) const override;

        QString roomid() const { return QString::fromStdString(roomid_); }
        QString statekey() const { return QString::fromStdString(statekey_); }
        QString packname() const { return QString::fromStdString(pack.pack->display_name); }
        QString attribution() const { return QString::fromStdString(pack.pack->attribution); }
        QString avatarUrl() const { return QString::fromStdString(pack.pack->avatar_url); }
        bool isStickerPack() const { return pack.pack->is_sticker(); }
        bool isEmotePack() const { return pack.pack->is_emoji(); }

        bool isGloballyEnabled() const;
        void setGloballyEnabled(bool enabled);

signals:
        void globallyEnabledChanged();

private:
        std::string roomid_;
        std::string statekey_;

        mtx::events::msc2545::ImagePack pack;
        std::vector<std::string> shortcodes;
};
