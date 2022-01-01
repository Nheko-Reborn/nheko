// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "BlurhashProvider.h"

#include <algorithm>

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

    auto decoded = blurhash::decode(QUrl::fromPercentEncoding(m_id.toUtf8()).toStdString(),
                                    m_requestedSize.width(),
                                    m_requestedSize.height());
    if (decoded.image.empty()) {
        emit error(QStringLiteral("Failed decode!"));
        return;
    }

    QImage image(decoded.image.data(),
                 (int)decoded.width,
                 (int)decoded.height,
                 (int)decoded.width * 3,
                 QImage::Format_RGB888);

    emit done(image.convertToFormat(QImage::Format_RGB32));
}
