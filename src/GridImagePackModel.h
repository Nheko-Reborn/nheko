// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QString>

#include <mtx/events/mscs/image_packs.hpp>

struct StickerImage
{
    Q_GADGET
    Q_PROPERTY(QString url MEMBER url CONSTANT)
    Q_PROPERTY(QString shortcode MEMBER shortcode CONSTANT)
    Q_PROPERTY(QString body MEMBER body CONSTANT)
    Q_PROPERTY(QStringList descriptor READ descriptor CONSTANT)

public:
    QStringList descriptor() const
    {
        if (descriptor_.size() == 3)
            return QStringList{
              QString::fromStdString(descriptor_[0]),
              QString::fromStdString(descriptor_[1]),
              QString::fromStdString(descriptor_[2]),
            };
        else
            return {};
    }

    QString url;
    QString shortcode;
    QString body;

    std::vector<std::string> descriptor_; // roomid, statekey, shortcode
};

class GridImagePackModel final : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles
    {
        PackName = Qt::UserRole,
        Row,
    };

    GridImagePackModel(const std::string &roomId, bool stickers, QObject *parent = nullptr);
    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;

private:
    std::string room_id;

    struct PackDesc
    {
        QString packname;
        QString packavatar;
        std::string room_id, state_key;

        std::vector<std::pair<mtx::events::msc2545::PackImage, QString>> images;
        std::size_t firstRow;
    };

    std::vector<PackDesc> packs;
    std::vector<size_t> rowToPack;
    int columns = 3;
};
