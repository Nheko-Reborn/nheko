// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "MxcImageProvider.h"

#include <optional>

#include <mtxclient/crypto/client.hpp>

#include <QByteArray>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

#include "Logging.h"
#include "MatrixClient.h"
#include "Utils.h"

QHash<QString, mtx::crypto::EncryptedFile> infos;

QQuickImageResponse *
MxcImageProvider::requestImageResponse(const QString &id, const QSize &requestedSize)
{
        MxcImageResponse *response = new MxcImageResponse(id, requestedSize);
        pool.start(response);
        return response;
}

void
MxcImageProvider::addEncryptionInfo(mtx::crypto::EncryptedFile info)
{
        infos.insert(QString::fromStdString(info.url), info);
}
void
MxcImageResponse::run()
{
        MxcImageProvider::download(
          m_id, m_requestedSize, [this](QString, QSize, QImage image, QString) {
                  if (image.isNull()) {
                          m_error = "Failed to download image.";
                  } else {
                          m_image = image;
                  }
                  emit finished();
          });
}

void
MxcImageProvider::download(const QString &id,
                           const QSize &requestedSize,
                           std::function<void(QString, QSize, QImage, QString)> then)
{
        std::optional<mtx::crypto::EncryptedFile> encryptionInfo;
        auto temp = infos.find("mxc://" + id);
        if (temp != infos.end())
                encryptionInfo = *temp;

        if (requestedSize.isValid() && !encryptionInfo) {
                QString fileName =
                  QString("%1_%2x%3_crop")
                    .arg(QString::fromUtf8(id.toUtf8().toBase64(QByteArray::Base64UrlEncoding |
                                                                QByteArray::OmitTrailingEquals)))
                    .arg(requestedSize.width())
                    .arg(requestedSize.height());
                QFileInfo fileInfo(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) +
                                     "/media_cache",
                                   fileName);
                QDir().mkpath(fileInfo.absolutePath());

                if (fileInfo.exists()) {
                        QImage image(fileInfo.absoluteFilePath());
                        if (!image.isNull()) {
                                image = image.scaled(
                                  requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

                                if (!image.isNull()) {
                                        then(id, requestedSize, image, fileInfo.absoluteFilePath());
                                        return;
                                }
                        }
                }

                mtx::http::ThumbOpts opts;
                opts.mxc_url = "mxc://" + id.toStdString();
                opts.width   = requestedSize.width() > 0 ? requestedSize.width() : -1;
                opts.height  = requestedSize.height() > 0 ? requestedSize.height() : -1;
                opts.method  = "crop";
                http::client()->get_thumbnail(
                  opts,
                  [fileInfo, requestedSize, then, id](const std::string &res,
                                                      mtx::http::RequestErr err) {
                          if (err || res.empty()) {
                                  then(id, QSize(), {}, "");

                                  return;
                          }

                          auto data    = QByteArray(res.data(), (int)res.size());
                          QImage image = utils::readImage(data);
                          if (!image.isNull()) {
                                  image = image.scaled(
                                    requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                          }
                          image.setText("mxc url", "mxc://" + id);
                          if (image.save(fileInfo.absoluteFilePath(), "png"))
                                  nhlog::ui()->debug("Wrote: {}",
                                                     fileInfo.absoluteFilePath().toStdString());
                          else
                                  nhlog::ui()->debug("Failed to write: {}",
                                                     fileInfo.absoluteFilePath().toStdString());

                          then(id, requestedSize, image, fileInfo.absoluteFilePath());
                  });
        } else {
                try {
                        QString fileName = QString::fromUtf8(id.toUtf8().toBase64(
                          QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
                        QFileInfo fileInfo(
                          QStandardPaths::writableLocation(QStandardPaths::CacheLocation) +
                            "/media_cache",
                          fileName);
                        QDir().mkpath(fileInfo.absolutePath());

                        if (fileInfo.exists()) {
                                if (encryptionInfo) {
                                        QFile f(fileInfo.absoluteFilePath());
                                        f.open(QIODevice::ReadOnly);

                                        QByteArray fileData = f.readAll();
                                        auto temp =
                                          mtx::crypto::to_string(mtx::crypto::decrypt_file(
                                            fileData.toStdString(), encryptionInfo.value()));
                                        auto data    = QByteArray(temp.data(), (int)temp.size());
                                        QImage image = utils::readImage(data);
                                        image.setText("mxc url", "mxc://" + id);
                                        if (!image.isNull()) {
                                                then(id,
                                                     requestedSize,
                                                     image,
                                                     fileInfo.absoluteFilePath());
                                                return;
                                        }
                                } else {
                                        QImage image(fileInfo.absoluteFilePath());
                                        if (!image.isNull()) {
                                                then(id,
                                                     requestedSize,
                                                     image,
                                                     fileInfo.absoluteFilePath());
                                                return;
                                        }
                                }
                        }

                        http::client()->download(
                          "mxc://" + id.toStdString(),
                          [fileInfo, requestedSize, then, id, encryptionInfo](
                            const std::string &res,
                            const std::string &,
                            const std::string &originalFilename,
                            mtx::http::RequestErr err) {
                                  if (err) {
                                          then(id, QSize(), {}, "");
                                          return;
                                  }

                                  auto temp = res;
                                  QFile f(fileInfo.absoluteFilePath());
                                  if (!f.open(QIODevice::Truncate | QIODevice::WriteOnly)) {
                                          then(id, QSize(), {}, "");
                                          return;
                                  }
                                  f.write(temp.data(), temp.size());
                                  f.close();

                                  if (encryptionInfo) {
                                          temp = mtx::crypto::to_string(mtx::crypto::decrypt_file(
                                            temp, encryptionInfo.value()));
                                          auto data    = QByteArray(temp.data(), (int)temp.size());
                                          QImage image = utils::readImage(data);
                                          image.setText("original filename",
                                                        QString::fromStdString(originalFilename));
                                          image.setText("mxc url", "mxc://" + id);
                                          then(
                                            id, requestedSize, image, fileInfo.absoluteFilePath());
                                          return;
                                  }

                                  QImage image(fileInfo.absoluteFilePath());
                                  image.setText("original filename",
                                                QString::fromStdString(originalFilename));
                                  image.setText("mxc url", "mxc://" + id);
                                  image.save(fileInfo.absoluteFilePath());
                                  then(id, requestedSize, image, fileInfo.absoluteFilePath());
                          });
                } catch (std::exception &e) {
                        nhlog::net()->error("Exception while downloading media: {}", e.what());
                }
        }
}
