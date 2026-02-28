// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "MxcImageProvider.h"

#include <optional>

#include <mtx/common.hpp>
#include <mtxclient/crypto/client.hpp>

#include <QByteArray>
#include <QCache>
#include <QDir>
#include <QFileInfo>
#include <QPainter>
#include <QPainterPath>
#include <QStandardPaths>
#include <QThreadPool>
#include <QTimer>

#include "Logging.h"
#include "MatrixClient.h"
#include "Utils.h"

QHash<QString, mtx::crypto::EncryptedFile> infos;

MxcImageProvider::MxcImageProvider()
  : QQuickAsyncImageProvider()
{
    auto timer = new QTimer(this);
    timer->setInterval(std::chrono::hours(1));
    connect(timer, &QTimer::timeout, this, [] {
        QThreadPool::globalInstance()->start([] {
            nhlog::net()->debug("Running media purge");
            QDir dir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) +
                       "/media_cache",
                     "",
                     QDir::SortFlags(QDir::Name | QDir::IgnoreCase),
                     QDir::Filter::Writable | QDir::Filter::NoDotAndDotDot | QDir::Filter::Files |
                       QDir::Filter::Dirs);

            auto handleFile = [](const QFileInfo &fileInfo) {
                if (fileInfo.fileTime(QFile::FileTime::FileAccessTime)
                      .daysTo(QDateTime::currentDateTime()) > 14) {
                    if (QFile::remove(fileInfo.absoluteFilePath()))
                        nhlog::net()->debug("Deleted stale media '{}'",
                                            fileInfo.absoluteFilePath().toStdString());
                    else
                        nhlog::net()->warn("Failed to delete stale media '{}'",
                                           fileInfo.absoluteFilePath().toStdString());
                }
            };

            auto files = dir.entryInfoList();
            for (const auto &fileInfo : std::as_const(files)) {
                if (fileInfo.isDir()) {
                    // handle one level of legacy directories
                    auto nestedDir = QDir(fileInfo.absoluteFilePath(),
                                          "",
                                          QDir::SortFlags(QDir::Name | QDir::IgnoreCase),
                                          QDir::Filter::Writable | QDir::Filter::NoDotAndDotDot |
                                            QDir::Filter::Files)
                                       .entryInfoList();
                    for (const auto &nestedFile : std::as_const(nestedDir)) {
                        handleFile(nestedFile);
                    }
                } else {
                    handleFile(fileInfo);
                }
            }
        });
    });
    timer->start();
}

QQuickImageResponse *
MxcImageProvider::requestImageResponse(const QString &id, const QSize &requestedSize)
{
    auto id_      = id;
    bool crop     = true;
    double radius = 0;
    auto size     = requestedSize;

    if (requestedSize.width() == 0 && requestedSize.height() == 0)
        size = QSize();

    auto queryStart = id.lastIndexOf('?');
    if (queryStart != -1) {
        id_            = id.left(queryStart);
        auto query     = QStringView(id).mid(queryStart + 1);
        auto queryBits = query.split('&');

        for (auto b : std::as_const(queryBits)) {
            if (b == QStringView(u"scale")) {
                crop = false;
            } else if (b.startsWith(QStringView(u"radius="))) {
                radius = b.mid(7).toDouble();
            } else if (b.startsWith(u"height=")) {
                size.setHeight(b.mid(7).toInt());
                size.setWidth(0);
            }
        }
    }

    return new MxcImageResponse(id_, crop, radius, size);
}

void
MxcImageProvider::addEncryptionInfo(const mtx::crypto::EncryptedFile &info)
{
    infos.insert(QString::fromStdString(info.url), info);
}
void
MxcImageRunnable::run()
{
    MxcImageProvider::download(
      m_id,
      m_requestedSize,
      [this](QString id, QSize, QImage image, QString) {
          if (image.isNull()) {
              emit error(QStringLiteral("Failed to download image: %1").arg(id));
          } else {
              emit done(image);
          }
          this->deleteLater();
      },
      m_crop,
      m_radius);
}

static QImage
clipRadius(QImage img, double radius)
{
    QImage out(img.size(), QImage::Format_ARGB32_Premultiplied);
    out.fill(Qt::transparent);

    QPainter painter(&out);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    QPainterPath ppath;
    ppath.addRoundedRect(img.rect(), radius, radius, Qt::SizeMode::RelativeSize);

    painter.setClipPath(ppath);
    painter.drawImage(img.rect(), img);

    return out;
}

