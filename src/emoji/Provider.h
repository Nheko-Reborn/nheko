/*
 * nheko Copyright (C) 2017  Konstantinos Sideris <siderisk@auth.gr>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <QObject>
#include <QSet>
#include <QString>
#include <QVector>
#include <vector>

namespace emoji {

class Emoji
{
        Q_GADGET

        Q_PROPERTY(const QString &unicode READ unicode CONSTANT)
        Q_PROPERTY(const QString &shortName READ shortName CONSTANT)
        Q_PROPERTY(emoji::Emoji::Category category READ category CONSTANT)

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

        Emoji(const QString &unicode   = {},
              const QString &shortName = {},
              Emoji::Category cat      = Emoji::Category::Search)
          : unicode_(unicode)
          , shortName_(shortName)
          , category_(cat)
        {}

        inline const QString &unicode() const { return unicode_; }
        inline const QString &shortName() const { return shortName_; }
        inline Emoji::Category category() const { return category_; }
        inline void setUnicode(const QString &unicode) { unicode_ = unicode; }

private:
        QString unicode_;
        QString shortName_;
        Emoji::Category category_;
};

class Provider
{
public:
        // all emoji for QML purposes
        static const QVector<Emoji> emoji;
        static const std::vector<Emoji> people;
        static const std::vector<Emoji> nature;
        static const std::vector<Emoji> food;
        static const std::vector<Emoji> activity;
        static const std::vector<Emoji> travel;
        static const std::vector<Emoji> objects;
        static const std::vector<Emoji> symbols;
        static const std::vector<Emoji> flags;
};

} // namespace emoji
Q_DECLARE_METATYPE(emoji::Emoji)
