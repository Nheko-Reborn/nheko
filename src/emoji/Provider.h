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

        Q_PROPERTY(const QString &unicode MEMBER unicode)
        Q_PROPERTY(const QString &shortName MEMBER shortName)
        Q_PROPERTY(emoji::Emoji::Category category MEMBER category)

public:
        QString unicode;
        QString shortName;
        Category category;
};

class Provider
{
public:
        // all emoji for QML purposes
        static const QVector<Emoji> emoji;
};

} // namespace emoji
Q_DECLARE_METATYPE(emoji::Emoji)
