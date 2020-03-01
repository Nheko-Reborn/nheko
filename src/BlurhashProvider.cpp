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
          QUrl::fromPercentEncoding(id.toUtf8()).toStdString(), sz.width(), sz.height());
        if (decoded.image.empty()) {
                *size = QSize();
                return QImage();
        }

        QImage image(sz, QImage::Format_RGB888);

        for (int y = 0; y < sz.height(); y++) {
                for (int x = 0; x < sz.width(); x++) {
                        int base = (y * sz.width() + x) * 3;
                        image.setPixel(x,
                                       y,
                                       qRgb(decoded.image[base],
                                            decoded.image[base + 1],
                                            decoded.image[base + 2]));
                }
        }

        // std::copy(decoded.image.begin(), decoded.image.end(), image.bits());

        return image;
}
