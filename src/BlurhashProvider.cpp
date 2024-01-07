// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "BlurhashProvider.h"

#include <QUrl>

#include "blurhash.hpp"

void
BlurhashRunnable::run()
{
    if (m_requestedSize.width() < 0 || m_requestedSize.height() < 0) {
        emit error(QStringLiteral("Blurhash needs size request"));
        return;
    }
    if (m_requestedSize.width() == 0 || m_requestedSize.height() == 0) {
        auto image = QImage(m_requestedSize, QImage::Format_RGB32);
        image.fill(QColor(0, 0, 0));
        emit done(image);
        return;
    }

    auto blurhashDecodeSize = m_requestedSize;
    if (blurhashDecodeSize.height() > 100 && blurhashDecodeSize.width() > 100) {
        blurhashDecodeSize.scale(100, 100, Qt::AspectRatioMode::KeepAspectRatio);
    }

    auto decoded = blurhash::decode(QUrl::fromPercentEncoding(m_id.toUtf8()).toStdString(),
                                    blurhashDecodeSize.width(),
                                    blurhashDecodeSize.height());
    if (decoded.image.empty()) {
        emit error(QStringLiteral("Failed decode!"));
        return;
    }

    QImage image(decoded.image.data(),
                 (int)decoded.width,
                 (int)decoded.height,
                 (int)decoded.width * 3,
                 QImage::Format_RGB888);

    image = image.scaled(m_requestedSize);

    emit done(image.convertToFormat(QImage::Format_RGB32));
}
