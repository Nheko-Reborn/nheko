// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "GridImagePackModel.h"

#include <QCoreApplication>
#include <QTextBoundaryFinder>

#include <algorithm>

#include "Cache.h"
#include "emoji/Provider.h"

QString
emoji::categoryToName(emoji::Emoji::Category cat)
{
    switch (cat) {
    case emoji::Emoji::Category::People:
        return QCoreApplication::translate("emoji-catagory", "People");
    case emoji::Emoji::Category::Nature:
        return QCoreApplication::translate("emoji-catagory", "Nature");
    case emoji::Emoji::Category::Food:
        return QCoreApplication::translate("emoji-catagory", "Food");
    case emoji::Emoji::Category::Activity:
        return QCoreApplication::translate("emoji-catagory", "Activity");
    case emoji::Emoji::Category::Travel:
        return QCoreApplication::translate("emoji-catagory", "Travel");
    case emoji::Emoji::Category::Objects:
        return QCoreApplication::translate("emoji-catagory", "Objects");
    case emoji::Emoji::Category::Symbols:
        return QCoreApplication::translate("emoji-catagory", "Symbols");
    case emoji::Emoji::Category::Flags:
        return QCoreApplication::translate("emoji-catagory", "Flags");
    default:
        return "";
    }
}

static QString
categoryToIcon(emoji::Emoji::Category cat)
{
    switch (cat) {
    case emoji::Emoji::Category::People:
        return QStringLiteral(":/icons/icons/emoji-categories/people.svg");
    case emoji::Emoji::Category::Nature:
        return QStringLiteral(":/icons/icons/emoji-categories/nature.svg");
    case emoji::Emoji::Category::Food:
        return QStringLiteral(":/icons/icons/emoji-categories/foods.svg");
    case emoji::Emoji::Category::Activity:
        return QStringLiteral(":/icons/icons/emoji-categories/activity.svg");
    case emoji::Emoji::Category::Travel:
        return QStringLiteral(":/icons/icons/emoji-categories/travel.svg");
    case emoji::Emoji::Category::Objects:
        return QStringLiteral(":/icons/icons/emoji-categories/objects.svg");
    case emoji::Emoji::Category::Symbols:
        return QStringLiteral(":/icons/icons/emoji-categories/symbols.svg");
    case emoji::Emoji::Category::Flags:
        return QStringLiteral(":/icons/icons/emoji-categories/flags.svg");
    default:
        return "";
    }
}

