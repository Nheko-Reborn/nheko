// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QQuickAsyncImageProvider>
#include <QQuickImageResponse>

#include <QImage>
#include <QThreadPool>

class BlurhashResponse
  : public QQuickImageResponse
  , public QRunnable
{
public:
    BlurhashResponse(const QString &id, const QSize &requestedSize)

      : m_id(id)
      , m_requestedSize(requestedSize)
    {
        setAutoDelete(false);
    }

    QQuickTextureFactory *textureFactory() const override
    {
        return QQuickTextureFactory::textureFactoryForImage(m_image);
    }
    QString errorString() const override { return m_error; }

    void run() override;

    QString m_id, m_error;
    QSize m_requestedSize;
    QImage m_image;
};

class BlurhashProvider
  : public QObject
  , public QQuickAsyncImageProvider
{
    Q_OBJECT
public slots:
    QQuickImageResponse *
    requestImageResponse(const QString &id, const QSize &requestedSize) override
    {
        BlurhashResponse *response = new BlurhashResponse(id, requestedSize);
        pool.start(response);
        return response;
    }

private:
    QThreadPool pool;
};
