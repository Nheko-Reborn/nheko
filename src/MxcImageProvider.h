// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QQuickAsyncImageProvider>
#include <QQuickImageResponse>

#include <QImage>
//#include <QThreadPool>

#include <functional>

#include <mtx/common.hpp>

class MxcImageRunnable : public QObject
{
    Q_OBJECT

signals:
    void done(QImage image);
    void error(QString error);

public:
    MxcImageRunnable(const QString &id, bool crop, double radius, const QSize &requestedSize)
      : m_id(id)
      , m_requestedSize(requestedSize)
      , m_crop(crop)
      , m_radius(radius)
    {}

    void run();

    QString m_id;
    QSize m_requestedSize;
    bool m_crop;
    double m_radius;
};
class MxcImageResponse : public QQuickImageResponse
{
public:
    MxcImageResponse(const QString &id, bool crop, double radius, const QSize &requestedSize)

    {
        auto runnable = new MxcImageRunnable(id, crop, radius, requestedSize);
        connect(runnable, &MxcImageRunnable::done, this, &MxcImageResponse::handleDone);
        connect(runnable, &MxcImageRunnable::error, this, &MxcImageResponse::handleError);
        runnable->run();
    }

    void handleDone(QImage image)
    {
        m_image = image;
        emit finished();
    }
    void handleError(QString error)
    {
        m_error = error;
        emit finished();
    }

    QQuickTextureFactory *textureFactory() const override
    {
        return QQuickTextureFactory::textureFactoryForImage(m_image);
    }
    QString errorString() const override { return m_error; }

    QString m_error;
    QImage m_image;
};

class MxcImageProvider
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
    requestImageResponse(const QString &id, const QSize &requestedSize) override;

    static void addEncryptionInfo(mtx::crypto::EncryptedFile info);
    static void download(const QString &id,
                         const QSize &requestedSize,
                         std::function<void(QString, QSize, QImage, QString)> then,
                         bool crop     = true,
                         double radius = 0);
};
