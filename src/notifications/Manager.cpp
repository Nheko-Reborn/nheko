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
          [path, url, encryptionInfo](const std::string &data,
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
                          file.write(QByteArray(temp.data(), (int)temp.size()));

                          // resize the image (really inefficient, I know, but I can't find any
                          // better way right off
                          QImage img{path};

                          // delete existing contents
                          file.resize(0);

                          // make sure to save as PNG (because Plasma doesn't do JPEG in
                          // notifications)
                          //                          if (!file.fileName().endsWith(".png"))
                          //                                  file.rename(file.fileName() + ".png");

                          img.scaled(200, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation)
                            .save(&file);
                          file.close();

                          return;
                  } catch (const std::exception &e) {
                          nhlog::ui()->warn("Error while caching file to: {}", e.what());
                  }
          });

        return path.toHtmlEscaped();
}
