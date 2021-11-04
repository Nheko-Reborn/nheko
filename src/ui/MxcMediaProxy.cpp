// SPDX-FileCopyrightText: 2021 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "MxcMediaProxy.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMediaObject>
#include <QMediaPlayer>
#include <QMimeDatabase>
#include <QStandardPaths>
#include <QUrl>

#if defined(Q_OS_MACOS)
// TODO (red_sky): Remove for Qt6.  See other ifdef below
#include <QTemporaryFile>
#endif

#include "EventAccessors.h"
#include "Logging.h"
#include "MatrixClient.h"
#include "timeline/TimelineModel.h"

void
MxcMediaProxy::setVideoSurface(QAbstractVideoSurface *surface)
{
    qDebug() << "Changing surface";
    m_surface = surface;
    setVideoOutput(m_surface);
}

QAbstractVideoSurface *
MxcMediaProxy::getVideoSurface()
{
    return m_surface;
}

void
MxcMediaProxy::startDownload()
{
    if (!room_)
        return;
    if (eventId_.isEmpty())
        return;

    auto event = room_->eventById(eventId_);
    if (!event) {
        nhlog::ui()->error("Failed to load media for event {}, event not found.",
                           eventId_.toStdString());
        return;
    }

    QString mxcUrl           = QString::fromStdString(mtx::accessors::url(*event));
    QString originalFilename = QString::fromStdString(mtx::accessors::filename(*event));
    QString mimeType         = QString::fromStdString(mtx::accessors::mimetype(*event));

    auto encryptionInfo = mtx::accessors::file(*event);

    // If the message is a link to a non mxcUrl, don't download it
    if (!mxcUrl.startsWith("mxc://")) {
        return;
    }

    QString suffix = QMimeDatabase().mimeTypeForName(mimeType).preferredSuffix();

    const auto url  = mxcUrl.toStdString();
    const auto name = QString(mxcUrl).remove("mxc://");
    QFileInfo filename(QString("%1/media_cache/media/%2.%3")
                         .arg(QStandardPaths::writableLocation(QStandardPaths::CacheLocation))
                         .arg(name)
                         .arg(suffix));
    if (QDir::cleanPath(name) != name) {
        nhlog::net()->warn("mxcUrl '{}' is not safe, not downloading file", url);
        return;
    }

    QDir().mkpath(filename.path());

    QPointer<MxcMediaProxy> self = this;

    auto processBuffer = [this, encryptionInfo, filename, self, suffix](QIODevice &device) {
        if (!self)
            return;

        if (encryptionInfo) {
            QByteArray ba = device.readAll();
            std::string temp(ba.constData(), ba.size());
            temp = mtx::crypto::to_string(mtx::crypto::decrypt_file(temp, encryptionInfo.value()));
            buffer.setData(temp.data(), temp.size());
        } else {
            buffer.setData(device.readAll());
        }
        buffer.open(QIODevice::ReadOnly);
        buffer.reset();

        QTimer::singleShot(0, this, [this, filename, suffix, encryptionInfo] {
#if defined(Q_OS_MACOS)
            if (encryptionInfo) {
                // macOS has issues reading from a buffer in setMedia for whatever reason.
                // Instead, write the buffer to a temporary file and read from that.
                // This should be fixed in Qt6, so update this when we do that!
                // TODO: REMOVE IN QT6
                QTemporaryFile tempFile;
                tempFile.setFileTemplate(tempFile.fileTemplate() + QLatin1Char('.') + suffix);
                tempFile.open();
                tempFile.write(buffer.data());
                tempFile.close();
                nhlog::ui()->debug("Playing media from temp buffer file: {}.  Remove in QT6!",
                                   filename.filePath().toStdString());
                this->setMedia(QUrl::fromLocalFile(tempFile.fileName()));
            } else {
                nhlog::ui()->info(
                  "Playing buffer with size: {}, {}", buffer.bytesAvailable(), buffer.isOpen());
                this->setMedia(QUrl::fromLocalFile(filename.filePath()));
            }
#else
            Q_UNSUED(suffix)
            Q_UNUSED(encryptionInfo)

            nhlog::ui()->info(
              "Playing buffer with size: {}, {}", buffer.bytesAvailable(), buffer.isOpen());
            this->setMedia(QMediaContent(filename.fileName()), &buffer);
#endif
            emit loadedChanged();
        });
    };

    if (filename.isReadable()) {
        QFile f(filename.filePath());
        if (f.open(QIODevice::ReadOnly)) {
            processBuffer(f);
            return;
        }
    }

    http::client()->download(url,
                             [filename, url, processBuffer](const std::string &data,
                                                            const std::string &,
                                                            const std::string &,
                                                            mtx::http::RequestErr err) {
                                 if (err) {
                                     nhlog::net()->warn("failed to retrieve media {}: {} {}",
                                                        url,
                                                        err->matrix_error.error,
                                                        static_cast<int>(err->status_code));
                                     return;
                                 }

                                 try {
                                     QFile file(filename.filePath());

                                     if (!file.open(QIODevice::WriteOnly))
                                         return;

                                     QByteArray ba(data.data(), (int)data.size());
                                     file.write(ba);
                                     file.close();

                                     QBuffer buf(&ba);
                                     buf.open(QBuffer::ReadOnly);
                                     processBuffer(buf);
                                 } catch (const std::exception &e) {
                                     nhlog::ui()->warn("Error while saving file to: {}", e.what());
                                 }
                             });
}
