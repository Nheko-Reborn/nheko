#include <QQuickImageProvider>

class BlurhashProvider : public QQuickImageProvider
{
public:
        BlurhashProvider()
          : QQuickImageProvider(QQuickImageProvider::Image)
        {}

        QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;
};