GridImagePackModel::GridImagePackModel(const std::string &roomId, bool stickers, QObject *parent)
  : QAbstractListModel(parent)
  , room_id(roomId)
  , columns(stickers ? 3 : 7)
{
    if (!stickers) {
        for (const auto &category : {
               emoji::Emoji::Category::People,
               emoji::Emoji::Category::Nature,
               emoji::Emoji::Category::Food,
               emoji::Emoji::Category::Activity,
               emoji::Emoji::Category::Travel,
               emoji::Emoji::Category::Objects,
               emoji::Emoji::Category::Symbols,
               emoji::Emoji::Category::Flags,
             }) {
            PackDesc newPack{};
            newPack.packname   = categoryToName(category);
            newPack.packavatar = categoryToIcon(category);

            auto emojisInCategory = std::ranges::equal_range(
              emoji::Provider::emoji, category, {}, &emoji::Emoji::category);
            newPack.emojis.reserve(emojisInCategory.size());

            for (const auto &e : emojisInCategory) {
                newPack.emojis.push_back(TextEmoji{.unicode     = e.unicode(),
                                                   .unicodeName = e.unicodeName(),
                                                   .shortcode   = e.shortName()});
            }

            size_t packRowCount =
              (newPack.emojis.size() / columns) + (newPack.emojis.size() % columns ? 1 : 0);
            newPack.firstRow = rowToPack.size();
            for (size_t i = 0; i < packRowCount; i++)
                rowToPack.push_back(packs.size());
            packs.push_back(std::move(newPack));
        }
    }

    auto originalPacks = cache::getImagePacks(room_id, stickers);

    for (auto &pack : originalPacks) {
        PackDesc newPack{};
        newPack.packname =
          pack.pack.pack ? QString::fromStdString(pack.pack.pack->display_name) : QString();
        newPack.packavatar =
          pack.pack.pack ? QString::fromStdString(pack.pack.pack->avatar_url) : QString();
        newPack.room_id   = pack.source_room;
        newPack.state_key = pack.state_key;

        newPack.images.resize(pack.pack.images.size());
        std::ranges::transform(std::move(pack.pack.images), newPack.images.begin(), [](auto &&img) {
            return std::pair(std::move(img.second), QString::fromStdString(img.first));
        });

        size_t packRowCount =
          (newPack.images.size() / columns) + (newPack.images.size() % columns ? 1 : 0);
        newPack.firstRow = rowToPack.size();
        for (size_t i = 0; i < packRowCount; i++)
            rowToPack.push_back(packs.size());
        packs.push_back(std::move(newPack));
    }

    // prepare search index

    auto insertParts = [this](const QString &str, std::pair<std::uint32_t, std::uint32_t> id) {
        QTextBoundaryFinder finder(QTextBoundaryFinder::BoundaryType::Word, str);
        finder.toStart();
        do {
            auto start = finder.position();
            finder.toNextBoundary();
            auto end = finder.position();

            auto ref = QStringView(str).mid(start, end - start).trimmed();
            if (!ref.isEmpty())
                trie_.insert<ElementRank::second>(ref.toUcs4(), id);
        } while (finder.position() < str.size());
    };

    std::uint32_t packIndex = 0;
    for (const auto &pack : packs) {
        std::uint32_t emojiIndex = 0;
        for (const auto &emoji : pack.emojis) {
            std::pair<std::uint32_t, std::uint32_t> key{packIndex, emojiIndex};

            QString string1 = emoji.shortcode.toCaseFolded();
            QString string2 = emoji.unicodeName.toCaseFolded();

            if (!string1.isEmpty()) {
                trie_.insert<ElementRank::first>(string1.toUcs4(), key);
                insertParts(string1, key);
            }
            if (!string2.isEmpty()) {
                trie_.insert<ElementRank::first>(string2.toUcs4(), key);
                insertParts(string2, key);
            }

            emojiIndex++;
        }

        std::uint32_t imgIndex = 0;
        for (const auto &img : pack.images) {
            std::pair<std::uint32_t, std::uint32_t> key{packIndex, imgIndex};

            QString string1 = img.second.toCaseFolded();
            QString string2 = QString::fromStdString(img.first.body).toCaseFolded();

            if (!string1.isEmpty()) {
                trie_.insert<ElementRank::first>(string1.toUcs4(), key);
                insertParts(string1, key);
            }
            if (!string2.isEmpty()) {
                trie_.insert<ElementRank::first>(string2.toUcs4(), key);
                insertParts(string2, key);
            }

            imgIndex++;
        }
        packIndex++;
    }
}

int
GridImagePackModel::rowCount(const QModelIndex &) const
{
    return static_cast<int>(searchString_.isEmpty() ? rowToPack.size()
                                                    : rowToFirstRowEntryFromSearch.size());
}

QHash<int, QByteArray>
GridImagePackModel::roleNames() const
{
    return {
      {Roles::PackName, "packname"},
      {Roles::Row, "row"},
    };
}

QVariant
GridImagePackModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < rowCount() && index.row() >= 0) {
        if (searchString_.isEmpty()) {
            const auto &pack = packs[rowToPack[index.row()]];
            switch (role) {
            case Roles::PackName:
                return nameFromPack(pack);
            case Roles::Row: {
                std::size_t offset = static_cast<std::size_t>(index.row()) - pack.firstRow;
                if (pack.emojis.empty()) {
                    QList<StickerImage> imgs;
                    auto endOffset = std::min((offset + 1) * columns, pack.images.size());
                    for (std::size_t img = offset * columns; img < endOffset; img++) {
                        const auto &data = pack.images.at(img);
                        // See
                        // https://developercommunity.visualstudio.com/t/Internal-compile-error-while-compiling-c/1227337
                        imgs.push_back({/*.url        =*/QString::fromStdString(data.first.url),
                                        /*.shortcode  =*/data.second,
                                        /*.body       =*/QString::fromStdString(data.first.body),
                                        /*.descriptor_=*/
                                        std::vector{
                                          pack.room_id,
                                          pack.state_key,
                                          data.second.toStdString(),
                                        }});
                    }
                    return QVariant::fromValue(imgs);
                } else {
                    auto endOffset = std::min((offset + 1) * columns, pack.emojis.size());
                    QList<TextEmoji> imgs(pack.emojis.begin() + offset * columns,
                                          pack.emojis.begin() + endOffset);

                    return QVariant::fromValue(imgs);
                }
            }
            default:
                return {};
            }
        } else {
            if (static_cast<size_t>(index.row()) >= rowToFirstRowEntryFromSearch.size())
                return {};

            const auto firstIndex = rowToFirstRowEntryFromSearch[index.row()];
            const auto firstEntry = currentSearchResult[firstIndex];
            const auto &pack      = packs[firstEntry.first];

            switch (role) {
            case Roles::PackName:
                return nameFromPack(pack);
            case Roles::Row: {
                if (pack.emojis.empty()) {
                    QList<StickerImage> imgs;
                    for (auto img = firstIndex;
                         imgs.size() < columns && img < currentSearchResult.size() &&
                         currentSearchResult[img].first == firstEntry.first;
                         img++) {
                        const auto &data = pack.images.at(currentSearchResult[img].second);
                        // See
                        // https://developercommunity.visualstudio.com/t/Internal-compile-error-while-compiling-c/1227337
                        imgs.push_back({/*.url         = */ QString::fromStdString(data.first.url),
                                        /*.shortcode   = */ data.second,
                                        /*.body        = */ QString::fromStdString(data.first.body),
                                        /*.descriptor_ = */
                                        std::vector{
                                          pack.room_id,
                                          pack.state_key,
                                          data.second.toStdString(),
                                        }});
                    }
                    return QVariant::fromValue(imgs);
                } else {
                    QList<TextEmoji> emojis;
                    for (auto emoji = firstIndex;
                         emojis.size() < columns && emoji < currentSearchResult.size() &&
                         currentSearchResult[emoji].first == firstEntry.first;
                         emoji++) {
                        emojis.push_back(pack.emojis.at(currentSearchResult[emoji].second));
                    }
                    return QVariant::fromValue(emojis);
                }
            }
            default:
                return {};
            }
        }
    }
    return {};
}

