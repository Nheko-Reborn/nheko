// SPDX-FileCopyrightText: 2021 Nheko Contributors
// SPDX-FileCopyrightText: 2022 Nheko Contributors
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractVideoSurface>
#include <QIODevice>
#include <QImage>
#include <QObject>
#include <QSize>
#include <QStringList>
#include <QTimer>
#include <QUrl>
#include <QVariantList>
#include <deque>
#include <memory>

#include <mtx/common.hpp>
#include <mtx/responses/messages.hpp>

class TimelineModel;
class CombinedImagePackModel;
class QMimeData;
class QDropEvent;

enum class MarkdownOverride
{
    NOT_SPECIFIED, // no override set
    ON,
    OFF,
};

class InputVideoSurface : public QAbstractVideoSurface
{
    Q_OBJECT

public:
    InputVideoSurface(QObject *parent)
      : QAbstractVideoSurface(parent)
    {}

    bool present(const QVideoFrame &frame) override;

    QList<QVideoFrame::PixelFormat>
    supportedPixelFormats(QAbstractVideoBuffer::HandleType type) const override;

signals:
    void newImage(QImage img);
};

class MediaUpload : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int mediaType READ type NOTIFY mediaTypeChanged)
    // https://stackoverflow.com/questions/33422265/pass-qimage-to-qml/68554646#68554646
    Q_PROPERTY(QUrl thumbnail READ thumbnailDataUrl NOTIFY thumbnailChanged)
    //    Q_PROPERTY(QString humanSize READ humanSize NOTIFY huSizeChanged)
    Q_PROPERTY(QString filename READ filename WRITE setFilename NOTIFY filenameChanged)

    // thumbnail video
    // https://stackoverflow.com/questions/26229633/display-on-screen-using-qabstractvideosurface

public:
    enum MediaType
    {
        File,
        Image,
        Video,
        Audio,
    };
    Q_ENUM(MediaType)

    explicit MediaUpload(std::unique_ptr<QIODevice> data,
                         QString mimetype,
                         QString originalFilename,
                         bool encrypt,
                         QObject *parent = nullptr);

    [[nodiscard]] int type() const
    {
        if (mimeClass_ == u"video")
            return MediaType::Video;
        else if (mimeClass_ == u"audio")
            return MediaType::Audio;
        else if (mimeClass_ == u"image")
            return MediaType::Image;
        else
            return MediaType::File;
    }
    [[nodiscard]] QString url() const { return url_; }
    [[nodiscard]] QString mimetype() const { return mimetype_; }
    [[nodiscard]] QString mimeClass() const { return mimeClass_; }
    [[nodiscard]] QString filename() const { return originalFilename_; }
    [[nodiscard]] QString blurhash() const { return blurhash_; }
    [[nodiscard]] uint64_t size() const { return size_; }
    [[nodiscard]] uint64_t duration() const { return duration_; }
    [[nodiscard]] std::optional<mtx::crypto::EncryptedFile> encryptedFile_()
    {
        return encryptedFile;
    }
    [[nodiscard]] std::optional<mtx::crypto::EncryptedFile> thumbnailEncryptedFile_()
    {
        return thumbnailEncryptedFile;
    }
    [[nodiscard]] QSize dimensions() const { return dimensions_; }

    QImage thumbnailImg() const { return thumbnail_; }
    QString thumbnailUrl() const { return thumbnailUrl_; }
    QUrl thumbnailDataUrl() const;
    [[nodiscard]] uint64_t thumbnailSize() const { return thumbnailSize_; }

    void setFilename(QString fn)
    {
        if (fn != originalFilename_) {
            originalFilename_ = std::move(fn);
            emit filenameChanged();
        }
    }

signals:
    void uploadComplete(MediaUpload *self, QString url);
    void uploadFailed(MediaUpload *self);
    void filenameChanged();
    void thumbnailChanged();
    void mediaTypeChanged();

public slots:
    void startUpload();

private slots:
    void setThumbnail(QImage img)
    {
        this->thumbnail_ = std::move(img);
        emit thumbnailChanged();
    }

public:
    // void uploadThumbnail(QImage img);

    std::unique_ptr<QIODevice> source;
    QByteArray data;
    QString mimetype_;
    QString mimeClass_;
    QString originalFilename_;
    QString blurhash_;
    QString thumbnailUrl_;
    QString url_;
    std::optional<mtx::crypto::EncryptedFile> encryptedFile, thumbnailEncryptedFile;

    QImage thumbnail_;

    QSize dimensions_;
    uint64_t size_          = 0;
    uint64_t thumbnailSize_ = 0;
    uint64_t duration_      = 0;
    bool encrypt_;
};

