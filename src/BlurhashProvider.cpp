#include "BlurhashProvider.h"

#include <algorithm>

#include <QUrl>

#include "blurhash.hpp"

void
BlurhashResponse::run()
{
        if (m_requestedSize.width() < 0 || m_requestedSize.height() < 0) {
                m_error = QStringLiteral("Blurhash needs size request");
                emit finished();
                return;
        }
        if (m_requestedSize.width() == 0 || m_requestedSize.height() == 0) {
                m_image = QImage(m_requestedSize, QImage::Format_RGB32);
                m_image.fill(QColor(0, 0, 0));
                emit finished();
                return;
        }

        auto decoded = blurhash::decode(QUrl::fromPercentEncoding(m_id.toUtf8()).toStdString(),
                                        m_requestedSize.width(),
                                        m_requestedSize.height(),
                                        4);
        if (decoded.image.empty()) {
                m_error = QStringLiteral("Failed decode!");
                emit finished();
                return;
        }

        QImage image(decoded.image.data(), decoded.width, decoded.height, QImage::Format_RGB32);

        m_image = image.copy();
        emit finished();
}
