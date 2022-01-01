// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QQuickAsyncImageProvider>
#include <QQuickImageResponse>

#include <QImage>
#include <QThreadPool>

class BlurhashRunnable
  : public QObject
  , public QRunnable
{
    Q_OBJECT
public:
    BlurhashRunnable(const QString &id, const QSize &requestedSize)
      : m_id(id)
      , m_requestedSize(requestedSize)
    {}

    void run() override;
signals:
    void done(QImage);
    void error(QString);

private:
    QString m_id;
    QSize m_requestedSize;
};

class BlurhashResponse : public QQuickImageResponse
{
public:
    BlurhashResponse(const QString &id, const QSize &requestedSize)
    {
        auto runnable = new BlurhashRunnable(id, requestedSize);
        connect(runnable, &BlurhashRunnable::done, this, &BlurhashResponse::handleDone);
        connect(runnable, &BlurhashRunnable::error, this, &BlurhashResponse::handleError);
        QThreadPool::globalInstance()->start(runnable);
    }

    QQuickTextureFactory *textureFactory() const override
    {
        return QQuickTextureFactory::textureFactoryForImage(m_image);
    }
    QString errorString() const override { return m_error; }

    void handleDone(QImage image)
    {
        m_image = std::move(image);
        emit finished();
    }
    void handleError(QString error)
    {
        m_error = error;
        emit finished();
    }

    QString m_error;
    QImage m_image;
};

class BlurhashProvider
  :
#if QT_VERSION < 0x60000
  public QObject
  ,
#endif
  public QQuickAsyncImageProvider
{
    Q_OBJECT
public slots:
    QQuickImageResponse *
    requestImageResponse(const QString &id, const QSize &requestedSize) override
    {
        return new BlurhashResponse(id, requestedSize);
    }
};