QString
GridImagePackModel::nameFromPack(const PackDesc &pack) const
{
    if (!pack.packname.isEmpty()) {
        return pack.packname;
    }

    if (!pack.state_key.empty()) {
        return QString::fromStdString(pack.state_key);
    }

    if (!pack.room_id.empty()) {
        auto info = cache::singleRoomInfo(pack.room_id);
        return QString::fromStdString(info.name);
    }

    return tr("Account Pack");
}

QString
GridImagePackModel::avatarFromPack(const PackDesc &pack) const
{
    if (!pack.packavatar.isEmpty()) {
        return pack.packavatar;
    }

    if (!pack.images.empty()) {
        return QString::fromStdString(pack.images.begin()->first.url);
    }

    return "";
}

QList<SectionDescription>
GridImagePackModel::sections() const
{
    QList<SectionDescription> sectionNames;
    if (searchString_.isEmpty()) {
        std::size_t packIdx = -1;
        for (std::size_t i = 0; i < rowToPack.size(); i++) {
            if (rowToPack[i] != packIdx) {
                const auto &pack = packs[rowToPack[i]];
                sectionNames.push_back({
                  .name         = nameFromPack(pack),
                  .url          = avatarFromPack(pack),
                  .firstRowWith = static_cast<int>(i),
                });
                packIdx = rowToPack[i];
            }
        }
    } else {
        std::uint32_t packIdx = -1;
        int row               = 0;
        for (const auto &i : rowToFirstRowEntryFromSearch) {
            const auto res = currentSearchResult[i];
            if (res.first != packIdx) {
                packIdx          = res.first;
                const auto &pack = packs[packIdx];
                sectionNames.push_back({
                  .name         = nameFromPack(pack),
                  .url          = avatarFromPack(pack),
                  .firstRowWith = row,
                });
            }
            row++;
        }
    }

    return sectionNames;
}

void
GridImagePackModel::setSearchString(QString key)
{
    beginResetModel();
    currentSearchResult.clear();
    rowToFirstRowEntryFromSearch.clear();
    searchString_ = key;

    if (!key.isEmpty()) {
        auto searchParts = key.toCaseFolded().toUcs4();
        auto tempResults =
          trie_.search(searchParts, static_cast<std::size_t>(columns * columns * 4));

        std::map<std::uint32_t, std::size_t> firstPositionOfPack;
        for (const auto &e : tempResults)
            firstPositionOfPack.emplace(e.first, firstPositionOfPack.size());

        std::ranges::stable_sort(tempResults, [&firstPositionOfPack](auto a, auto b) {
            return firstPositionOfPack[a.first] < firstPositionOfPack[b.first];
        });
        currentSearchResult = std::move(tempResults);

        std::size_t lastPack = -1;
        int columnIndex      = 0;
        for (std::size_t i = 0; i < currentSearchResult.size(); i++) {
            auto elem = currentSearchResult[i];
            if (elem.first != lastPack || columnIndex == columns) {
                columnIndex = 0;
                lastPack    = elem.first;
                rowToFirstRowEntryFromSearch.push_back(i);
            }
            columnIndex++;
        }
    }

    endResetModel();
    emit newSearchString();
}
