// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QImage>
#include <QQuickAsyncImageProvider>
#include <QQuickImageResponse>
#include <QThreadPool>

#include <mtx/common.hpp>

#include "jdenticoninterface.h"

class JdenticonRunnable
  : public QObject
  , public QRunnable
{
    Q_OBJECT
public:
    JdenticonRunnable(const QString &key, bool crop, double radius, const QSize &requestedSize);

    void run() override;

signals:
    void done(QImage img);

private:
    QString m_key;
    bool m_crop;
    double m_radius;
    QSize m_requestedSize;
};

class JdenticonResponse : public QQuickImageResponse
{
public:
    JdenticonResponse(const QString &key, bool crop, double radius, const QSize &requestedSize);

    QQuickTextureFactory *textureFactory() const override
    {
        return QQuickTextureFactory::textureFactoryForImage(m_pixmap);
    }

    void handleDone(QImage img)
    {
        m_pixmap = std::move(img);
        emit finished();
    }

    QImage m_pixmap;
};

class JdenticonProvider
  :
#if QT_VERSION < 0x60000
  public QObject
  ,
#endif
  public QQuickAsyncImageProvider
{
    Q_OBJECT

public:
    static bool isAvailable();

public slots:
    QQuickImageResponse *
    requestImageResponse(const QString &id, const QSize &requestedSize) override
    {
        auto id_      = id;
        bool crop     = true;
        double radius = 0;

        auto queryStart = id.lastIndexOf('?');
        if (queryStart != -1) {
            id_            = id.left(queryStart);
            auto query     = id.mid(queryStart + 1);
            auto queryBits = query.splitRef('&');

            for (const auto &b : queryBits) {
                if (b.startsWith(QStringView(u"radius="))) {
                    radius = b.mid(7).toDouble();
                }
            }
        }

        return new JdenticonResponse(id_, crop, radius, requestedSize);
    }
};
