#include "BlurhashProvider.h"

#include <algorithm>

#include <QUrl>

#include "blurhash.hpp"

QImage
BlurhashProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
        QSize sz = requestedSize;
        if (sz.width() < 1 || sz.height() < 1)
                return QImage();

        if (size)
                *size = sz;

        auto decoded = blurhash::decode(
          QUrl::fromPercentEncoding(id.toUtf8()).toStdString(), sz.width(), sz.height(), 4);
        if (decoded.image.empty()) {
                *size = QSize();
                return QImage();
        }

        QImage image(decoded.image.data(), decoded.width, decoded.height, QImage::Format_RGB32);

        return image.copy();
}