static void
possiblyUpdateAccessTime(const QFileInfo &fileInfo)
{
    if (fileInfo.fileTime(QFile::FileTime::FileAccessTime).daysTo(QDateTime::currentDateTime()) >
        7) {
        nhlog::net()->debug("Updating file time for '{}'",
                            fileInfo.absoluteFilePath().toStdString());

        QFile f(fileInfo.absoluteFilePath());

        if (!f.open(QIODevice::ReadWrite) ||
            !f.setFileTime(QDateTime::currentDateTime(), QFile::FileTime::FileAccessTime)) {
            nhlog::net()->warn("Failed to update filetime for '{}'",
                               fileInfo.absoluteFilePath().toStdString());
        }
    }
}

void
MxcImageProvider::download(const QString &id,
                           const QSize &requestedSize,
                           std::function<void(QString, QSize, QImage, QString)> then,
                           bool crop,
                           double radius)
{
    if (id.isEmpty()) {
        nhlog::net()->warn("Attempted to download image with empty ID");
        then(id, QSize{}, QImage{}, QString{});
        return;
    }

    bool cropLocally = false;
    if (crop && requestedSize.width() > 96) {
        crop        = false;
        cropLocally = true;
    }

    std::optional<mtx::crypto::EncryptedFile> encryptionInfo;
    auto temp = infos.find("mxc://" + id);
    if (temp != infos.end())
        encryptionInfo = *temp;

    if (requestedSize.isValid() &&
        !encryptionInfo
        // Protect against synapse not following the spec:
        // https://github.com/matrix-org/synapse/issues/5302
        && requestedSize.height() <= 600 && requestedSize.width() <= 800) {
        QString fileName = QStringLiteral("%1_%2x%3_%4_radius%5")
                             .arg(QString::fromUtf8(id.toUtf8().toBase64(
                               QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals)))
                             .arg(requestedSize.width())
                             .arg(requestedSize.height())
                             .arg(crop ? "crop" : "scale")
                             .arg(radius);
        QFileInfo fileInfo(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) +
                             "/media_cache",
                           fileName);
        QDir().mkpath(fileInfo.absolutePath());

        if (fileInfo.exists()) {
            QImage image = utils::readImageFromFile(fileInfo.absoluteFilePath());
            if (!image.isNull()) {
                possiblyUpdateAccessTime(fileInfo);

                if (requestedSize.width() <= 0) {
                    image = image.scaledToHeight(requestedSize.height(), Qt::SmoothTransformation);
                } else {
                    image = image.scaled(requestedSize,
                                         cropLocally ? Qt::KeepAspectRatioByExpanding
                                                     : Qt::KeepAspectRatio,
                                         Qt::SmoothTransformation);
                    if (cropLocally) {
                        image = image.copy((image.width() - requestedSize.width()) / 2,
                                           (image.height() - requestedSize.height()) / 2,
                                           requestedSize.width(),
                                           requestedSize.height());
                    }
                }

                if (radius != 0) {
                    image = clipRadius(std::move(image), radius);
                }

                if (!image.isNull()) {
                    then(id, requestedSize, image, fileInfo.absoluteFilePath());
                    return;
                }
            }
        }

        mtx::http::ThumbOpts opts;
        opts.mxc_url = "mxc://" + id.toStdString();
        opts.width = static_cast<uint16_t>(requestedSize.width() > 0 ? requestedSize.width() : -1);
        opts.height =
          static_cast<uint16_t>(requestedSize.height() > 0 ? requestedSize.height() : -1);
        opts.method = crop ? "crop" : "scale";
        http::client()->get_thumbnail(
          opts,
          [fileInfo, requestedSize, radius, then, id, crop, cropLocally](
            const std::string &res, mtx::http::RequestErr err) {
              if (err || res.empty()) {
                  if (err)
                      nhlog::net()->warn(
                        "Failed to download thumbnail for mxc://{}: {}", id.toStdString(), *err);
                  else
                      nhlog::net()->warn(
                        "Failed to download thumbnail for mxc://{}: empty response",
                        id.toStdString());
                  download(id, QSize(), then, crop, radius);
                  return;
              }

              auto data    = QByteArray(res.data(), (int)res.size());
              QImage image = utils::readImage(data);
              if (!image.isNull()) {
                  possiblyUpdateAccessTime(fileInfo);

                  if (requestedSize.width() <= 0) {
                      image =
                        image.scaledToHeight(requestedSize.height(), Qt::SmoothTransformation);
                  } else {
                      image = image.scaled(requestedSize,
                                           cropLocally ? Qt::KeepAspectRatioByExpanding
                                                       : Qt::KeepAspectRatio,
                                           Qt::SmoothTransformation);
                      if (cropLocally) {
                          image = image.copy((image.width() - requestedSize.width()) / 2,
                                             (image.height() - requestedSize.height()) / 2,
                                             requestedSize.width(),
                                             requestedSize.height());
                      }
                  }

                  if (radius != 0) {
                      image = clipRadius(std::move(image), radius);
                  }
              }
              image.setText(QStringLiteral("mxc url"), "mxc://" + id);
              if (image.save(fileInfo.absoluteFilePath(), "png")) {
                  utils::markFileAsFromWeb(fileInfo.absoluteFilePath());
                  nhlog::ui()->debug("Wrote: {}", fileInfo.absoluteFilePath().toStdString());
              } else
                  nhlog::ui()->debug("Failed to write: {}",
                                     fileInfo.absoluteFilePath().toStdString());

              then(id, requestedSize, image, fileInfo.absoluteFilePath());
          });
    } else {
        try {
            QString fileName = QStringLiteral("%1_radius%2")
                                 .arg(QString::fromUtf8(id.toUtf8().toBase64(
                                   QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals)))
                                 .arg(radius);

            QFileInfo fileInfo(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) +
                                 "/media_cache",
                               fileName);
            QDir().mkpath(fileInfo.absolutePath());
            QFile f(fileInfo.absoluteFilePath());

            if (fileInfo.exists() && f.open(QIODevice::ReadOnly)) {
                if (encryptionInfo) {
                    QByteArray fileData = f.readAll();
                    auto tempData       = mtx::crypto::to_string(
                      mtx::crypto::decrypt_file(fileData.toStdString(), encryptionInfo.value()));
                    auto data    = QByteArray(tempData.data(), (int)tempData.size());
                    QImage image = utils::readImage(data);
                    image.setText(QStringLiteral("mxc url"), "mxc://" + id);
                    if (!image.isNull()) {
                        possiblyUpdateAccessTime(fileInfo);
                        if (radius != 0) {
                            image = clipRadius(std::move(image), radius);
                        }

                        then(id, requestedSize, image, fileInfo.absoluteFilePath());
                        return;
                    }
                } else {
                    QImage image = utils::readImageFromFile(fileInfo.absoluteFilePath());
                    if (!image.isNull()) {
                        possiblyUpdateAccessTime(fileInfo);
                        if (radius != 0) {
                            image = clipRadius(std::move(image), radius);
                        }

                        then(id, requestedSize, image, fileInfo.absoluteFilePath());
                        return;
                    }
                }
            }

            http::client()->download(
              "mxc://" + id.toStdString(),
              [fileInfo, requestedSize, then, id, radius, encryptionInfo](
                const std::string &res,
                const std::string &,
                const std::string &originalFilename,
                mtx::http::RequestErr err) {
                  if (err) {
                      nhlog::net()->error("Failed to download {}: {}", id.toStdString(), *err);
                      then(id, QSize(), {}, QLatin1String(""));
                      return;
                  }

                  auto tempData = res;
                  QFile f(fileInfo.absoluteFilePath());
                  if (!f.open(QIODevice::Truncate | QIODevice::WriteOnly)) {
                      nhlog::net()->error(
                        "Failed to write {}: {}", id.toStdString(), f.errorString().toStdString());
                      then(id, QSize(), {}, QLatin1String(""));
                      return;
                  }
                  f.write(tempData.data(), tempData.size());
                  f.close();
                  utils::markFileAsFromWeb(fileInfo.absoluteFilePath());

                  if (encryptionInfo) {
                      tempData = mtx::crypto::to_string(
                        mtx::crypto::decrypt_file(tempData, encryptionInfo.value()));
                      auto data    = QByteArray(tempData.data(), (int)tempData.size());
                      QImage image = utils::readImage(data);
                      if (radius != 0) {
                          image = clipRadius(std::move(image), radius);
                      }

                      image.setText(QStringLiteral("original filename"),
                                    QString::fromStdString(originalFilename));
                      image.setText(QStringLiteral("mxc url"), "mxc://" + id);

                      then(id, requestedSize, image, fileInfo.absoluteFilePath());
                      return;
                  }

                  QImage image = utils::readImageFromFile(fileInfo.absoluteFilePath());
                  if (radius != 0) {
                      image = clipRadius(std::move(image), radius);
                  }

                  image.setText(QStringLiteral("original filename"),
                                QString::fromStdString(originalFilename));
                  image.setText(QStringLiteral("mxc url"), "mxc://" + id);

                  then(id, requestedSize, image, fileInfo.absoluteFilePath());
              });
        } catch (std::exception &e) {
            nhlog::net()->error("Exception while downloading media: {}", e.what());
        }
    }
}

#include "moc_MxcImageProvider.cpp"
