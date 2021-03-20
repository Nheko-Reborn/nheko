// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QQuickAsyncImageProvider>
#include <QQuickImageResponse>

#include <QImage>
#include <QThreadPool>

#include <functional>

#include <mtx/common.hpp>

class MxcImageResponse
  : public QQuickImageResponse
  , public QRunnable
{
public:
        MxcImageResponse(const QString &id, const QSize &requestedSize)
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

class MxcImageProvider
  : public QObject
  , public QQuickAsyncImageProvider
{
        Q_OBJECT
public slots:
        QQuickImageResponse *requestImageResponse(const QString &id,
                                                  const QSize &requestedSize) override;

        static void addEncryptionInfo(mtx::crypto::EncryptedFile info);
        static void download(const QString &id,
                             const QSize &requestedSize,
                             std::function<void(QString, QSize, QImage, QString)> then);

private:
        QThreadPool pool;
};
