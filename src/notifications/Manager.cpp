#include "notifications/Manager.h"

#include "Cache.h"
#include "EventAccessors.h"
#include "Logging.h"
#include "MatrixClient.h"
#include "Utils.h"

#include <QFile>
#include <QImage>
#include <QStandardPaths>

#include <mtxclient/crypto/client.hpp>

QString
NotificationsManager::cacheImage(const mtx::events::collections::TimelineEvents &event)
{
        const auto url      = mtx::accessors::url(event);
        auto encryptionInfo = mtx::accessors::file(event);

        auto filename = QString::fromStdString(mtx::accessors::body(event));
        QString path{QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/" +
                     filename};

        http::client()->download(
          url,
          [&path, url, encryptionInfo](const std::string &data,
                                      const std::string &,
                                      const std::string &,
                                      mtx::http::RequestErr err) {
                  if (err) {
                          nhlog::net()->warn("failed to retrieve image {}: {} {}",
                                             url,
                                             err->matrix_error.error,
                                             static_cast<int>(err->status_code));
                          return;
                  }

                  try {
                          auto temp = data;
                          if (encryptionInfo)
                                  temp = mtx::crypto::to_string(
                                    mtx::crypto::decrypt_file(temp, encryptionInfo.value()));

                          QFile file{path};

                          if (!file.open(QIODevice::WriteOnly))
                                  return;

                          // delete any existing file content
                          file.resize(0);

                          // resize the image
                          QImage img{utils::readImage(QByteArray{temp.data()})};

                          if (img.isNull())
                          {
                              path.clear();
                              return;
                          }

#ifdef NHEKO_DBUS_SYS // the images in D-Bus notifications are to be 200x100 max
                          img.scaled(200, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation)
                            .save(&file);
#else
                          img.save(&file);
#endif // NHEKO_DBUS_SYS

                          file.close();

                          return;
                  } catch (const std::exception &e) {
                          nhlog::ui()->warn("Error while caching file to: {}", e.what());
                  }
          });

        return path.toHtmlEscaped();
}
