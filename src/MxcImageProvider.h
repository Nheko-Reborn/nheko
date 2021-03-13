// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QQuickAsyncImageProvider>
#include <QQuickImageResponse>

#include <QImage>
#include <QThreadPool>

#include <mtx/common.hpp>

#include <boost/optional.hpp>

class MxcImageResponse
  : public QQuickImageResponse
  , public QRunnable
{
public:
        MxcImageResponse(const QString &id,
                         const QSize &requestedSize,
                         boost::optional<mtx::crypto::EncryptedFile> encryptionInfo)
          : m_id(id)
          , m_requestedSize(requestedSize)
          , m_encryptionInfo(encryptionInfo)
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
        boost::optional<mtx::crypto::EncryptedFile> m_encryptionInfo;
};

class MxcImageProvider
  : public QObject
  , public QQuickAsyncImageProvider
{
        Q_OBJECT
public slots:
        QQuickImageResponse *requestImageResponse(const QString &id,
                                                  const QSize &requestedSize) override
        {
                boost::optional<mtx::crypto::EncryptedFile> info;
                auto temp = infos.find("mxc://" + id);
                if (temp != infos.end())
                        info = *temp;

                MxcImageResponse *response = new MxcImageResponse(id, requestedSize, info);
                pool.start(response);
                return response;
        }

        void addEncryptionInfo(mtx::crypto::EncryptedFile info)
        {
                infos.insert(QString::fromStdString(info.url), info);
        }

private:
        QThreadPool pool;
        QHash<QString, mtx::crypto::EncryptedFile> infos;
};
