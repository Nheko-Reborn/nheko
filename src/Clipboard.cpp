// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Clipboard.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QMimeData>
#include <QPixmap>

Clipboard::Clipboard(QObject *parent)
  : QObject(parent)
{
    connect(QGuiApplication::clipboard(), &QClipboard::dataChanged, this, &Clipboard::textChanged);
}

void
Clipboard::setText(QString text)
{
    QMimeData *mimeData = new QMimeData();
    mimeData->setText(text);

    QClipboard *clipboard = QGuiApplication::clipboard();
    if (mimeData->hasImage()) {
        clipboard->setPixmap(qvariant_cast<QPixmap>(mimeData->imageData()));
    } else if (mimeData->hasHtml()) {
        clipboard->setText(mimeData->html());
    } else if (mimeData->hasText()) {
        clipboard->setText(mimeData->text());
    }
}

QString
Clipboard::text() const
{
    return QGuiApplication::clipboard()->text();
}

#include "moc_Clipboard.cpp"
