// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QPalette>

class Nheko : public QObject
{
        Q_OBJECT

        Q_PROPERTY(QPalette colors READ colors NOTIFY colorsChanged)
        Q_PROPERTY(QPalette inactiveColors READ inactiveColors NOTIFY colorsChanged)
        Q_PROPERTY(int avatarSize READ avatarSize CONSTANT)
        Q_PROPERTY(int paddingSmall READ paddingSmall CONSTANT)
        Q_PROPERTY(int paddingMedium READ paddingMedium CONSTANT)
        Q_PROPERTY(int paddingLarge READ paddingLarge CONSTANT)

public:
        Nheko();

        QPalette colors() const;
        QPalette inactiveColors() const;

        int avatarSize() const { return 40; }

        int paddingSmall() const { return 4; }
        int paddingMedium() const { return 8; }
        int paddingLarge() const { return 20; }

        Q_INVOKABLE void openLink(QString link) const;

signals:
        void colorsChanged();
};

