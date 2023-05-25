// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>

#include <mtx/events/mscs/image_packs.hpp>

class CombinedImagePackModel final : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles
    {
        Url = Qt::UserRole,
        ShortCode,
        Body,
        PackName,
        Unicode,
    };

    CombinedImagePackModel(const std::string &roomId, QObject *parent = nullptr);
    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;

private:
    std::string room_id;

    struct ImageDesc
    {
        QString shortcode;
        QString packname;

        mtx::events::msc2545::PackImage image;
    };

    std::vector<ImageDesc> images;
};
