// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>

#include <mtx/events/mscs/image_packs.hpp>

class CombinedImagePackModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles
    {
        Url = Qt::UserRole,
        ShortCode,
        Body,
        PackName,
        OriginalRow,
    };

    CombinedImagePackModel(const std::string &roomId, bool stickers, QObject *parent = nullptr);
    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    mtx::events::msc2545::PackImage imageAt(int row)
    {
        if (row < 0 || static_cast<size_t>(row) >= images.size())
            return {};
        return images.at(static_cast<size_t>(row)).image;
    }
    QString shortcodeAt(int row)
    {
        if (row < 0 || static_cast<size_t>(row) >= images.size())
            return {};
        return images.at(static_cast<size_t>(row)).shortcode;
    }

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
