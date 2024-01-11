// SPDX-FileCopyrightText: Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "MxcMediaProxy.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMediaMetaData>
#include <QMediaPlayer>
#include <QMimeDatabase>
#include <QStandardPaths>
#include <QUrl>

#include "ChatPage.h"
#include "EventAccessors.h"
#include "Logging.h"
#include "MatrixClient.h"
#include "timeline/TimelineModel.h"
#include "timeline/TimelineViewManager.h"

MxcMediaProxy::MxcMediaProxy(QObject *parent)
  : QMediaPlayer(parent)
{
    connect(
      this, &QMediaPlayer::errorOccurred, this, [](QMediaPlayer::Error error, QString errorString) {
          nhlog::ui()->debug("Media player error {} and errorStr {}",
                             static_cast<int>(error),
                             errorString.toStdString());
      });
    connect(
      this, &MxcMediaProxy::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status) {
          nhlog::ui()->info("Media player status {} and error {}",
                            static_cast<int>(status),
                            static_cast<int>(this->error()));
      });
    connect(
      this, &MxcMediaProxy::playbackStateChanged, this, [this](QMediaPlayer::PlaybackState status) {
          // We only set the output when starting the playback because otherwise the audio device
          // lookup takes about 500ms, which causes a lot of stutter...
          if (status == QMediaPlayer::PlayingState && !audioOutput()) {
              nhlog::ui()->debug("Set audio output");
              auto newOut = new QAudioOutput(this);
              newOut->setMuted(muted_);
              newOut->setVolume(QAudio::convertVolume(
                volume_, QAudio::LogarithmicVolumeScale, QAudio::LinearVolumeScale));
              setAudioOutput(newOut);
          }
      });
    connect(this, &MxcMediaProxy::metaDataChanged, [this]() { emit orientationChanged(); });

    connect(ChatPage::instance()->timelineManager()->rooms(),
            &RoomlistModel::currentRoomChanged,
            this,
            &MxcMediaProxy::pause);
}

int
MxcMediaProxy::orientation() const
{
    // nhlog::ui()->debug("metadata: {}",
    // availableMetaData().join(QStringLiteral(",")).toStdString());
    auto orientation = metaData().value(QMediaMetaData::Orientation).toInt();
    nhlog::ui()->debug("Video orientation: {}", orientation);
    return orientation;
}

void
MxcMediaProxy::startDownload(bool onlyCached)
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

    QString mxcUrl   = QString::fromStdString(mtx::accessors::url(*event));
    QString mimeType = QString::fromStdString(mtx::accessors::mimetype(*event));

    auto encryptionInfo = mtx::accessors::file(*event);

    // If the message is a link to a non mxcUrl, don't download it
    if (!mxcUrl.startsWith(QLatin1String("mxc://"))) {
        return;
    }

    QString suffix = QMimeDatabase().mimeTypeForName(mimeType).preferredSuffix();

    const auto url  = mxcUrl.toStdString();
    const auto name = QString(mxcUrl).remove(QStringLiteral("mxc://"));
    QFileInfo filename(
      QStringLiteral("%1/media_cache/media/%2.%3")
        .arg(QStandardPaths::writableLocation(QStandardPaths::CacheLocation), name, suffix));
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
            buffer.setData(temp.data(), static_cast<int>(temp.size()));
        } else {
            buffer.setData(device.readAll());
        }
        buffer.open(QIODevice::ReadOnly);
        buffer.reset();

        QTimer::singleShot(0, this, [this, filename] {
            nhlog::ui()->info(
              "Playing buffer with size: {}, {}", buffer.bytesAvailable(), buffer.isOpen());
            this->setSourceDevice(&buffer, QUrl(filename.fileName()));
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

    if (onlyCached)
        return;

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
