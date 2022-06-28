// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ColorImageProvider.h"

#include <QIcon>
#include <QPainter>

QPixmap
ColorImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &req)
{
    auto args = id.split('?');

    QPixmap source(args[0]);

    if (size)
        *size = QSize(source.width(), source.height());

    if (req.width() > 0 && req.height() > 0)
        source = QIcon(args[0]).pixmap(req);
    if (args.size() < 2)
        return source;

    QColor color(args[1]);

    QPixmap colorized = source;
    QPainter painter(&colorized);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(colorized.rect(), color);
    painter.end();

    return colorized;
}
