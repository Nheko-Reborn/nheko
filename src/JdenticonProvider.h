// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QImage>
#include <QQuickAsyncImageProvider>
#include <QQuickImageResponse>
#include <QThreadPool>

class JdenticonRunnable final
  : public QObject
  , public QRunnable
{
    Q_OBJECT
public:
    JdenticonRunnable(const QString &key, double radius, const QSize &requestedSize);

    void run() override;

signals:
    void done(QImage img);

private:
    QString m_key;
    double m_radius;
    QSize m_requestedSize;
};

class JdenticonResponse final : public QQuickImageResponse
{
public:
    JdenticonResponse(const QString &key, double radius, const QSize &requestedSize);

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

class JdenticonProvider : public QQuickAsyncImageProvider
{
    Q_OBJECT

public:
    static bool isAvailable();

public slots:
    QQuickImageResponse *
    requestImageResponse(const QString &id, const QSize &requestedSize) override
    {
        auto id_      = id;
        double radius = 0;

        auto queryStart = id.lastIndexOf('?');
        if (queryStart != -1) {
            id_            = id.left(queryStart);
            auto query     = id.mid(queryStart + 1);
            auto queryBits = QStringView(query).split('&');

            for (const auto &b : std::as_const(queryBits)) {
                if (b.startsWith(QStringView(u"radius="))) {
                    radius = b.mid(7).toDouble();
                }
            }
        }

        return new JdenticonResponse(id_, radius, requestedSize);
    }
};
