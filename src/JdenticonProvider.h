// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QImage>
#include <QQuickAsyncImageProvider>
#include <QQuickImageResponse>
#include <QThreadPool>

#include <mtx/common.hpp>

#include "jdenticoninterface.h"

namespace Jdenticon {
JdenticonInterface *
getJdenticonInterface();
}

class JdenticonResponse
  : public QQuickImageResponse
  , public QRunnable
{
public:
        JdenticonResponse(const QString &key, bool crop, double radius, const QSize &requestedSize);

        QQuickTextureFactory *textureFactory() const override
        {
                return QQuickTextureFactory::textureFactoryForImage(m_pixmap.toImage());
        }

        void run() override;

        QString m_key;
        bool m_crop;
        double m_radius;
        QSize m_requestedSize;
        QPixmap m_pixmap;
        JdenticonInterface *jdenticonInterface_ = nullptr;
};

class JdenticonProvider
  : public QObject
  , public QQuickAsyncImageProvider
{
        Q_OBJECT

public:
        static bool isAvailable() { return Jdenticon::getJdenticonInterface() != nullptr; }

public slots:
        QQuickImageResponse *requestImageResponse(const QString &id,
                                                  const QSize &requestedSize) override
        {
                auto id_      = id;
                bool crop     = true;
                double radius = 0;

                auto queryStart = id.lastIndexOf('?');
                if (queryStart != -1) {
                        id_            = id.left(queryStart);
                        auto query     = id.midRef(queryStart + 1);
                        auto queryBits = query.split('&');

                        for (auto b : queryBits) {
                                if (b == "scale") {
                                        crop = false;
                                } else if (b.startsWith("radius=")) {
                                        radius = b.mid(7).toDouble();
                                }
                        }
                }

                JdenticonResponse *response =
                  new JdenticonResponse(id_, crop, radius, requestedSize);
                pool.start(response);
                return response;
        }

private:
        QThreadPool pool;
};
