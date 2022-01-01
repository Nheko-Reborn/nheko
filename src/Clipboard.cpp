// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Clipboard.h"

#include <QClipboard>
#include <QGuiApplication>

Clipboard::Clipboard(QObject *parent)
  : QObject(parent)
{
    connect(QGuiApplication::clipboard(), &QClipboard::dataChanged, this, &Clipboard::textChanged);
}

void
Clipboard::setText(QString text)
{
    QGuiApplication::clipboard()->setText(text);
}

QString
Clipboard::text() const
{
    return QGuiApplication::clipboard()->text();
}