class InputBar : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool uploading READ uploading NOTIFY uploadingChanged)
    Q_PROPERTY(bool containsAtRoom READ containsAtRoom NOTIFY containsAtRoomChanged)
    Q_PROPERTY(QString text READ text NOTIFY textChanged)
    Q_PROPERTY(QVariantList uploads READ uploads NOTIFY uploadsChanged)

public:
    explicit InputBar(TimelineModel *parent)
      : QObject()
      , room(parent)
    {
        typingRefresh_.setInterval(10'000);
        typingRefresh_.setSingleShot(true);
        typingTimeout_.setInterval(5'000);
        typingTimeout_.setSingleShot(true);
        connect(&typingRefresh_, &QTimer::timeout, this, &InputBar::startTyping);
        connect(&typingTimeout_, &QTimer::timeout, this, &InputBar::stopTyping);
    }

    QVariantList uploads() const;

public slots:
    [[nodiscard]] QString text() const;
    QString previousText();
    QString nextText();
    void setText(const QString &newText);

    [[nodiscard]] bool containsAtRoom() const { return containsAtRoom_; }

    void send();
    bool tryPasteAttachment(bool fromMouse);
    bool insertMimeData(const QMimeData *data);
    void updateState(int selectionStart, int selectionEnd, int cursorPosition, const QString &text);
    void openFileSelection();
    [[nodiscard]] bool uploading() const { return uploading_; }
    void message(const QString &body,
                 MarkdownOverride useMarkdown = MarkdownOverride::NOT_SPECIFIED,
                 bool rainbowify              = false);
    void reaction(const QString &reactedEvent, const QString &reactionKey);
    void sticker(CombinedImagePackModel *model, int row);

    void acceptUploads();
    void declineUploads();

private slots:
    void startTyping();
    void stopTyping();

    void finalizeUpload(MediaUpload *upload, QString url);
    void removeRunUpload(MediaUpload *upload);

signals:
    void textChanged(QString newText);
    void uploadingChanged(bool value);
    void containsAtRoomChanged();
    void uploadsChanged();

private:
    void emote(const QString &body, bool rainbowify);
    void notice(const QString &body, bool rainbowify);
    void command(const QString &name, QString args);
    void image(const QString &filename,
               const std::optional<mtx::crypto::EncryptedFile> &file,
               const QString &url,
               const QString &mime,
               uint64_t dsize,
               const QSize &dimensions,
               const std::optional<mtx::crypto::EncryptedFile> &thumbnailEncryptedFile,
               const QString &thumbnailUrl,
               uint64_t thumbnailSize,
               const QSize &thumbnailDimensions,
               const QString &blurhash);
    void file(const QString &filename,
              const std::optional<mtx::crypto::EncryptedFile> &encryptedFile,
              const QString &url,
              const QString &mime,
              uint64_t dsize);
    void audio(const QString &filename,
               const std::optional<mtx::crypto::EncryptedFile> &file,
               const QString &url,
               const QString &mime,
               uint64_t dsize,
               uint64_t duration);
    void video(const QString &filename,
               const std::optional<mtx::crypto::EncryptedFile> &file,
               const QString &url,
               const QString &mime,
               uint64_t dsize,
               uint64_t duration,
               const QSize &dimensions,
               const std::optional<mtx::crypto::EncryptedFile> &thumbnailEncryptedFile,
               const QString &thumbnailUrl,
               uint64_t thumbnailSize,
               const QSize &thumbnailDimensions,
               const QString &blurhash);

    void startUploadFromPath(const QString &path);
    void startUploadFromMimeData(const QMimeData &source, const QString &format);
    void startUpload(std::unique_ptr<QIODevice> dev, const QString &orgPath, const QString &format);
    void setUploading(bool value)
    {
        if (value != uploading_) {
            uploading_ = value;
            emit uploadingChanged(value);
        }
    }

    void updateAtRoom(const QString &t);

    QTimer typingRefresh_;
    QTimer typingTimeout_;
    TimelineModel *room;
    std::deque<QString> history_;
    std::size_t history_index_ = 0;
    int selectionStart = 0, selectionEnd = 0, cursorPosition = 0;
    bool uploading_      = false;
    bool containsAtRoom_ = false;

    struct DeleteLaterDeleter
    {
        void operator()(QObject *p)
        {
            if (p)
                p->deleteLater();
        }
    };
    using UploadHandle = std::unique_ptr<MediaUpload, DeleteLaterDeleter>;
    std::vector<UploadHandle> unconfirmedUploads;
    std::vector<UploadHandle> runningUploads;
};
