// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include <QMultiMap>
#include <QObject>
#include <QString>

#include <mtx/events/mscs/image_packs.hpp>

#include "CompletionProxyModel.h"

struct StickerImage
{
    Q_GADGET
    Q_PROPERTY(QString url MEMBER url CONSTANT)
    Q_PROPERTY(QString shortcode MEMBER shortcode CONSTANT)
    Q_PROPERTY(QString body MEMBER body CONSTANT)
    Q_PROPERTY(QStringList descriptor READ descriptor CONSTANT)
    Q_PROPERTY(QString markdown READ markdown CONSTANT)

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

    QString markdown() const
    {
        return QStringLiteral(
                 "<img data-mx-emoticon height=\"32\" src=\"%1\" alt=\"%2\" title=\"%2\">")
          .arg(url.toHtmlEscaped(), !body.isEmpty() ? body : shortcode);
    }

    QString url;
    QString shortcode;
    QString body;

    std::vector<std::string> descriptor_; // roomid, statekey, shortcode
};

struct SectionDescription
{
    Q_GADGET
    Q_PROPERTY(QString url MEMBER url CONSTANT)
    Q_PROPERTY(QString name MEMBER name CONSTANT)
    Q_PROPERTY(int firstRowWith MEMBER firstRowWith CONSTANT)

public:
    QString name;
    QString url;
    int firstRowWith = 0;
};

struct TextEmoji
{
    Q_GADGET
    Q_PROPERTY(QString unicode MEMBER unicode CONSTANT)
    Q_PROPERTY(QString unicodeName MEMBER unicodeName CONSTANT)
    Q_PROPERTY(QString shortcode MEMBER shortcode CONSTANT)

public:
    QString unicode;
    QString unicodeName;
    QString shortcode;
};

class GridImagePackModel final : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString searchString READ searchString WRITE setSearchString NOTIFY newSearchString)
    Q_PROPERTY(QList<SectionDescription> sections READ sections NOTIFY newSearchString)

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

    QString searchString() const { return searchString_; }
    void setSearchString(QString newValue);

    QList<SectionDescription> sections() const;

signals:
    void newSearchString();

private:
    std::string room_id;

    struct PackDesc
    {
        QString packname;
        QString packavatar;
        std::string room_id, state_key;

        std::vector<std::pair<mtx::events::msc2545::PackImage, QString>> images;
        std::vector<TextEmoji> emojis;
        std::size_t firstRow;
    };

    std::vector<PackDesc> packs;
    std::vector<size_t> rowToPack;
    int columns = 3;

    QString searchString_;
    trie<uint, std::pair<std::uint32_t, std::uint32_t>> trie_;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> currentSearchResult;
    std::vector<std::size_t> rowToFirstRowEntryFromSearch;

    QString nameFromPack(const PackDesc &pack) const;
    QString avatarFromPack(const PackDesc &pack) const;
};
