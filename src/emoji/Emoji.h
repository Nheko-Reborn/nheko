// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

namespace emoji {
Q_NAMESPACE

struct Emoji
{
    Q_GADGET
public:
    enum class Category
    {
        People,
        Nature,
        Food,
        Activity,
        Travel,
        Objects,
        Symbols,
        Flags,
        Search
    };
    Q_ENUM(Category)

    Q_PROPERTY(QString unicode READ unicode CONSTANT)
    Q_PROPERTY(QString shortName READ shortName CONSTANT)
    Q_PROPERTY(QString unicodeName READ unicodeName CONSTANT)
    Q_PROPERTY(emoji::Emoji::Category category MEMBER category)

public:
    constexpr Emoji(std::u16string_view unicode,
                    std::u16string_view shortName,
                    std::u16string_view unicodeName,
                    Category cat)
      : unicode_(unicode)
      , shortName_(shortName)
      , unicodeName_(unicodeName)
      , category(cat)
    {
    }

    constexpr Emoji()
      : unicode_(u"", 0)
      , shortName_(u"", 0)
      , unicodeName_(u"", 0)
      , category(Category::Search)
    {
    }

    constexpr Emoji(const Emoji &) = default;
    constexpr Emoji(Emoji &&)      = default;

    constexpr Emoji &operator=(const Emoji &) = default;
    constexpr Emoji &operator=(Emoji &&)      = default;

    QString unicode() const
    {
        return QString::fromRawData(reinterpret_cast<const QChar *>(unicode_.data()),
                                    unicode_.size());
    }
    QString shortName() const
    {
        return QString::fromRawData(reinterpret_cast<const QChar *>(shortName_.data()),
                                    shortName_.size());
    }
    QString unicodeName() const
    {
        return QString::fromRawData(reinterpret_cast<const QChar *>(unicodeName_.data()),
                                    unicodeName_.size());
    }

private:
    std::u16string_view unicode_;
    std::u16string_view shortName_;
    std::u16string_view unicodeName_;

public:
    Category category;
};

QString
categoryToName(emoji::Emoji::Category cat);
} // namespace emoji
