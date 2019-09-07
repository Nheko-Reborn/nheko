#pragma once

#include <QQuickAsyncImageProvider>
#include <QQuickImageResponse>

#include <QImage>
#include <QThreadPool>

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

class MxcImageProvider : public QQuickAsyncImageProvider
{
public:
        QQuickImageResponse *requestImageResponse(const QString &id,
                                                  const QSize &requestedSize) override
        {
                MxcImageResponse *response = new MxcImageResponse(id, requestedSize);
                pool.start(response);
                return response;
        }

private:
        QThreadPool pool;
};

