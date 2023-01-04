// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
// SPDX-FileCopyrightText: 2023 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QColor>
#include <QPalette>

class Theme final : public QPalette
{
    Q_GADGET
    Q_PROPERTY(QColor sidebarBackground READ sidebarBackground CONSTANT)
    Q_PROPERTY(QColor alternateButton READ alternateButton CONSTANT)
    Q_PROPERTY(QColor separator READ separator CONSTANT)
    Q_PROPERTY(QColor red READ red CONSTANT)
    Q_PROPERTY(QColor green READ green CONSTANT)
    Q_PROPERTY(QColor error READ error CONSTANT)
    Q_PROPERTY(QColor orange READ orange CONSTANT)
    Q_PROPERTY(QColor online READ online CONSTANT)
    Q_PROPERTY(QColor unavailable READ unavailable CONSTANT)
public:
    Theme() {}
    explicit Theme(QStringView theme);
    static QPalette paletteFromTheme(QStringView theme);

    QColor sidebarBackground() const { return sidebarBackground_; }
    QColor alternateButton() const { return alternateButton_; }
    QColor separator() const { return separator_; }
    QColor red() const { return red_; }
    QColor green() const { return green_; }
    QColor error() const { return error_; }
    QColor orange() const { return orange_; }
    QColor online() const { return QColor(0x00, 0xcc, 0x66); }
    QColor unavailable() const { return QColor(0xff, 0x99, 0x33); }

private:
    QColor sidebarBackground_, separator_, red_, green_, error_, orange_, alternateButton_;
};
