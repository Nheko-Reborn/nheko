// SPDX-FileCopyrightText: 2017 Konstantinos Sideris <siderisk@auth.gr>
// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

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
